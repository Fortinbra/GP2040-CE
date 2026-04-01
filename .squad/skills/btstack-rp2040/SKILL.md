# BTstack RP2040 Patterns & Best Practices

This document captures reusable patterns for BTstack BLE development on RP2040/RP2350 with the Pico SDK.

## Pattern: NEVER Set _notificationsEnabled on SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED

**Confidence:** CRITICAL — Proven root cause of bonded reconnect disconnect/loop (2025-01-31)

**Context:** On bonded device reconnect (power cycle or disconnect/reconnect), BTstack fires `SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED` when the Security Manager resolves the peer's IRK against the bonded device database. This event fires **before** the LTK encryption handshake completes.

**Anti-pattern (causes disconnect loop):**
```cpp
case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
    mgr._notificationsEnabled = true;  // ❌ NEVER — link is not yet encrypted
    mgr._hasBondedPeers = true;
    break;
```

**Result:** ATT notifications sent on unencrypted link → ATT security error → disconnect → Windows/macOS retries immediately → infinite loop.

**Correct pattern (gate on encryption complete):**
```cpp
// In _hciPacketHandler, handle HCI_EVENT_ENCRYPTION_CHANGE instead:
case HCI_EVENT_ENCRYPTION_CHANGE: {
    uint8_t encStatus = hci_event_encryption_change_get_status(packet);
    if (encStatus == ERROR_CODE_SUCCESS && mgr._connected) {
        mgr._notificationsEnabled = true;  // ✅ Link is now encrypted
    }
    break;
}

// In _smPacketHandler, SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED only sets:
case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
    mgr._hasBondedPeers = true;  // ✅ Identity confirmed (informational)
    break;
```

**Why:** The correct event ordering for a bonded BLE reconnect is:
1. `HCI_SUBEVENT_LE_CONNECTION_COMPLETE` — link exists, unencrypted
2. `SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED` — identity confirmed, still unencrypted
3. `HCI_EVENT_ENCRYPTION_CHANGE` (status=SUCCESS) — link is now encrypted ← **safe to send ATT notifications**
4. `SM_EVENT_PAIRING_COMPLETE` — (on fresh pair only)
5. `HIDS_SUBEVENT_INPUT_REPORT_ENABLE` — (on fresh pair only; host may skip on reconnect)

`SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED` is purely an identity confirmation event. It MUST NEVER be used to gate ATT/GATT operations that require link encryption.

**Key Rule:** Gate all ATT/GATT notification sends on `HCI_EVENT_ENCRYPTION_CHANGE` (status==ERROR_CODE_SUCCESS && connected), never on SM identity events.

---

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

**CRITICAL: Always use SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING for modern OS compatibility.** Windows 10+, macOS 11+, iOS, and Android prefer LE Secure Connections (LESC). If the device is configured for legacy bonding only (`SM_AUTHREQ_BONDING` without SC), the OS will pair using SC and store an SC-LTK. On reconnect, the OS presents the SC-LTK but the device cannot accept it → encryption fails → disconnect loop (HCI reason 0x13). Adding `SM_AUTHREQ_SECURE_CONNECTION` enables the device to accept SC-LTKs on reconnect.

**Implication of adding SC:** Existing legacy bonds are incompatible with SC mode. Users must delete old pairings and re-pair once after enabling SC.

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

---

## Pattern: Aggressive Bond Clearing on SM_EVENT_IDENTITY_RESOLVING_FAILED

**Confidence:** HIGH — Critical for Secure Connections (SC) pairing after enabling `SM_AUTHREQ_SECURE_CONNECTION`

**Context:** When `SM_EVENT_IDENTITY_RESOLVING_FAILED` fires, BTstack couldn't match the connecting peer's IRK to any stored bond. This typically means:
1. The host deleted its side of the pairing but the Pico still has a stored bond entry
2. The bond is stale (e.g., legacy non-SC bond when SC is now required)
3. The peer re-paired on their side without notifying the device

**Problem:** Simply calling `sm_request_pairing()` is insufficient. BTstack will keep trying to resolve the stale bond entry, preventing fresh SC pairing from completing. This creates a fast reconnect loop.

**Solution:** Clear ALL stored bonds BEFORE requesting fresh pairing:

```cpp
case SM_EVENT_IDENTITY_RESOLVING_FAILED: {
    hci_con_handle_t handle = sm_event_identity_resolving_failed_get_handle(packet);
    // Stale or mismatched bond — wipe all stored bonds so fresh SC pairing can complete.
    // The host will need to remove and re-add the device on their side as well.
    int deviceCount = le_device_db_count();
    for (int i = deviceCount - 1; i >= 0; i--) {
        le_device_db_remove(i);
    }
    mgr._hasBondedPeers = false;
    sm_request_pairing(handle);
    break;
}
```

**Why:**
- **Clearing bonds BEFORE `sm_request_pairing()` is critical** — the stale entry must be gone before BTstack can accept new SC pairing
- **Reverse iteration is safe** — removing from highest index to lowest avoids index shifting bugs
- **No `le_device_db_remove_all()` exists** in BTstack — must iterate manually using `le_device_db_count()` and `le_device_db_remove(index)`

