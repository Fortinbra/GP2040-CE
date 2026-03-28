# BT Battery Protocol Analysis: Classic HID vs BLE HID

**Author:** Edward (Firmware Developer)  
**Date:** 2026-03-28  
**Status:** Research complete — grounded in BTStack SDK 2.2.0 source  
**Context:** Resolves tension between BT Classic HID transport (chosen in bluetooth-support.md) and GATT Battery Service (UUID 0x180F) documentation in the same file

---

## 1. Confirmed Mechanism: BT Classic HID Battery Reporting

### Spec path: HID descriptor Feature report

BT Classic HID (BR/EDR L2CAP + HIDP) reports battery level via a **Feature report** in the **HID report descriptor itself**. This is dictated by the HID Usage Tables specification (HUT1_22.pdf §4.5.1):

- **Usage Page:** `0x06` — Generic Device Controls  
- **Usage:** `0x20` — Battery Strength  
- **Report type:** `Feature` (tag `0xB1`)  
- **Logical range:** 0–100 (percentage)  
- **Report size:** 8 bits

**HID descriptor snippet (to be embedded in the gamepad descriptor):**

```c
// Battery Strength — Feature report (BT Classic HID)
0x05, 0x06,        // Usage Page (Generic Device Controls)
0x09, 0x20,        // Usage (Battery Strength)
0x15, 0x00,        // Logical Minimum (0)
0x26, 0x64, 0x00,  // Logical Maximum (100)
0x75, 0x08,        // Report Size (8 bits)
0x95, 0x01,        // Report Count (1)
0xB1, 0x02,        // Feature (Data, Variable, Absolute)
```

This block is added inside the top-level `Application` collection of the gamepad descriptor.

### How the host reads it

The host OS sends a `GET_REPORT(Feature, battery_report_id)` request over the **HID control channel (L2CAP)**. The device responds with the current battery percentage. There is **no background notification** mechanism for Classic HID battery — the host polls when it needs the value (typically on connect and then periodically, e.g., every 30–120 s).

**BTStack API surface for handling this:**

```c
// Register callback for GET_REPORT(Feature) requests from host
hid_device_register_report_request_callback(report_request_cb);

// In the callback, send feature report data:
void report_request_cb(uint16_t hid_cid,
                       hid_report_type_t report_type,
                       uint16_t report_id,
                       int * out_report_size,
                       uint8_t * out_report) {
    if (report_type == HID_REPORT_TYPE_FEATURE
        && report_id == BATTERY_REPORT_ID) {
        *out_report      = current_battery_percent;  // 0–100
        *out_report_size = 1;
    }
}
```

Key API functions from `classic/hid_device.h` (Pico SDK 2.2.0):

| Function | Role |
|---|---|
| `hid_device_init(boot, len, descriptor)` | Init HID Classic device; passes full descriptor |
| `hid_create_sdp_record(buf, handle, &params)` | Build SDP record — includes descriptor |
| `hid_device_register_report_request_callback(cb)` | Handle GET_REPORT from host |
| `hid_device_register_set_report_callback(cb)` | Handle SET_REPORT from host |
| `hid_device_send_interrupt_message(cid, msg, len)` | Send Input report (button state) |
| `hid_device_send_control_message(cid, msg, len)` | Send Feature/Output response on control channel |

The full HID descriptor — including the battery Feature report — is passed to both `hid_device_init()` and the `hid_sdp_record_t` struct in `hid_create_sdp_record()`. The SDP record advertises the descriptor to the host so it knows a battery Feature report is available.

### BTStack usage page constant confirmation

`btstack_hid.h` (Pico SDK 2.2.0, `lib/btstack/src/btstack_hid.h`) defines:

```c
#define HID_USAGE_PAGE_GENERIC_DEVICE  0x06   // confirmed present
```

The Battery Strength usage `0x20` is from the HID Usage Tables specification. BTStack does not define a named constant for it (no `HID_USAGE_BATTERY_STRENGTH`), but `0x20` is the unambiguous spec value. Use the raw byte in the descriptor.

---

## 2. Why `battery_service_server_set_battery_value()` Does NOT Apply to Classic HID

### It is a BLE GATT API

`battery_service_server_set_battery_value()` lives in:

```
${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.h
```

The path `/ble/gatt-service/` is the definitive tell. This function:

