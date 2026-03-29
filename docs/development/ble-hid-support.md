# BLE HID Support — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning  
**SDK version:** 2.2.0+  
**Depends on:** RP2350/RP2040 boards with CYW43439 wireless chip (Pico W, Pico 2 W, Pimoroni Pico Lipo 2 XL W)

---

## Overview

This document describes planned **Bluetooth Low Energy (BLE) HID** support for GP2040-CE as an alternative wireless output mode to Bluetooth Classic HID.

BLE HID uses the **HID Over GATT Profile (HOGP)** — a standardized BLE profile where the controller advertises itself as an HID peripheral using GATT services rather than the L2CAP/SDP stack used by Bluetooth Classic. The result is a simpler, more reliable connection flow on modern platforms.

**What this adds:**
- Wireless gamepad output via BLE HID (HOGP) on CYW43 boards
- A new `INPUT_MODE_BLE` output mode alongside the existing `INPUT_MODE_BLUETOOTH` (Classic)
- Pairing and bonding without a PIN or numeric confirmation dialog
- Auto-reconnect using stored bonding keys
- Foundation for battery level reporting via the BLE Battery Service

**Why BLE instead of (or alongside) Bluetooth Classic:**

| | BT Classic HID | BLE HID (HOGP) |
|---|---|---|
| Windows 10/11 | ✅ Supported | ✅ Supported |
| Android | ✅ (varies by OEM) | ✅ Broadly supported |
| iOS / iPadOS | ❌ Not supported for gamepads | ✅ Supported |
| macOS | ✅ Supported | ✅ Supported |
| Pairing UX | PIN / numeric confirmation | Simple tap-to-pair |
| Connection reliability | Complex L2CAP/SDP negotiation | Simpler GATT connection |
| Power consumption | Higher | Lower |
| BTstack complexity | High (Classic HCI + L2CAP + SDP) | Lower (GATT server) |
| Audio support | Yes (not needed here) | No (irrelevant) |

BLE HID is the better choice for a gamepad that targets Windows, Android, and iOS universally. Bluetooth Classic may be revisited for platforms that require it, but BLE should be the primary wireless implementation.

---

## Platform Support

### Windows
Windows 10 (1809+) and Windows 11 fully support BLE HID gamepads via **Bluetooth LE Device**. The controller will appear in Device Manager as a Bluetooth LE HID-compliant game controller. No special drivers are needed — the inbox HID driver handles it.

### Android
Android 8.0+ supports BLE HID peripherals. The gamepad will appear as a Bluetooth input device and be recognized by games that accept standard Android gamepad input.

### iOS / iPadOS
iOS 13+ supports BLE HID gamepads via the Made for iPhone (MFi) HOGP profile. Controllers must advertise correctly (correct GATT services, correct appearance value for gamepad) to be recognized.

### macOS
macOS supports BLE HID devices natively. No additional configuration required.

---

## Technical Architecture

### Profile: HID Over GATT Profile (HOGP)

HOGP requires the device to expose the following GATT services:

| Service | UUID | Purpose |
|---|---|---|
| Human Interface Device | 0x1812 | HID Report Map, HID Report, HID Information, HID Control Point |
| Device Information | 0x180A | PnP ID, Manufacturer Name, Firmware Revision |
| Battery | 0x180F | Battery level percentage (0–100) |
| Generic Attribute | 0x1801 | Service Changed (for bonding) |

The **HID Report Map** is the same USB HID descriptor used by the existing USB HID driver — this gives us a free reuse of the existing gamepad report format across USB, BT Classic, and BLE modes.

### BTstack Integration

BTstack (the Bluetooth stack used by the Pico SDK) provides `hids_device` — a GATT server implementation for HOGP. This is distinct from the `hid_device` Classic HID implementation.

Key BTstack components:
- `hids_device.h` / `hids_device.c` — GATT HID server
- `battery_service_server.h` — BLE Battery Service
- `device_information_service_server.h` — Device Information Service
- `sm.h` — Security Manager for BLE pairing/bonding
- `gatt_client.h` — not needed (we are the peripheral)

### Run Loop

BLE HID on Pico W uses the same `pico_cyw43_arch_poll` + `pico_btstack_run_loop_async_context` integration as Bluetooth Classic. The `cyw43_arch_poll()` call in the main game loop services both the CYW43 hardware and the BTstack timer queue.

Unlike Classic HID, BLE does **not** use L2CAP or SDP — the GATT server is registered at startup and connections are handled entirely within the BLE stack.

### Security Manager (Bonding)

BLE HID requires **bonding** with the host to persist the connection key. BTstack's Security Manager (`sm_init()`, `sm_set_io_capabilities()`) handles this. For a gamepad with no display or input:
- IO capability: `IO_CAPABILITY_NO_INPUT_NO_OUTPUT`
- Bonding: enabled (`SM_AUTHREQ_BONDING`)
- MITM protection: not required (no display)
- Stored: bonded host address and LTK (Long Term Key) saved to flash via `BluetoothOptions`

---

