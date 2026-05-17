# Bluetooth Low Energy (BLE) HID Support — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning / Not yet implemented  
**SDK version:** 2.2.0

---

## Overview

This document describes the planned **Bluetooth Low Energy (BLE) HID** support for GP2040-CE. BLE HID is a **distinct protocol** from Bluetooth HID Classic (covered in `docs/development/bluetooth-support.md`) with different characteristics, pairing requirements, and platform support.

**What BLE HID adds:**
- Wireless gamepad output via BLE HID on CYW43-equipped boards
- Lower power consumption (~50% less than BT Classic)
- Target platforms: Windows 10/11 and Android with strong BLE HID support
- Full integration with the OutputManager architecture (same as BT Classic HID)

**What BLE HID is NOT:**
- A replacement for BT Classic HID — it is a separate implementation for power-constrained scenarios
- Supported on Nintendo Switch — Switch exclusively requires BT Classic. BLE gamepads are not recognized.
- A simple modification of Classic — BLE HID requires a completely different BTstack profile, GATT database, and pairing flow

**Why it matters:**
BLE HID enables ultra-low-power wireless gaming on battery-backed boards (Pico W with external LiPo, Pimoroni Pico Lipo 2 XL W). While BT Classic (Phase 1 in `bluetooth-support.md`) targets broad platform support, BLE HID is the efficiency choice for mobile play and longer battery life.

---

## Scope

### In Scope

1. **BLE HID Gamepad Profile** — Minimal GATT database with HID Input Reports, no Boot Keyboard/Mouse characteristics
2. **Pairing and Bonding** — Just Works authentication, TLV flash-backed bonding key storage
3. **Windows 10/11 Support** — Requires Secure Connections in SMP (SM_AUTHREQ_SECURE_CONNECTIONS)
4. **Android Support** — Standard BLE HID pairing flow
5. **Report Delivery** — HID Input Reports over BLE ATT notifications
6. **Battery Reporting** — GATT Battery Service (UUID 0x180F) with percentage characteristic
7. **USB Config Mode Fallback** — Mechanism to still access web configurator when in BLE-only mode
8. **Power Management** — Sleep/dormant integration (same as BT Classic)

### Out of Scope

1. **iOS Support** — BLE HID has known pairing and reconnection issues on iOS. iOS is a stretch goal for Phase 3; Phase 1 does not support it.
2. **Nintendo Switch** — Switch does NOT support BLE HID gamepads. This is a hard constraint. Users requiring Switch must use BT Classic.
3. **PS5/PlayStation** — PS5 requires BT Classic with proprietary extensions. BLE is insufficient.
4. **BLE Mesh or Extended Advertisements** — Broadcast mode only; no Mesh extensions.
5. **HID-over-GATT Simplifications** — Full HID over GATT protocol is implemented; no shortcuts.

---

## Board Requirements

### CYW43-Equipped Boards Only

BLE HID requires:
- **Wireless chip:** CYW43439 (Cypress/Infineon)
- **Microcontroller:** RP2040 or RP2350
- **BTstack support:** Pico SDK 2.2.0 with `pico_btstack_cyw43` linkage

### Supported Boards

| Board | Chip | Status |
|-------|------|--------|
| Raspberry Pi Pico W | RP2040 + CYW43439 | ✅ Target |
| Raspberry Pi Pico 2 W | RP2350A + CYW43439 | ✅ Target |
| Pimoroni Pico Lipo 2 XL W | RP2350B + CYW43439 | ✅ Reference board (battery hardware) |
| All other boards | RP2040 / RP2350 | ❌ USB only |

### Minimum Build Requirements

**For BLE HID support:**
- Pico SDK version: **2.2.0** (enforced in CMakeLists.txt)
- CMake target linkage: `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device`
- BTstack configuration: `btstack_config.h` with `ENABLE_LE_SECURE_CONNECTIONS` defined
- Flash storage: Minimum 256 KB flash for bonding TLV database (RP2040/RP2350 standard)

---

## Architecture

### Integration with OutputManager

BLE HID fits into the existing OutputManager architecture (see `bluetooth-support.md` for full context):

```
┌─────────────────────────────────────────────────┐
│ Main Loop (gp2040.cpp)                          │
│  + GPIO read, button processing, input addons   │
└────────────────┬────────────────────────────────┘
                 │ gamepad state
                 ▼
┌──────────────────────────────────────────────────┐
│ OutputManager                                    │
│  + Selects active output transport               │
│  + Modes: USB / BT Classic HID / BLE HID         │
└────┬──────────────────────────┬──────────────────┘
     │                          │
     ▼                          ▼
┌───────────────────┐  ┌────────────────────┐
│ DriverManager     │  │ BTClassicHIDManager│
│ (USB output)      │  │ (Phase 1)          │
└───────────────────┘  └────────────────────┘
                             │
                             │ (BLE implementation adds)
                             ▼
                       ┌────────────────────┐
                       │ BLEHIDManager      │
                       │ (Phase 2/3)        │
                       └────────────────────┘
```

