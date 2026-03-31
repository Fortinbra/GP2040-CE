# BTstack RP2040 Patterns & Best Practices

This document captures reusable patterns for BTstack BLE development on RP2040/RP2350 with the Pico SDK.

## Pattern: volatile for IRQ-Shared State

**Context:** BTstack runs in `sync_context_threadsafe_background`, which fires from a periodic alarm IRQ on the same core as the main thread (not SMP). The IRQ preempts the main thread at instruction boundaries.

**Problem:** Without `volatile`, the compiler caches shared state in registers. The main thread never sees IRQ-written values.

**Solution:** Declare all state variables that are written in BTstack event handlers and read in the main thread as `volatile`:

```cpp
class BLEHIDManager {
private:
    volatile bool     _connected;
    volatile bool     _notificationsEnabled;
    volatile bool     _reportPending;
    volatile uint16_t _conHandle;
    volatile uint16_t _pendingReportLen;
    // ...
};
```

**Why:** ARM Cortex-M's sequential store model ensures that `memcpy(_pendingReport, ...)` writes are visible to the IRQ handler after `_reportPending = true` is written, because the IRQ can only preempt AFTER all instructions in program order complete. `volatile` ensures the main thread re-reads these variables on every access.

**When NOT needed:** Variables that are only read by IRQ handlers (e.g., read-only state passed to BTstack) or only written by the main thread (e.g., `_initialized`, `_bootTimeMs`) do NOT need `volatile`.

---

## Pattern: CAN_SEND_NOW for BLE HID Report Transmission

**Context:** BTstack BLE HID uses a "request can send now" mechanism to avoid blocking when the ATT layer is busy.

**Anti-pattern (blocking):**
```cpp
void sendReport(const uint8_t* report, uint16_t len) {
    hids_device_send_input_report(_conHandle, report, len);  // ❌ May fail if busy
}
```

**Correct pattern (non-blocking):**
```cpp
// 1. Cache the report in process()
bool sendReport(const uint8_t* report, uint16_t len) {
    if (!_connected || !_notificationsEnabled) return false;
    memcpy(_pendingReport, report, len);
    _pendingReportLen = len;
    _reportPending = true;
    return true;
}

// 2. Request CAN_SEND_NOW in the main loop
void process() {
    cyw43_arch_poll();
    if (_reportPending && _connected && _notificationsEnabled) {
        hids_device_request_can_send_now_event(_conHandle);
    }
}

// 3. Send when HIDS_SUBEVENT_CAN_SEND_INPUT_NOW arrives
static void _hciPacketHandler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
    if (packet_type == HCI_EVENT_PACKET) {
        uint8_t event = hci_event_packet_get_type(packet);
        if (event == HCI_EVENT_HIDS_META) {
            uint8_t subevent = hci_event_hids_meta_get_subevent_code(packet);
            if (subevent == HIDS_SUBEVENT_CAN_SEND_INPUT_NOW) {
                if (_reportPending) {
                    hids_device_send_input_report(_conHandle, _pendingReport, _pendingReportLen);
                    _reportPending = false;
                }
            }
        }
    }
}
```

**Why:** BTstack's ATT notification layer is rate-limited by the BLE connection interval and the remote device's receive buffers. `hids_device_request_can_send_now_event()` queues a request; BTstack calls your handler when the layer is ready.

---

## Pattern: sm_init() Must Precede att_server_init()

**Context:** BTstack requires Security Manager initialization before any ATT/GATT service layer setup.

**Anti-pattern:**
```cpp
void _doInit() {
    sm_init();
    att_server_init(...);         // ✅ Correct order
    hids_device_init(...);        // ✅ Correct order
    battery_service_server_init(...);  // ✅ Correct order
}
```

**Correct order:**
```cpp
void _doInit() {
    // 1. Core protocol stack
    l2cap_init();
    
    // 2. Security Manager (MUST be before att_server)
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);
    
    // 3. ATT/GATT layer
    att_server_init(profile_data, NULL, NULL);
    
    // 4. Service layer (HID, Battery, etc.)
    hids_device_init(...);
    battery_service_server_init(...);
    device_information_service_server_init();
}
```

**Why:** `att_server_init()` and service inits may register callbacks that depend on Security Manager state. Out-of-order initialization can cause silent failures or bonding issues.

---

## Pattern: Report ID in Descriptor vs ATT Notification Payload

**Context:** BLE HID Report Descriptors include a Report ID item (`0x85, 0x01`), but the ATT notification payload does NOT include the Report ID byte.

**Report Descriptor:**
```cpp
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,        // USAGE (Game Pad)
    0xA1, 0x01,        // COLLECTION (Application)
    0x85, 0x01,        // Report ID (1)  ← Descriptor declares this
    // ... 32 buttons, hat, 4 axes ...
    0xC0,              // END_COLLECTION
};
```