**When to use:**
- After enabling `SM_AUTHREQ_SECURE_CONNECTION` on a device that previously used legacy bonding
- When reconnect loops occur after OS updates (Windows 10→11, macOS upgrades)
- When identity resolution consistently fails despite valid connections

**Trade-off:** This is an aggressive approach — it clears ALL bonds, not just the mismatched one. But BTstack doesn't provide enough context to identify which specific bond is stale, and identity resolution failure is rare in normal operation, so security benefit outweighs UX cost.


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
## Pattern: Defer State-Changing API Calls from IRQ Context to Main Loop

**Confidence:** HIGH (confirmed root cause of advertising restart race, 2026-03-31)

**Context:** BTstack packet handlers run in IRQ context (`sync_context_threadsafe_background` periodic alarm). Calling state-changing API functions (e.g., `gap_advertisements_enable`, `hci_disconnect`, `sm_request_pairing`) directly from packet handlers can race with BTstack's internal state machines.

**Anti-pattern (race condition):**
```cpp
case HCI_EVENT_DISCONNECTION_COMPLETE: {
    // ... clear state ...
    gap_advertisements_enable(1);  // ❌ Called from IRQ context → races with LL cleanup
    break;
}
```

**Correct pattern (defer to main loop):**
```cpp
// In headers/BLEHIDManager.h:
volatile bool _needsAdvRestart = false;

// In packet handler (IRQ context):
case HCI_EVENT_DISCONNECTION_COMPLETE: {
    // ... clear state ...
    _needsAdvRestart = true;  // ✅ Set flag, don't call API
    break;
}

// In process() main loop (after cyw43_arch_poll):
if (_needsAdvRestart && !_connected) {
    gap_advertisements_enable(1);  // ✅ Called from main loop, safe
    _needsAdvRestart = false;
}
```

**Why:** BTstack's Link Layer state machine cleans up the connection asynchronously after `HCI_EVENT_DISCONNECTION_COMPLETE` fires. Calling `gap_advertisements_enable` while the LL is still in the cleanup phase can cause undefined behavior (advertising starts but LL state is stale → next connection attempt fails → disconnect loop).

**Key Rule:** Packet handlers should ONLY:
1. Read event data (safe — data is read-only in handler context)
2. Set flags (`volatile` variables written in IRQ, read in main loop)
3. Call BTstack "response" APIs that are explicitly IRQ-safe (e.g., `sm_just_works_confirm`, documented as callable from handler)

**Prohibited in packet handlers:**
- `gap_advertisements_enable` / `gap_advertisements_disable`
- `hci_disconnect`
- `sm_request_pairing` (unless documented as IRQ-safe)
- Any function that modifies GAP/SM/L2CAP state

**When to defer:** If an API function modifies connection state, advertising state, or initiates a new protocol exchange, defer it to the main loop via a `volatile bool` flag.

---

## Pattern: Handle SM_EVENT_IDENTITY_RESOLVING_FAILED for Bond Mismatch Recovery

**Confidence:** HIGH (correct recovery path for bond state mismatch, 2026-03-31)

**Context:** `SM_EVENT_IDENTITY_RESOLVING_FAILED` fires when BTstack cannot match a reconnecting peer's IRK (Identity Resolving Key) to any stored bond. This happens when:
- User deletes the Bluetooth pairing on the host (Windows, macOS, etc.) but the peripheral still has a stored bond
- Bond database entry is corrupted or stale
- IRK mismatch due to out-of-sync bond state

**Anti-pattern (silent disconnect):**
```cpp
// SM_EVENT_IDENTITY_RESOLVING_FAILED not handled → connection proceeds in undefined state → silent disconnect
```

**Correct pattern (graceful re-pairing):**
```cpp
case SM_EVENT_IDENTITY_RESOLVING_FAILED:
    // BTstack could not match reconnecting peer's IRK to any stored bond.
    // Request a fresh pairing exchange to recover.
    sm_request_pairing(sm_event_identity_resolving_failed_get_handle(packet));
    break;
```

**Why:** Without handling this event, the connection proceeds with no valid bond → BTstack may silently disconnect the peer (HCI reason 0x13 or 0x16) → user sees reconnect loop with no pairing dialog.

**Correct behavior:** When IRK resolution fails, trigger a fresh pairing exchange. The host sees a pairing dialog → user can re-pair → new bond established → future reconnects succeed.

**Key Rule:** Peripherals that use bonding (`SM_AUTHREQ_BONDING`) MUST handle `SM_EVENT_IDENTITY_RESOLVING_FAILED` and call `sm_request_pairing` to recover from bond state mismatch.

**Note:** Do NOT call `sm_request_pairing` from IRQ context in high-traffic scenarios. For BLE HID reconnect (single-peer, low-frequency event), it is safe. For multi-peer scenarios, consider deferring via flag.

---


## Pattern: Replace le_device_db_tlv with Custom Protobuf Backend

**Confidence:** CONFIRMED — Implemented and built clean (2026-03-31)

**Context:** BTstack's default bond storage (le_device_db_tlv.c) uses pico_flash_bank_instance() which maps to the top of flash — the same region as GP2040-CE's FlashPROM. Every config save corrupts the bond bank.