**Key difference:** Unlike BT Classic HID (which uses L2CAP + HID profiles), BLE HID uses **GATT (Generic Attribute Profile)** for service discovery and report delivery. ATT (Attribute Protocol) handles all communication over the BLE link.

### BTstack BLE HID Stack: Critical Requirements

BLE HID implementation depends on BTstack's `pico_btstack_hid_device` component. The following **must** be in place or the implementation will hard fault:

#### 1. TLV Flash Bonding Database Initialization

Bonding keys must be stored in flash via BTstack's TLV (Tag-Length-Value) format. **This is not optional.** If the TLV context is NULL when BTstack initializes, SMP pairing will fail with a hard fault.

```c
// In main firmware initialization (src/main.cpp or equivalent)
#include "pico/btstack_cyw43.h"
#include "pico/cyw43_arch.h"
#include "btstack_tlv_flash_bank.h"

static btstack_tlv_flash_bank_t tlv_context;

void setupBLE() {
    // Initialize TLV flash bank BEFORE any BLE/SMP init.
    // pico_flash_bank_instance() returns the hal_flash_bank_t* for the Pico flash bank.
    // The third argument is a context pointer — pass NULL for Pico (unused).
    const btstack_tlv_t* tlv_impl = btstack_tlv_flash_bank_init_instance(
        &tlv_context, pico_flash_bank_instance(), NULL);

    // Configure SMP with the TLV context — two-arg form required.
    // le_device_db_tlv_configure(impl, context): first arg is the btstack_tlv_t* interface,
    // second arg is the btstack_tlv_flash_bank_t* context. Both must be non-NULL.
    le_device_db_tlv_configure(tlv_impl, &tlv_context);

    // Only AFTER TLV is configured, call hci_power_control(HCI_POWER_ON)
    hci_power_control(HCI_POWER_ON);
}
```

**Critical:** `le_device_db_tlv_configure(tlv_impl, &tlv_context)` takes **two arguments**: the `btstack_tlv_t*` interface pointer returned by `btstack_tlv_flash_bank_init_instance()`, and the `btstack_tlv_flash_bank_t*` context. Both must be non-NULL. Passing a one-argument form will not compile; passing NULL for either will cause `sm_init()` to skip key storage configuration and subsequent bonding attempts will hard fault.

#### 2. Secure Connections (SC) for Windows 10/11

Windows 10/11 requires the Secure Connections (SC) bit in the SMP Pairing Request. BTstack enables this with:

```c
// In btstack_config.h or equivalent configuration:
#define ENABLE_LE_SECURE_CONNECTIONS 1
```

Without this, Windows will see the device as insecure and may refuse pairing or present warning dialogs. Android does not strictly require SC but benefits from it for security.

**SMP Configuration:**

```c
sm_init();
sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
sm_set_authentication_requirements(SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTIONS);
```

This enables:
- **Bonding:** Keys are saved after initial pairing
- **Just Works:** No PIN entry required (appropriate for a gamepad with no keyboard)
- **SC:** Elliptic Curve (P-256) ECDH + AES-CCM instead of Legacy pairing

#### 3. Advertising Timing: Wait for HCI_STATE_WORKING

Advertising must NOT start until the Bluetooth stack is fully initialized. **This is a common bug.**

```c
static uint8_t adv_started = 0;

// In the BTstack event handler:
case BTSTACK_EVENT_STATE:
    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        // NOW safe to start advertising
        if (!adv_started) {
            gap_advertisements_enable(1);
            adv_started = 1;
        }
    }
    break;
```

**Wrong:** Starting advertising in `setupBLE()` after `hci_power_control(HCI_POWER_ON)` before the HCI_STATE_WORKING event. This causes the advertisement to be queued but never sent, and the device appears invisible to hosts.

#### 4. GATT Service Permissions: No Encryption on Discovery

Characteristics required for initial service discovery must NOT require encryption. This is a chicken-and-egg problem: the host must discover services to initiate pairing, but encrypted characteristics are invisible to unpaired hosts.

**Correct:**
```c
// HID Service characteristics are READABLE without encryption
// Report Map, HID Information, Protocol Mode do not require pairing
ATT_CHARACTERISTIC(hid_information_uuid, ATT_PROPERTY_READ, NULL),
// ^^^^^^ No encryption required for discovery
```