1. Requires `att_server_init()` (ATT = Attribute Protocol, BLE-only layer)
2. Requires `sm_init()` (Security Manager, BLE pairing)
3. Requires a GATT profile file importing `<battery_service.gatt>`
4. Manages UUID **0x180F** (GATT Battery Service) with characteristic UUID **0x2A19** (Battery Level)
5. Pushes GATT **Notifications** over an active BLE connection handle (`con_handle`) — not over L2CAP HID channels

**None of these exist in a BT Classic HID connection.** A Classic HID connection uses L2CAP channels (PSM 0x0011 for HID Control, 0x0013 for HID Interrupt). There is no ATT server, no GATT, no UUID-based service discovery.

### SDK example evidence

The BTStack SDK includes two HID demo examples that make the split explicit:

| File | Transport | Battery mechanism |
|---|---|---|
| `example/hid_keyboard_demo.c` | **BT Classic HID** | No battery service. No `battery_service_server.h`. Battery in descriptor if needed. |
| `example/hog_keyboard_demo.c` | **BLE HID (HOG)** | Calls `battery_service_server_init(battery)` in setup. Includes `ble/gatt-service/battery_service_server.h`. |

`hog_keyboard_demo.c` also calls:
- `att_server_init(profile_data, NULL, NULL)` — ATT server
- `sm_init()` — BLE Security Manager
- `hids_device_init()` — GATT HID Service (UUID 0x1812) — NOT `hid_device_init()`

The Classic demo (`hid_keyboard_demo.c`) calls:
- `hid_device_init()` — Classic HIDP
- `hid_create_sdp_record()` — SDP registration
- `hid_device_send_interrupt_message()` — sends Input reports via L2CAP

No BLE stack calls. No `battery_service_server_*` calls. No `att_server_init()`. No GATT.

### Calling `battery_service_server_set_battery_value()` in a Classic HID build

If a developer calls `battery_service_server_set_battery_value()` while connected via BT Classic HID:
- It is dead code — the ATT server has no active BLE connection to notify
- It wastes ~2 KB flash (battery GATT service code + ATT server)
- It does NOT update the host's battery display because the Classic host reads battery via `GET_REPORT(Feature)` over L2CAP, not via GATT notifications

---

## 3. Recommended Approach for GP2040-CE Phase 1

### Option A: BT Classic HID + HID descriptor battery (RECOMMENDED)

**Consistent with the doc's stated goal.** `bluetooth-support.md` explicitly chose BT Classic for "broadest platform support (Switch, PS5, Android, PC)" and "zero fragmentation in the gaming ecosystem." This decision is correct and should not change.

Battery reporting for Classic HID:
1. Add Battery Strength Feature item to the gamepad HID descriptor (6 bytes, see §1 above)
2. Assign a distinct Report ID (e.g., `0x02`) to the battery Feature report
3. Register `hid_device_register_report_request_callback()` — respond to `GET_REPORT(Feature, 0x02)`
4. In the callback, return `readBatteryPercent()` (the ADC read logic is unchanged)
5. On connect, the host discovers the battery Feature report via SDP and starts polling

**Platform compatibility:**
- **Windows 10/11:** Reads BT Classic battery via HID Feature report. Shows in Settings → Bluetooth & devices. ✓
- **macOS:** Reads via HID Feature report. Shows in menu bar battery indicator. ✓
- **Nintendo Switch:** Does not show a controller battery UI for non-proprietary controllers, but the Feature report is harmless. ✓
- **Android:** OS-level display varies. Some OEMs show it; some don't. The data is available via HID. ✓

### Option B: BLE HID + GATT Battery Service (NOT recommended for Phase 1)

Uses `hids_device_init()` + `battery_service_server_init()` + ATT server. Better battery UX on mobile but BLE HID has **worse gamepad compatibility**:
- Nintendo Switch does NOT support BLE gamepads
- Many gaming-focused BT Classic hosts reject BLE HID gamepads
- This directly contradicts the doc's stated rationale for Classic HID

Correct choice for a mobile-only optimization in a future phase.

### Option C: Dual-mode (Classic + BLE simultaneously) — Overkill for Phase 1

Running BT Classic HIDP and BLE GATT simultaneously requires:
- Both `pico_btstack_classic` and `pico_btstack_ble` linked
- ~8–12 KB additional RAM for BLE stack state
- Advertising two separate connection handles
- Syncing reports to both active connections

Not appropriate for Phase 1. If needed, an explicit Phase 2 decision is required.

---