**Fix:** Implement the le_device_db.h interface directly, backed by protobuf config.

### CMake: exclude SDK's le_device_db_tlv.c

`cmake
# Inside if(PICO_CYW43_SUPPORTED) block, AFTER target_sources/target_link_libraries:
set_source_files_properties(
    "${PICO_SDK_PATH}/lib/btstack/src/ble/le_device_db_tlv.c"
    PROPERTIES HEADER_FILE_ONLY TRUE
)
`

HEADER_FILE_ONLY TRUE suppresses compilation of a specific source file that was added by an INTERFACE library. No CMake 3.18+ required (no TARGET_DIRECTORY needed when setting the property in the same directory scope as the consuming target).

### le_device_db_memory.c is already safe

le_device_db_memory.c has #ifndef NVM_NUM_DEVICE_DB_ENTRIES at the top — when NVM_NUM_DEVICE_DB_ENTRIES is defined (as it is in tstack_config.h), the entire file compiles to nothing. No action needed.

### Required noop stub: le_device_db_tlv_configure

tstack_cyw43.c (Pico SDK, not project code) calls le_device_db_tlv_configure() unconditionally from tstack_cyw43_init(). This is the ONLY SDK caller. Add this noop to your custom le_device_db implementation:

`cpp
#include "ble/le_device_db_tlv.h"  // for the declaration

void le_device_db_tlv_configure(const btstack_tlv_t* btstack_tlv_impl,
                                 void* btstack_tlv_context)
{
    (void)btstack_tlv_impl;
    (void)btstack_tlv_context;
    // protobuf backend needs no TLV wiring
}
`

### BTstack header include path

BTstack headers live under src/ble/ in the BTstack tree. Always use:
- #include "ble/le_device_db.h" (NOT "le_device_db.h")
- #include "ble/le_device_db_tlv.h"

### Nanopb bytes fields in C++

For optional bytes field = N [(nanopb).max_size = K] on a message Foo, nanopb generates:
`c
typedef PB_BYTES_ARRAY_T(K) Foo_field_t;
typedef struct _Foo {
    bool has_field;
    Foo_field_t field;   // .size (pb_size_t) + .bytes[K]
} Foo;
`
Access bytes as ntry.addr.bytes (array) and ntry.addr.size (length in use).

### LRU eviction pattern for fixed-size bond array

`cpp
static int _find_lru(const BLEConfig& c) {
    int lru_idx = 0;
    uint32_t lru_seq = c.bonds[0].seqNr;
    for (int i = 1; i < 4; i++) {
        if (c.bonds[i].seqNr < lru_seq) { lru_seq = c.bonds[i].seqNr; lru_idx = i; }
    }
    return lru_idx;
}
`
Increment seqCounter on every le_device_db_add(), store it in the slot's seqNr.

---

---
## Battery Voltage Reading on CYW43 Boards (RP2350B / Pico W / Pico 2W)

**Confidence:** CONFIRMED — Implemented and built clean (2026-04-01)

### `cyw43_get_battery_voltage()` does NOT exist

Searched `cyw43_arch.h` and `cyw43-driver/src/cyw43.h` in Pico SDK 2.2.0 — no battery voltage function found anywhere in the CYW43 driver or pico_cyw43_arch. Do NOT assume this API exists.

### Safe approach: Direct ADC read on GPIO29

GPIO29 is shared with WL_CLK (CYW43 SPI clock), but ADC reads coexist safely. The key rule from BoardConfig.h for PimoroniPicoLipo2XLW:

> "GPIO29 is shared with WL_CLK but ADC reads coexist safely — no GPIO conflict."

You can call `adc_gpio_init(29)` and `adc_read()` **after** `cyw43_arch_init()` without issues on RP2350B.

### Correct call sequence

```cpp
// In _doInit(), AFTER cyw43_arch_init():
adc_init();
adc_gpio_init(BATTERY_ADC_GPIO);   // 29 on PimoroniPicoLipo2XLW

// Helper function:
adc_select_input(BATTERY_ADC_CHANNEL);  // 3 on PimoroniPicoLipo2XLW
uint16_t raw = adc_read();
// LiPo 3:1 divider, 3.3V ref, 12-bit:
//   raw ≤ 1241 → 0%,  raw ≥ 1737 → 100%,  span = 496 counts
```

### BTstack `battery_service_server` API

`battery_service_server_init(uint8_t level)` MUST be called before `hci_power_control()`. It registers the service with the ATT server. `battery_service_server_set_battery_value(level)` updates the value and triggers GATT notifications to subscribed clients.

**Do NOT call only `_set_battery_value()` without first calling `_init()`** — the service will not be registered in the ATT database.

### CMake: no explicit source needed

`pico_btstack_ble` (Pico SDK INTERFACE library) already includes `battery_service_server.c` (line 87 in `pico_btstack`'s CMakeLists). No need to add it manually.

### GATT: already included via

```
#import <battery_service.gatt>
```

in your `.gatt` file. This provides the standard Battery Service (UUID 0x180F) characteristics.