**Wrong:**
```c
ATT_CHARACTERISTIC(hid_information_uuid, ATT_PROPERTY_READ | ATT_PROPERTY_ENCRYPTED, NULL),
// ^^^^^^ This prevents unpaired hosts from discovering the service
```

Pairing can be initiated by the host after discovering the HID service. Once paired, encrypted characteristics (e.g., Report Input notifications) are visible.

#### 5. Omit Boot Keyboard/Mouse Characteristics

BLE HID supports optional "Boot Mode" characteristics (UUIDs 0x2A4C for Boot Keyboard, 0x2A4D for Boot Mouse). A gamepad **must not include these**.

**Why:** The Boot Mode mechanism has special meaning in HID—these characteristics are intended only for Boot Keyboard and Boot Mouse devices. A gamepad without Boot Mode characteristics is valid; one WITH Boot Mode characteristics for a gamepad is confusing and can cause ATT discovery errors when the host looks for Boot Mode handlers that don't exist.

**Correct GATT structure for a gamepad:**
```
HID Service (0x1812)
  ├─ Protocol Mode (0x2A4E)         [Read/Write, no encryption]
  ├─ Input Report (0x2A4D)          [Notify, encryption after pairing]
  │   └─ Report Reference (0x2908)  [Read, Report ID = 0x01]
  ├─ Report Map (0x2A4B)            [Read, no encryption]
  ├─ HID Information (0x2A4A)       [Read, no encryption]
  └─ HID Control Point (0x2A4C)     [Write, no encryption]
```

**Wrong structure (do not use):**
```
HID Service (0x1812)
  ├─ Boot Keyboard Input (0x2A22)   ← NOT for gamepads
  ├─ Boot Mouse Input (0x2A33)      ← NOT for gamepads
  ├─ Input Report (0x2A4D)
  └─ ... (rest of structure)
```

#### 6. Report ID Consistency

HID Report IDs must match across two sources:
1. The GATT Report Reference characteristic (attribute value)
2. The HID Report Descriptor byte sequence

**Example:**
- HID descriptor: `0x85, 0x01` (Report ID = 0x01)
- Report Reference characteristic: descriptor value = 0x01

If these mismatch, the host may misinterpret reports or fail HID validation.

---

## GATT Database Structure

BTstack for Pico uses a `.gatt` DSL file compiled at build time by `pico_btstack_make_gatt_header` — **not C structs**. The compiled output is a `uint8_t profile_data[]` array passed to `att_server_init()`.

**Do NOT** use `gatt_char_t gatt_db[]`, `PRIMARY_SERVICE_UUID16()`, `CHARACTERISTIC_UUID16()`, or `DESCRIPTOR_UUID16()` as C runtime array initializers — these types and macros do not exist in BTstack's C API.

**Do NOT** use `#import <hids.gatt>` from BTstack's built-in service definitions — it adds `ENCRYPTION_KEY_SIZE_16` to all report characteristics, which blocks service discovery before pairing completes.

### GATT DSL File (`src/ble_hid.gatt`)

```
// src/ble_hid.gatt
PRIMARY_SERVICE, GAP_SERVICE
CHARACTERISTIC, GAP_DEVICE_NAME, READ, "GP2040-CE Gamepad"
CHARACTERISTIC, GAP_APPEARANCE, READ, 964

#import <battery_service.gatt>
#import <device_information_service.gatt>

PRIMARY_SERVICE, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_PROTOCOL_MODE, DYNAMIC | READ | WRITE_WITHOUT_RESPONSE,
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_REPORT, DYNAMIC | READ | NOTIFY,
REPORT_REFERENCE, READ, 1, 1
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_REPORT_MAP, DYNAMIC | READ,
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_HID_INFORMATION, READ, 01 01 00 02
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_HID_CONTROL_POINT, DYNAMIC | WRITE_WITHOUT_RESPONSE,

PRIMARY_SERVICE, GATT_SERVICE
CHARACTERISTIC, GATT_DATABASE_HASH, READ,
```

### CMakeLists.txt Integration

Add this to `CMakeLists.txt` to compile the `.gatt` file and generate the GATT header:

```cmake
pico_btstack_make_gatt_header(GP2040-CE PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src/ble_hid.gatt")
```

Then in your initialization code, pass the generated `profile_data` to `att_server_init()`:

```c
#include "ble_hid.h"  // Generated from ble_hid.gatt by pico_btstack_make_gatt_header

// ...
att_server_init(profile_data, att_read_callback, att_write_callback);
```