**ATT Notification Payload (9 bytes, NO Report ID):**
```cpp
uint8_t report[9] = {};
report[0] = (uint8_t)(state.buttons & 0xFF);         // Buttons byte 0
report[1] = (uint8_t)((state.buttons >> 8) & 0xFF);  // Buttons byte 1
report[2] = (uint8_t)((state.buttons >> 16) & 0xFF); // Buttons byte 2
report[3] = (uint8_t)((state.buttons >> 24) & 0xFF); // Buttons byte 3
report[4] = hat & 0x0F;                              // Hat + padding
report[5] = (uint8_t)(state.lx >> 8);                // Axis X
report[6] = (uint8_t)(state.ly >> 8);                // Axis Y
report[7] = (uint8_t)(state.rx >> 8);                // Axis Z
report[8] = (uint8_t)(state.ry >> 8);                // Axis Rz
hids_device_send_input_report(_conHandle, report, 9);
```

**GATT Attribute (Report Reference Descriptor):**
```
// In ble_hid.gatt (GATT database source):
CHARACTERISTIC, 2A4D, READ | NOTIFY,
    // Report body characteristic
REPORT_REFERENCE, READ, 01 01
    // ^^^^^^^^^^  ^^^^^^^^  ^^^^^
    // 0x2908       readable  [Report ID = 1, Report Type = 1 (Input)]
```

**Why:** The Report Reference descriptor (UUID 0x2908) conveys the Report ID to the GATT client. The ATT notification payload contains only the report body (buttons, hat, axes) — the GATT client infers the Report ID from the characteristic's Report Reference descriptor.

**Key Rule:** BLE HID descriptor body = USB HID descriptor body + Report ID item. ATT notification payload = USB HID report struct (no Report ID byte).

---

## Pattern: Avoid TinyUSB Headers in BTstack Translation Units

**Problem:** Both TinyUSB (`tusb.h`) and BTstack (`btstack.h`) define `hid_report_type_t`. Including both in the same `.cpp` file causes a compile error:
```
error: conflicting declaration 'typedef enum hid_report_type_t hid_report_type_t'
```

**Solution:** Isolate BLE HID Manager in its own translation unit and **NEVER include `tusb.h` or any TinyUSB header**:

```cpp
// BLEHIDManager.cpp — CORRECT
// IMPORTANT: Do NOT include tusb.h or any TinyUSB header in this translation unit.
// hid_report_type_t is defined by both TinyUSB and BTstack — including both
// in the same translation unit causes a compile error.

#include "BLEHIDManager.h"
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "btstack.h"
// ❌ NEVER: #include "tusb.h"
```

**Why:** C++ does not allow conflicting typedefs. TinyUSB and BTstack are independent stacks that happen to define the same enum name. They must be compiled in separate translation units.

**Workaround if you need both:** Use a facade/adapter pattern — call USB HID functions from a separate `.cpp` file that includes only TinyUSB headers, and call BLE HID functions from BLEHIDManager.cpp. Pass primitive types (uint8_t, int) across the boundary, not TinyUSB/BTstack types.

---

## Pattern: GPIO Reading Is Transport-Agnostic

**Context:** In a multi-transport gamepad firmware (USB HID, BLE HID, future WiFi HID), GPIO reading must happen BEFORE output dispatch, regardless of output transport.

**Anti-pattern (output-specific GPIO reading):**
```cpp
// USB mode
if (inputMode == USB) {
    inputDriver->process(gamepad);  // Reads GPIO + sends USB report
}

// BLE mode
if (inputMode == BLE) {
    // ❌ GPIO never read — OutputManager::dispatch sends all-zeros
    OutputManager::dispatch(gamepad);
}
```

**Correct pattern (GPIO reading independent of output):**
```cpp
// ALWAYS read GPIO from hardware
if (inputDriver != nullptr) {
    inputDriver->process(gamepad);  // Populates gamepad->state from GPIO
}

// Dispatch to the correct output transport
OutputManager::dispatch(gamepad);   // Sends to BLE if inputMode == BLE
```

**Why:**
1. `inputDriver->process(gamepad)` reads physical button/stick state into `gamepad->state` (buttons, dpad, axes).
2. It returns `true` if it sent a USB HID report (USB modes) or `false` otherwise (BLE, network, etc.).
3. The return value is only used by `addons.PostprocessAddons(processed)` for USB-specific addon timing.
4. `OutputManager::dispatch()` guards on `inputMode == INPUT_MODE_BLE` internally — it's safe to call in all modes.

**Key Rule:** GPIO input reading is orthogonal to output transport. Don't gate GPIO reading on output mode.

---

## Pattern: LED Blink Diagnostics for BLE-Only Mode

**Context:** When firmware runs in wireless-only mode (no TinyUSB, no USB serial), printf debugging is unavailable. LED blink patterns provide visual feedback for pipeline diagnostics.

**Problem:** Debugging BLE HID report transmission without USB serial or hardware debugger.

**Solution:** Use the onboard LED (GPIO 25 on standard Pico, or `CYW43_WL_GPIO_LED_PIN` via `cyw43_arch_gpio_put()` on Pico W/Pico 2 W) with distinct blink patterns for key events:

```cpp
// In BTstack event handler — notifications enabled
case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
    if (hids_subevent_input_report_enable_get_enable(packet) != 0) {
        // 5 fast blinks = notifications enabled by host
        for (int i = 0; i < 5; i++) {
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            sleep_ms(50);
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
            sleep_ms(50);
        }
    }
    break;

// In BTstack event handler — report sent
case HIDS_SUBEVENT_CAN_SEND_NOW:
    hids_device_send_input_report(_conHandle, report, len);
    // Single 50µs pulse = report sent
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_us(50);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    break;

// In output dispatch — button detection
if (state.buttons != 0) {
    // 2 rapid pulses = buttons detected in GPIO
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_us(30);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_us(30);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_us(30);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
}
```

**Pattern vocabulary:**
- **Fast blinks (50-100ms):** High-level events (connection, pairing, notifications enabled)
- **Slow blinks (150-300ms):** Error states or disabled features
- **Microsecond pulses:** Per-iteration events (reports sent, data detected)
- **Distinct counts:** 1 = basic event, 2 = data present, 3 = warning, 5 = success

**Why:** LED diagnostics are the only real-time feedback mechanism when USB serial is unavailable. Blink patterns allow you to trace execution flow and identify pipeline failures (e.g., "5 fast blinks never appear → Windows not subscribing to notifications").

**When to remove:** LED diagnostics add latency (especially `sleep_ms()` calls in IRQ context — use only in background thread). Wrap in `#ifdef BLE_DEBUG_LED` for easy removal after debugging.

---

## Pattern: hids_device_register_packet_handler() MUST Be Called Separately

**Confidence:** HIGH (confirmed working, validated against BTstack reference implementation)

**Context:** BTstack's HIDS profile requires event handlers for both general HCI events and HIDS-specific meta events.

**Problem:** Single event handler registration is insufficient. Calling only `hci_add_event_handler()` without `hids_device_register_packet_handler()` results in:
- HIDS meta events (HIDS_SUBEVENT_INPUT_REPORT_ENABLE, HIDS_SUBEVENT_CAN_SEND_NOW, etc.) never delivered to your handler
- Device connects successfully, but notifications never enabled
- Button presses sent but reports blocked at BTstack HIDS layer
- CCCD looks correct on host side; firmware unaware of subscription

**Solution:** Register BOTH handlers:

```cpp
class BLEHIDManager {
private:
    static void _hciPacketHandler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size) {
        // Handle HCI_EVENT_HIDS_META packets
        if (packet_type == HCI_EVENT_PACKET) {
            uint8_t event = hci_event_packet_get_type(packet);
            if (event == HCI_EVENT_HIDS_META) {
                uint8_t subevent = hci_event_hids_meta_get_subevent_code(packet);
                if (subevent == HIDS_SUBEVENT_INPUT_REPORT_ENABLE) {
                    uint8_t enable = hci_event_hids_meta_input_report_enable_get_enable(packet);
                    _notificationsEnabled = (enable != 0);
                }
                // ... handle other HIDS subevents
            }
        }
    }
    
public:
    void _doInit() {
        // ... other initialization ...
        
        // 1. Initialize HIDS profile instance
        hids_device_init(...);
        
        // 2. **CRITICAL:** Register HIDS packet handler
        hids_device_register_packet_handler(_hciPacketHandler);
        
        // 3. Setup GATT
        att_server_init(...);
        
        // 4. Register general HCI handler (for connection/disconnection events)
        hci_add_event_handler(...);
        
        // ... rest of setup ...
    }
};
```

**Why:** BTstack's architectural design gives service-specific handlers first access to events. When `hids_device_register_packet_handler()` is registered, HIDS meta events are routed to that handler ONLY — they do NOT appear in the general HCI event handler. This is by design, not a bug, but it's a common source of silent failures.

**Architectural Rule:** For any BTstack service (HIDS, Battery Service, Device Information Service, etc.) that emits meta events, you must register a service-specific handler to receive those events, even if you also have a general HCI handler.

**Symptom Checklist:**
- ✓ Device advertises and connects normally — HCI handlers working
- ✓ GATT profile looks correct on host (correct UUIDs, permissions) — GATT working
- ✗ No HIDS_SUBEVENT_INPUT_REPORT_ENABLE after host subscribes — HIDS handler not registered
- ✗ Notifications queued but never sent — CAN_SEND_NOW event not received — HIDS handler not registered

**References:**
- BTstack `hog_keyboard_demo.c` — official reference implementation
- GP2040-CE commit 0291e55a — confirmed working BLE HID with proper handler registration
- BTstack HIDS profile documentation: `btstack/src/ble/hids_device.c`

---

## References

- **Pico SDK 2.2.0 BTstack examples:** `pico-examples/pico_w/bt/standalone/`
- **BTstack documentation:** https://bluekitchen-gmbh.com/btstack/
- **GP2040-CE BLE HID commits:** c494add2, 176109ef, e2584c20, 0291e55a (HIDS handler registration)
- **Edward's history:** `.squad/agents/edward/history.md`