## 4. Required Changes in `bluetooth-support.md`

### Section: Battery Level Reporting (replace entire section)

**Current state (incorrect):** Documents `battery_service_server_init()` and `battery_service_server_set_battery_value()` as the battery reporting mechanism — these are BLE GATT APIs.

**Required replacement:**

1. **Remove:** "Bluetooth Battery Service" subsection documenting UUID 0x180F, GATT characteristic 0x2A19, `battery_service_server_init()`, `battery_service_server_set_battery_value()`, GATT notifications, "Only call the setter if percentage changed by ≥1% to avoid unnecessary GATT notifications."

2. **Replace with:** "HID Descriptor Battery Strength Feature Report" covering:
   - BT Classic battery = HID descriptor Feature report (Usage Page 0x06, Usage 0x20)
   - The 6-byte descriptor addition
   - `hid_device_register_report_request_callback()` for responding to `GET_REPORT(Feature)`
   - ADC read in the callback (same logic, unchanged)
   - Polling note: host initiates reads; no notification flooding concern

3. **Add a forward note:** "If BLE HID mode is added in a future phase (BLE + HID over GATT), the GATT Battery Service (UUID 0x180F) with `battery_service_server_set_battery_value()` would then be the correct mechanism — it is specifically a BLE GATT construct."

4. **ADC section (unchanged):** The ADC reading logic (GPIO29/ADC3, `adc_select_input(3)`, 12-bit, 3.3V reference, 3:1 voltage divider, LiPo 3.0V–4.2V range, linear percentage) is **transport-agnostic**. Keep it exactly as documented. The battery *measurement* is correct; only the battery *reporting mechanism* (how the host learns the value) changes.

5. **VBUS/charging detection (unchanged):** GPIO24 HIGH = USB charging = report 100%. The `GamepadAuxPower` struct integration is correct regardless of transport. Only the call site changes: instead of `battery_service_server_set_battery_value(100)`, the value is returned from the `GET_REPORT(Feature)` callback.

6. **Polling interval note (update):** The "30 second polling to avoid flooding GATT notifications" rationale no longer applies. The host decides when to poll `GET_REPORT(Feature)` — the device doesn't push. The 30-second re-read of ADC for internal state is still reasonable.

### Section: Minimum Requirements (minor update)

Replace `pico_btstack_hid_device` (not a real target) with the actual CMake targets:
- `pico_btstack_classic` (for BT Classic HID)
- `pico_btstack_base`
- `pico_btstack_run_loop_async_context`

Do NOT include `pico_btstack_ble` or `pico_btstack_flash_bank` unless BLE is explicitly added.

---

## 5. BTStack API Surface for HID Descriptor Battery Inclusion

### Complete flow for BT Classic HID with battery

```c
// 1. Gamepad HID descriptor — add battery feature report block
static const uint8_t bt_hid_descriptor[] = {
    // Application collection header
    0x05, 0x01,        // Usage Page (Generic Desktop)
    0x09, 0x05,        // Usage (Gamepad)
    0xA1, 0x01,        // Collection (Application)

    // === Input Report: buttons + axes (Report ID 0x01) ===
    0x85, 0x01,        //   Report ID (1)
    // ... button bits, hat switch, analog axes ...

    // === Feature Report: Battery Strength (Report ID 0x02) ===
    0x85, 0x02,        //   Report ID (2)
    0x05, 0x06,        //   Usage Page (Generic Device Controls)
    0x09, 0x20,        //   Usage (Battery Strength)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0x64, 0x00,  //   Logical Maximum (100)
    0x75, 0x08,        //   Report Size (8 bits)
    0x95, 0x01,        //   Report Count (1)
    0xB1, 0x02,        //   Feature (Data, Variable, Absolute)

    0xC0               // End Collection
};

// 2. SDP record registration (includes descriptor for host discovery)
static uint8_t hid_service_buffer[400];
hid_sdp_record_t hid_params = {
    .hid_device_subclass      = 0x2508,          // Gamepad CoD
    .hid_country_code         = 0x00,
    .hid_virtual_cable        = 0,
    .hid_remote_wake          = 1,
    .hid_reconnect_initiate   = 1,
    .hid_normally_connectable = true,
    .hid_boot_device          = false,
    .hid_ssr_host_max_latency = 1600,            // sniff latency
    .hid_ssr_host_min_timeout = 3200,
    .hid_supervision_timeout  = 3200,
    .hid_descriptor           = bt_hid_descriptor,
    .hid_descriptor_size      = sizeof(bt_hid_descriptor),
    .device_name              = "GP2040-CE Gamepad"
};
hid_create_sdp_record(hid_service_buffer,
                      sdp_create_service_record_handle(),
                      &hid_params);
sdp_register_service(hid_service_buffer);

// 3. HID device init — same descriptor
hid_device_init(false, sizeof(bt_hid_descriptor), bt_hid_descriptor);

// 4. Feature report callback — battery query from host
static int report_request_cb(uint16_t hid_cid,
                              hid_report_type_t report_type,
                              uint16_t report_id,
                              int * out_size,
                              uint8_t * out_report) {
    if (report_type == HID_REPORT_TYPE_FEATURE
        && report_id == 0x02) {
        bool usb = gpio_get(24);
        *out_report = usb ? 100 : readBatteryPercent();
        *out_size   = 1;
        return 0;  // success
    }
    return -1;  // not handled
}
hid_device_register_report_request_callback(report_request_cb);
```