### GATT Service Layout

- **GAP Service** — Device name ("GP2040-CE Gamepad") and appearance (964 = Gamepad)
- **Battery Service** (imported via `#import <battery_service.gatt>`) — Battery Level characteristic + CCCD for notifications
- **Device Information Service** (imported) — Manufacturer, model, firmware revision
- **HID Service** — Protocol Mode, Input Report (with CCCD + Report Reference), Report Map, HID Information, Control Point
- **GATT Service** — Database Hash for cache invalidation on re-pairing

**Key notes:**
- **CCCD** on the Input Report characteristic allows the host to subscribe to ATT notifications
- **REPORT_REFERENCE** descriptor: first value `1` = Report ID, second value `1` = Report Type (Input)
- **No encryption required** on Report Map, HID Information, Protocol Mode — enables service discovery before pairing
- **Report ID lives in the GATT Report Reference descriptor**, not in the ATT notification payload (see HID Report Descriptor section)

---

## HID Report Descriptor

The HID Report Descriptor is a compact binary structure that describes the gamepad's input layout. It is advertised in the GATT Report Map characteristic.

### Report Descriptor Structure (32 Buttons + Hat Switch + 4 Axes)

```c
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    
    // Report ID
    0x85, 0x01,        // Report ID (0x01)
    
    // Button 1–32 (32 buttons)
    0x05, 0x09,        // Usage Page (Button)
    0x19, 0x01,        // Usage Minimum (Button 1)
    0x29, 0x20,        // Usage Maximum (Button 32)
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x01,        // Logical Maximum (1)
    0x75, 0x01,        // Report Size (1 bit)
    0x95, 0x20,        // Report Count (32 buttons)
    0x81, 0x02,        // Input (Data, Variable, Absolute)
    
    // Hat Switch (D-Pad as 4-bit value: null, N, NE, E, SE, S, SW, W, NW)
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x39,        // Usage (Hat Switch)
    0x15, 0x00,        // Logical Minimum (0)
    0x25, 0x07,        // Logical Maximum (7)
    0x75, 0x04,        // Report Size (4 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x42,        // Input (Data, Variable, Absolute, Null State)
    
    // 4 Padding bits (to align to byte boundary)
    0x75, 0x04,        // Report Size (4 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x01,        // Input (Constant, Array)
    
    // X Axis (Left Stick X)
    0x09, 0x30,        // Usage (X)
    0x15, 0x80,        // Logical Minimum (-128)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data, Variable, Absolute)
    
    // Y Axis (Left Stick Y)
    0x09, 0x31,        // Usage (Y)
    0x15, 0x80,        // Logical Minimum (-128)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data, Variable, Absolute)
    
    // Z Axis (Right Stick X)
    0x09, 0x33,        // Usage (Rx)
    0x15, 0x80,        // Logical Minimum (-128)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data, Variable, Absolute)
    
    // Rz Axis (Right Stick Y)
    0x09, 0x34,        // Usage (Ry)
    0x15, 0x80,        // Logical Minimum (-128)
    0x25, 0x7F,        // Logical Maximum (127)
    0x75, 0x08,        // Report Size (8 bits)
    0x95, 0x01,        // Report Count (1)
    0x81, 0x02,        // Input (Data, Variable, Absolute)
    
    // End Collection
    0xC0,              // End Collection
};

// HID Input Report structure (matches the descriptor above)
//
// IMPORTANT: BLE HID ATT notifications do NOT carry a Report ID byte in-payload.
// The Report ID (0x01) lives in the GATT Report Reference descriptor (0x2908), not
// in the data bytes. Unlike USB HID where report_id IS a prefix byte in the packet,
// hids_device_send_input_report() sends the raw payload only — no Report ID prefix.
struct BLEHIDReport {
    uint32_t buttons;                           // Bits 0–31 for buttons 1–32      (4 bytes)
    uint8_t hat : 4;                            // Hat switch (0–7, with null state)(1 byte combined)
    uint8_t pad : 4;                            // Padding to byte boundary
    int8_t x, y;                                // Left analog stick               (2 bytes)
    int8_t rx, ry;                              // Right analog stick              (2 bytes)
} __attribute__((packed));                      // Total: 4 + 1 + 2 + 2 = 9 bytes
static_assert(sizeof(BLEHIDReport) == 9, "Report size must be 9 bytes");
```

### Report Flow

At each main loop iteration:

```c
BLEHIDReport report = {
    .buttons = gamepad->buttons,
    .hat = gamepad->dpad_as_hat(),
    .x = gamepad->lx,
    .y = gamepad->ly,
    .rx = gamepad->rx,
    .ry = gamepad->ry,
};

// Send via BLE HID
ble_hid_manager.sendReport(&report, sizeof(report));
```