## Implementation Plan

### Phase 1 — Core BLE HID (New Feature Branch)

**New files:**
- `headers/BLEHIDManager.h` — BLE HID singleton, GATT server lifecycle
- `src/BLEHIDManager.cpp` — HOGP init, advertising, report sending, bonding

**Modified files:**
- `CMakeLists.txt` — add `pico_btstack_ble` to BT libs (alongside `pico_btstack_classic`)
- `proto/enums.proto` — add `INPUT_MODE_BLE = 18`
- `proto/config.proto` — extend `BluetoothOptions` with BLE bonding fields
- `src/OutputManager.cpp` — add BLE branch alongside existing BT Classic branch
- `src/drivermanager.cpp` — add `INPUT_MODE_BLE` case
- `src/webconfig.cpp` — add BLE fields to `getAddonOptions` / `setAddonOptions`
- `www/src/Addons/Bluetooth.tsx` — add BLE mode toggle and BLE device display

**Key implementation notes:**
- `BLEHIDManager.cpp` must follow the same TU isolation as `BTHIDManager.cpp` — no `tusb.h` includes
- The HID report descriptor is shared with USB and BT Classic (`hid_descriptor_gamepad[]`)
- Advertising must include the HID service UUID and set the BLE appearance to `0x03C4` (Gamepad)
- `SM_EVENT_JUST_WORKS_REQUEST` — confirm automatically (no display/input)
- `SM_EVENT_PAIRING_COMPLETE` — save bonded address + IRK to flash
- On boot with stored bonding data: call `gap_connect()` to the stored host address

### Phase 2 — Coexistence (Classic + BLE)

Investigate whether both Classic HID and BLE HID can be active simultaneously on the same CYW43 (dual-mode BR/EDR + LE). BTstack supports this, but the Pico W firmware may have constraints. If coexistence is not practical, Classic and BLE remain mutually exclusive output modes selectable in Settings.

### Phase 3 — Battery Service

Once BLE HID is stable, integrate the BLE Battery Service to report LiPo charge level to the host (relevant for the Pimoroni Pico Lipo 2 XL W which has a battery gauge on GPIO29/ADC3).

---

## GATT Service Configuration

```
HID Service (0x1812)
  HID Information:    version=0x0111, countryCode=0x00, flags=0x02 (RemoteWake|NormallyConnectable)
  HID Control Point:  write-only (Suspend / ExitSuspend)
  Report Map:         READ — HID descriptor bytes (same as USB)
  HID Report [Input]: READ + NOTIFY, reportId=1, reportType=Input
  Protocol Mode:      READ + WRITE (Report Protocol = 0x01)

Device Information Service (0x180A)
  Manufacturer Name:  "OpenStick Community"
  Model Number:       "GP2040-CE"
  Firmware Revision:  build version string

Battery Service (0x180F)
  Battery Level:      READ + NOTIFY, value=0–100 (%)
```

---

## Configuration Storage

Extend `BluetoothOptions` in `proto/config.proto`:

```protobuf
message BluetoothOptions {
    bool enabled = 1;
    bool pairingMode = 2;
    bytes bondedDeviceAddr = 3;         // BT Classic bonded BD_ADDR (6 bytes)
    string bondedDeviceName = 4;        // BT Classic device name (display only)
    bytes bleBondedAddr = 5;            // BLE bonded host address (6 bytes)
    bytes bleIdentityResolvingKey = 6;  // BLE IRK for private address resolution (16 bytes)
    bytes bleLongTermKey = 7;           // BLE LTK for encryption (16 bytes)
}
```

---

## Acceptance Criteria

- [ ] Controller appears as a BLE gamepad in Windows 11 Bluetooth scan
- [ ] Pairs without PIN or numeric confirmation
- [ ] Buttons and axes register correctly in Windows game controller test
- [ ] Controller auto-reconnects on next power-on without re-pairing
- [ ] Stable connection maintained for 10+ minutes without drops
- [ ] RNDIS web config (USB) still functional when BLE is enabled
- [ ] Web config page correctly shows/saves BLE enable state
- [ ] No USB enumeration regression on boards without CYW43

---

## Open Questions

1. **Mode selection:** Should BLE and Classic be selectable as separate output modes (`INPUT_MODE_BLUETOOTH` vs `INPUT_MODE_BLE`), or should BLE be the default with Classic as a fallback? Recommendation: separate modes — let the user choose.

2. **iOS support:** The iOS MFi gamepad profile has additional requirements beyond HOGP (specific button mappings, L2/R2 axis treatment). Investigate whether standard HOGP is sufficient or if Apple-specific advertising data is needed.

3. **Classic + BLE coexistence:** Can both be active simultaneously? Useful for pairing from scratch (BLE) while staying connected to an existing Classic session — low priority but worth exploring.

4. **BLE bonding storage:** BTstack's Security Manager maintains an internal bonding database. We need to persist this to flash on power-down and restore it on boot. Determine whether BTstack provides a flash bank API we can hook or if we must manage it manually via `BluetoothOptions`.