### Contrast: BLE HID battery path (NOT for Phase 1)

```c
// BLE HID only — do NOT use for Classic HID
#include "ble/gatt-service/battery_service_server.h"

// Requires ATT server + BLE connection active:
att_server_init(profile_data, NULL, NULL);   // BLE ATT layer
sm_init();                                    // BLE Security Manager
battery_service_server_init(100);             // BLE GATT Battery Service
battery_service_server_set_battery_value(87); // Notifies BLE host
// None of the above has any effect over a Classic HID L2CAP connection
```

---

## 6. Summary Table

| Aspect | BT Classic HID (Phase 1 — correct) | BLE HID (future / incorrect for Phase 1) |
|---|---|---|
| **Transport** | BR/EDR L2CAP + HIDP | BLE ATT + GATT |
| **BTStack API** | `hid_device.h`, `hid_create_sdp_record()` | `hids_device.h`, `att_server_init()` |
| **Battery mechanism** | HID descriptor Feature report (0x06/0x20) | GATT Battery Service UUID 0x180F |
| **Battery API** | `hid_device_register_report_request_callback()` | `battery_service_server_set_battery_value()` |
| **Host reads battery** | `GET_REPORT(Feature)` over L2CAP control channel | GATT Read/Notify over BLE connection |
| **ADC read logic** | Same (GPIO29, adc_read, voltage formula) | Same (unchanged) |
| **Platform compatibility** | Switch ✓, PS5 ✓, Windows ✓, Android ✓ | Mobile ✓, PC ✓, Switch ✗ |
| **CMake target** | `pico_btstack_classic` + `pico_btstack_base` | `pico_btstack_ble` + `pico_btstack_base` |
| **`battery_service_server_*` calls** | Dead code / incorrect | Correct |
| **UUID 0x180F relevance** | None | Central |

---

## 7. Files Referenced

| File | Role |
|---|---|
| `${PICO_SDK_PATH}/lib/btstack/src/classic/hid_device.h` | BT Classic HID device API — `hid_device_init`, `hid_create_sdp_record`, `hid_sdp_record_t`, `hid_device_register_report_request_callback` |
| `${PICO_SDK_PATH}/lib/btstack/src/btstack_hid.h` | Usage page constants: `HID_USAGE_PAGE_GENERIC_DEVICE = 0x06`, `hid_report_type_t`, `HID_REPORT_TYPE_FEATURE` |
| `${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.h` | BLE GATT Battery Service API — `battery_service_server_init`, `battery_service_server_set_battery_value`. NOT applicable to Classic HID. |
| `${PICO_SDK_PATH}/lib/btstack/example/hid_keyboard_demo.c` | BT Classic HID example — no battery service, uses `hid_device_init()` + `hid_create_sdp_record()` |
| `${PICO_SDK_PATH}/lib/btstack/example/hog_keyboard_demo.c` | BLE HID (HOG) example — uses `battery_service_server_init()` + `att_server_init()` + `hids_device_init()` |
| `docs/development/bluetooth-support.md` | Current doc — Battery section must be corrected from GATT path to HID descriptor path |
| `headers/gamepad/GamepadAuxState.h:122–127` | `GamepadAuxPower` struct — `charging`, `pluggedIn`, `level` (unchanged, transport-agnostic) |
| `src/addons/analog.cpp:11–14` | ADC pattern reference (unchanged) |