The BLEHIDManager queues the report for transmission over the Input Report characteristic's notification channel.

---

## Pairing and Bonding

### SM (Security Manager) Configuration

BTstack's SM (Bluetooth Low Energy Security Manager) handles pairing. Configuration:

```c
void setupSM() {
    // Initialize SM with Just Works (no PIN)
    sm_init();
    
    // No keyboard/display on gamepad → No Input/Output capability
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    
    // Enable bonding + Secure Connections
    sm_set_authentication_requirements(
        SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTIONS
    );
}
```

### Bonding Key Persistence

BTstack stores bonding keys in a TLV database file on flash. The default location is `.pico-sdk/btstack_priv.tlv`.

**Bonding record includes:**
- Peer device address (BD_ADDR)
- Long-Term Key (LTK)
- Identity Resolving Key (IRK)
- Connection Signature Resolving Key (CSRK)
- Peer EDIV/Rand (for legacy pairing, if needed)
- Flags (bonded, authenticated, authorized)

**Manual bonding management:**

```c
// Clear all bonding keys (e.g., reset button combo)
void clearBondingKeys() {
    le_device_db_delete_all();  // Clears in-memory bonding list
    // TLV file will be recreated empty on next pairing
}

// Query bonded devices
void listBondedDevices() {
    int num_bonded = le_device_db_count();
    for (int i = 0; i < num_bonded; i++) {
        bd_addr_t addr;
        le_device_db_from_index(i, addr);
        printf("Bonded device %d: %s\n", i, bd_addr_to_str(addr));
    }
}
```

### Pairing Flow (Host-Initiated)

1. **Discovery:** Host scans and discovers the gamepad's BLE advertisement
2. **Connection:** Host initiates a BLE connection
3. **Pairing Request:** Host sends `Pairing Request` (SMP opcode 0x01) with SC bit set
4. **Pairing Response:** Device responds with matching SC bit + ECDH public key
5. **Confirm/Random:** Public key verification via ECDH (P-256) + AES-CCM confirmation
6. **LTK Distribution:** Both sides derive and exchange Long-Term Keys
7. **Bonding:** Keys are stored in TLV (automatic via BTstack)
8. **Connection Close:** Either side may close, but bonding persists
9. **Reconnection:** Future connections use the stored LTK; no pairing needed

### Just Works Authentication

With `IO_CAPABILITY_NO_INPUT_NO_OUTPUT`, the pairing mechanism defaults to "Just Works" — no PIN confirmation or user interaction required. This is appropriate for a gamepad with no input methods.

**Pairing is initiated by the host**, not the device. When a user manually pairs from their phone/PC settings, that OS sends the pairing request. The gamepad simply accepts it.

---

## USB Config Mode Fallback

When a device is in BLE HID mode, TinyUSB's HID output is disabled (to avoid namespace collision between TinyUSB and BTstack headers, and to conserve flash). However, users must still be able to access the web configurator.

### Proposed Solution: S2 Button Hold at Boot

**Configuration mode entry:**
1. Power on or reset while holding the **S2 button** (typically a config button)
2. Device enters **USB Config Mode**:
   - Advertise via TinyUSB as a USB HID device (currently implemented)
   - Start the web configurator over RNDIS/Ethernet at `192.168.7.1`
   - BLE advertising is disabled (to avoid resource conflicts)
3. User opens web browser, configures, and saves
4. After exit (button release or reboot), device returns to last-saved output mode (USB or BLE)

### Implementation

```c
// In src/main.cpp, during boot:

bool config_mode = gpio_get(GPIO_S2);  // Read S2 button state during early init

if (config_mode) {
    // Enter USB-only config mode
    driverManager.initialize();          // Initialize USB HID
    // Skip BLE initialization
} else {
    // Normal operation: check saved output mode from flash
    if (gamepadOptions.output_mode == OUTPUT_MODE_BLE) {
        bleHidManager.initialize();
    } else {
        driverManager.initialize();
    }
}
```

**Web Configurator Storage:**
The web config still persists to flash and survives across reboots. Even in BLE mode, users can:
1. Hold S2 at boot → USB Config Mode
2. Update settings in web UI
3. Reboot → Resumes BLE mode with updated settings

### Alternative: USB+BLE Coexistence (Future)

In a later phase, if BTstack memory constraints allow, USB HID and BLE HID could run simultaneously, with the web config always available over USB. This requires careful TinyUSB/BTstack integration and is not Phase 1.

---

## Battery Reporting (GATT Battery Service)

Battery level is reported via the **GATT Battery Service** (UUID 0x180F), not an HID Feature report (as used in BT Classic).

### Battery Service Setup

```c
#include "ble/battery_service_server.h"

void setupBatteryService() {
    // No separate init() call is needed — the Battery Service is initialized via
    // att_server_init() + the GATT database (via #import <battery_service.gatt>).
    // Set the initial battery level directly with battery_service_server_set_battery_value().
    battery_service_server_set_battery_value(100);  // Start at 100% until ADC reads
}

void updateBatteryLevel() {
    uint8_t level = readBatteryPercent();  // 0–100
    battery_service_server_set_battery_value(level);
}
```

### ADC Voltage Measurement

See `bluetooth-support.md` § Battery Level Reporting for details on ADC/voltage divider setup. Same code applies:

```c
uint8_t readBatteryPercent() {
    adc_select_input(BATTERY_ADC_CHANNEL);        // board-specific ADC input (GP43 = ADC 3 on Pico LiPo 2 XL W)
    uint16_t raw = adc_read();
    float v_adc = (raw / 4095.0f) * 3.3f;
    float v_bat = v_adc * 3.0f;                   // voltage divider ratio
    float pct = (v_bat - 3.0f) / (4.2f - 3.0f) * 100.0f;  // 3.0V = 0%, 4.2V = 100%
    return (uint8_t)std::clamp(pct, 0.0f, 100.0f);
}
```

### VBUS Detection and USB Power

When USB power is present:

```c
bool usb_connected = cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);

if (usb_connected) {
    battery_service_server_set_battery_value(100);  // Always 100% when charging
} else {
    uint8_t level = readBatteryPercent();
    battery_service_server_set_battery_value(level);
}
```

### Polling Interval

Update the battery level periodically (every 30–60 seconds):

```c
constexpr uint32_t BATTERY_UPDATE_MS = 30000;
static uint64_t last_battery_update = 0;

if (time_us_64() - last_battery_update > BATTERY_UPDATE_MS * 1000) {
    updateBatteryLevel();
    last_battery_update = time_us_64();
}
```

The GATT Battery Service sends notifications only when the host subscribes (via CCCD). No unsolicited updates are sent.

---

## Known Pitfalls & Lessons Learned

Implementation attempt 1 encountered several critical issues. This checklist consolidates those lessons for Phase 2 implementers:

### Critical API Checklist (Blocking Issues)

Before starting implementation, verify all 6 of these are correct in your code:

- [ ] **SDK version is exactly 2.2.0** — not "2.2.0+" or "2.2.0 or later". CMakeLists.txt enforces `set(sdkVersion 2.2.0)`.
- [ ] **`le_device_db_tlv_configure` uses the two-arg form** — `le_device_db_tlv_configure(tlv_impl, &tlv_context)`. The one-arg form does not exist in BTstack's API.
- [ ] **`btstack_tlv_flash_bank_init_instance` takes no file path** — correct call is `btstack_tlv_flash_bank_init_instance(&tlv_context, pico_flash_bank_instance(), NULL)`. No string path, no `FLASH_SECTOR_SIZE`.
- [ ] **`BLEHIDReport` struct has no `report_id` field** — BLE HID ATT notifications carry raw payload only. The Report ID is in the GATT Report Reference descriptor, not in the packet bytes. Struct must be `__attribute__((packed))` and 9 bytes exactly.
- [ ] **GATT database is a `.gatt` DSL file**, not C structs — use `pico_btstack_make_gatt_header` to compile it. `gatt_char_t`, `PRIMARY_SERVICE_UUID16()`, `CHARACTERISTIC_UUID16()` do not exist in BTstack's C API.
- [ ] **Battery service uses `battery_service_server_set_battery_value(uint8_t)`** — no separate `init()` call needed. `battery_service_server_init(NULL)` is a wrong signature; passing NULL will be treated as 0% battery.

---

### 1. TLV Database Must Be Initialized Before SM

**Symptom:** Hard fault in `sm_init()` or `le_device_db_tlv_configure()`  
**Cause:** Wrong API call — one-arg form used, file path passed, or not called at all  
**Fix:**
```c
static btstack_tlv_flash_bank_t tlv_context;
const btstack_tlv_t* tlv_impl = btstack_tlv_flash_bank_init_instance(
    &tlv_context, pico_flash_bank_instance(), NULL);
le_device_db_tlv_configure(tlv_impl, &tlv_context);  // Two-arg form: impl + context
sm_init();  // Only AFTER TLV is set up
```

### 2. Advertising Must Start After HCI_STATE_WORKING Event

**Symptom:** Device is invisible to host scans; no advertisement frames sent  
**Cause:** `gap_advertisements_enable()` called before `BTSTACK_EVENT_STATE` with `HCI_STATE_WORKING`  
**Fix:**
```c
case BTSTACK_EVENT_STATE:
    if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
        gap_advertisements_enable(1);  // Only here
    }
    break;
```

### 3. Secure Connections (SC) Not Enabled → Windows Rejection

**Symptom:** Windows 10/11 shows "Pairing Failed" or "Unknown Device"  
**Cause:** `ENABLE_LE_SECURE_CONNECTIONS` not defined in btstack_config.h, or SMP auth requirements don't include `SM_AUTHREQ_SECURE_CONNECTIONS`  
**Fix:**
```c
// btstack_config.h
#define ENABLE_LE_SECURE_CONNECTIONS 1

// setupSM():
sm_set_authentication_requirements(SM_AUTHREQ_BONDING | SM_AUTHREQ_SECURE_CONNECTIONS);
```

### 4. Encryption Required on Discovery Characteristics → ATT Errors

**Symptom:** Host sees empty HID Service or errors during discovery  
**Cause:** Report Map, HID Information, or Protocol Mode characteristics require encryption  
**Fix:** In the `.gatt` DSL file, do **not** add `ENCRYPTION_KEY_SIZE_*` to discovery characteristics:
```
// Correct — no encryption keyword on discovery characteristics:
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_REPORT_MAP, DYNAMIC | READ,
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_HID_INFORMATION, READ, 01 01 00 02
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_PROTOCOL_MODE, DYNAMIC | READ | WRITE_WITHOUT_RESPONSE,
```
This is also why `#import <hids.gatt>` must not be used — BTstack's built-in HIDS adds `ENCRYPTION_KEY_SIZE_16` to all characteristics.

### 5. Boot Keyboard/Mouse Characteristics Cause Discovery Failures

**Symptom:** HID discovery fails; host reports "unknown HID device type"  
**Cause:** GATT includes Boot Keyboard or Boot Mouse characteristics for a gamepad  
**Fix:** Do not add these to the `.gatt` file:
```
// Do NOT include these for a gamepad:
// CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_BOOT_KEYBOARD_INPUT_REPORT, ...
// CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_BOOT_MOUSE_INPUT_REPORT, ...
```

### 6. Report ID Mismatch Between Descriptor and GATT Report Reference

**Symptom:** Host receives reports but displays incorrectly (button presses don't map)  
**Cause:** HID descriptor has `0x85, 0x01` (Report ID 0x01) but GATT Report Reference has value 0x02  
**Fix:**
```c
// HID descriptor:
0x85, 0x01,  // Report ID = 0x01

// GATT Report Reference descriptor (attribute 0x0024):
// Attribute value: { 0x01, 0x00 }  // Report ID 0x01, Report Type 0x00 (Input)
```

### 7. TinyUSB and BTstack Header Namespace Collision

**Symptom:** Compilation error: `hid_report_type_t` ambiguous or redefined  
**Cause:** Both TinyUSB and BTstack headers define `hid_report_type_t` with conflicting values  
**Fix:**
- In BLE HID builds, **do not link TinyUSB HID code**
- Use conditional compilation:
  ```cpp
  #if !defined(ENABLE_BLE_HID) || !ENABLE_BLE_HID
      // USB HID code
  #endif
  ```
- OR isolate includes:
  ```cpp
  // btstack_config.h
  #define HAVE_HID_REPORT_TYPE_T 1  // Tell BTstack we define it
  ```

### 8. CCCD Not Subscribed → No Notifications Sent

**Symptom:** Device sends Input Report data, but host never receives it  
**Cause:** Host never enabled the Client Characteristic Config Descriptor (CCCD) for the Input Report  
**Fix:**
- Do not assume subscription. Check CCCD state before sending:
  ```c
  if (att_server_client_is_subscribed(connection_handle, INPUT_REPORT_HANDLE, ATT_CLIENT_CHARACTERISTIC_CONFIGURATION)) {
      att_server_notify(connection_handle, INPUT_REPORT_HANDLE, (uint8_t*)&report, sizeof(report));
  }
  ```
- Host should subscribe automatically after pairing, but defensive code is safer.

### 9. Connection Drops After Bonding

**Symptom:** Device pairs successfully, but connection drops immediately after bonding  
**Cause:** BLE connection parameters (latency, timeout) too conservative, or link supervision timeout too short  
**Fix:**
```c
// After bonding, request connection parameter update:
gap_request_connection_parameter_update(connection_handle, 
    MIN_CONN_INTERVAL,      // 6 (7.5 ms)
    MAX_CONN_INTERVAL,      // 40 (50 ms)
    SLAVE_LATENCY,          // 0
    SUPERVISION_TIMEOUT);   // 100 (1 second)
```

### 10. Android Reconnection Issues (Stretch Goal for iOS)

**Symptom:** Android shows device as paired but won't connect on subsequent boots  
**Cause:** Bonding keys stored, but device doesn't re-advertise with bonding flags  
**Fix:**
```c
// After power-on, check if any bonds exist:
if (le_device_db_count() > 0) {
    // Set ADV flags to indicate bonded device (will reconnect faster)
    gap_set_connection_parameters(...);
}
```
iOS has additional quirks (requires specific GATT database ordering, privacy, etc.); defer to Phase 3.

---

## Implementation Phases

### Phase 1: Core BLE HID (Estimated 10–15 days)

**Goals:**
1. Basic BLE advertisement and pairing
2. GATT database with HID Service
3. Input Report transmission over BLE
4. Bonding key persistence (TLV flash)
5. Battery Service integration
6. USB Config Mode fallback (S2 button hold)
7. Test on Windows 10/11 and Android

**Deliverables:**
- `src/output/BLEHIDManager.cpp` + `headers/output/BLEHIDManager.h`
- `src/addons/BLEAdvertisement.cpp` (GATT database)
- Updated `proto/config.proto` with BLE output mode
- New board config for Pimoroni Pico Lipo 2 XL W (if needed)
- Functional tests on Windows + Android

**Dependencies:**
- OutputManager architecture (from BT Classic Phase 1)
- TLV flash bonding infrastructure
- Power management framework (from BT Classic)

**Not included:** iOS support, runtime BLE parameter tuning, advanced pairing modes

### Phase 2: Polish & Documentation (Estimated 5–7 days)

**Goals:**
1. Comprehensive troubleshooting guide
2. API reference for BLEHIDManager
3. Configuration examples (web UI)
4. Known issues document
5. Performance characterization (latency, power consumption)

**Deliverables:**
- `docs/development/ble-hid-troubleshooting.md`
- Updated `docs/getting-started.md` with BLE section
- Changelog entry for release notes
- Inline API documentation (Doxygen-ready)

### Phase 3: iOS Support & Advanced Features (Estimated 8–10 days)

**Goals:**
1. iOS-specific GATT ordering and privacy mode
2. Enhanced reconnection for iOS devices
3. Connection parameter optimization per host OS
4. Advanced power management (sniff mode, dormant integration)

**Deliverables:**
- GATT database variant for iOS
- Connection parameter profiles (Windows, Android, iOS)
- Power state transitions (IDLE → DEEP_SLEEP) with BLE reconnect

**Note:** iOS has unique pairing and reconnection challenges. Phase 1 explicitly does not target iOS.

---

## Comparison: BLE HID vs. BT Classic HID

| Aspect | BLE HID | BT Classic HID |
|--------|---------|---|
| **Power consumption** | ~50% less than Classic | Higher baseline |
| **Latency** | 10–20 ms typical | 5–10 ms typical |
| **Windows support** | ✅ 10/11 (with SC) | ✅ All versions |
| **Android support** | ✅ 5.0+ | ✅ 4.0+ |
| **Nintendo Switch** | ❌ Not supported | ✅ Supported |
| **PS4/PS5** | ❌ Requires BT Classic | ✅ PS4 Classic; PS5 proprietary |
| **Profile complexity** | GATT HID over ATT | L2CAP + HID profile |
| **Pairing flow** | Just Works (no PIN) | SDP discovery + PIN/SSP |
| **Bonding keys** | TLV flash database | TLV flash database |
| **Implementation effort** | ~10–15 days (Phase 1) | ~15–20 days (est. Phase 1) |

**Recommendation:** 
- Choose **BLE HID** for mobile-first, battery-constrained scenarios
- Choose **BT Classic HID** for broad platform support (especially Switch/PS5 users)
- Implement both and allow user selection via OutputManager

---

## Reference Documentation

- **BTstack BLE HID Documentation:** https://bluekitchen-gmbh.com/btstack/manual/docs_profile_hid.html
- **Bluetooth Core Specification v5.3:** https://www.bluetooth.com/specifications/specs/core-specification/
- **HID Usage Tables:** https://usb.org/document-library/hid-usage-tables-13
- **Related docs:** `docs/development/bluetooth-support.md`, `docs/development/rp2350-support.md`

---

**End of document**
