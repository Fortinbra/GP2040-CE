# BLE Device Identity (PnP ID and DIS Hardening)

**Last updated:** 2026-05-24
**Maintained by:** GP2040-CE core team
**Status:** Feature spec — signed off, ready for implementation
**Related:** [bluetooth-support.md](./bluetooth-support.md), [ble-hid-report-rework.md](./ble-hid-report-rework.md)

---

## Purpose

Make BLE HID hosts (Windows Game Controllers / Gamepad Tester, Android, macOS, Linux `evdev`) display a meaningful vendor / product identity for the controller instead of `Unknown Gamepad / Vendor: 0000 / Product: 0000`.

Concretely:

1. Populate the BLE **PnP ID** characteristic (0x2A50) in the Device Information Service with a non-zero Vendor Source / Vendor ID / Product ID / Product Version.
2. Tighten the rest of the Device Information Service so the displayed metadata is consistent and useful.
3. Keep the GAP Device Name (`"GP2040-CE Gamepad"`) shown in the OS Bluetooth pairing UI.
4. Provide per-board override hooks so individual boards / forks can use their own identity without forking firmware code.

## Why hosts currently show `Unknown / 0000 / 0000`

The BLE HID host derives the visible name and the *Vendor*/*Product* fields from the **PnP ID** characteristic (0x2A50) in the Device Information Service. The DIS we ship today:

- Imports the template `device_information_service.gatt` from BTstack (`src/ble_hid.gatt`), which declares the `PNP_ID` characteristic with no value.
- In `BLEHIDManager::init()` (see [src/BLEHIDManager.cpp](../../src/BLEHIDManager.cpp)) we set:
  - `device_information_service_server_set_manufacturer_name("OpenStickCommunity")`
  - `device_information_service_server_set_model_number("GP2040-CE")`
  - `device_information_service_server_set_firmware_revision("1.0")`
- We **never** call `device_information_service_server_set_pnp_id(...)`, so the 7-byte PnP ID record stays all zero. Windows in particular keys its `Game Controllers` UI on PnP ID, so it falls back to `Unknown Gamepad / Vendor: 0000 / Product: 0000`.

The GAP Device Name (`"GP2040-CE Gamepad"`) and GAP Appearance (`0x03C4 Gamepad`) are correctly set in `src/ble_hid.gatt` and only drive the OS pairing UI, not the Gamepad Tester display.

## Scope

In scope:

- Add `device_information_service_server_set_pnp_id(...)` to `BLEHIDManager::init()`.
- Define default identity macros (Vendor Source ID, Vendor ID, Product ID, Product Version) in a single, central header.
- Allow per-board override of those macros via `BoardConfig.h` (or a sibling minimal header, see "Macro Visibility" below).
- Also populate the currently-empty DIS strings that materially help host display:
  - Hardware Revision String (board name, e.g. `"PimoroniPicoLipo2XLW"`).
  - Software Revision String (firmware version, derived from `version.h`).
  - Optional: Serial Number String (derived from RP2040/RP2350 unique chip ID).
- Document the re-pair requirement (DIS changes are cached on the host alongside the HID Report Map).

Out of scope (deferred):

- WebUI configuration of identity. v1 is firmware-time only.
- Per-input-mode VID/PID rotation (e.g. mimicking XInput VID/PID 0x045E/0x028E when XInput mode is selected) — BLE does not negotiate console-specific protocols, so doing this would mislead hosts into loading drivers that can't talk to a BLE endpoint. Out of scope here; revisit when/if a real BLE-XInput emulation lands.
- IEEE 11073 regulatory data, System ID, Serial Number certificate chain.

## PnP ID — Format and Strategy

### Wire format

The PnP ID characteristic (0x2A50) is exactly 7 bytes, little-endian:

| Bytes | Field                  | Notes                                                  |
|-------|------------------------|--------------------------------------------------------|
| 0     | Vendor ID Source       | `0x01` = Bluetooth SIG, `0x02` = USB Implementers Forum |
| 1–2   | Vendor ID              | LE                                                     |
| 3–4   | Product ID             | LE                                                     |
| 5–6   | Product Version        | LE; conventionally BCD `JJ.M.N` packed as `0xJJMN`     |

### Default identity (proposed)

| Field            | Default value          | Rationale                                                                                  |
|------------------|------------------------|--------------------------------------------------------------------------------------------|
| Vendor Source    | `0x02` (USB-IF)        | Windows and most desktop OSes index USB-IF VIDs in their PnP / `usb.ids` databases.        |
| Vendor ID        | `0x1209` (pid.codes)   | Public, free, community-allocated USB-IF VID for open-hardware projects. **Not** Microsoft / Sony / Nintendo. |
| Product ID       | **TBD** — see options  | See "Product ID options" below.                                                            |
| Product Version  | Derived from `version.h` (firmware version, BCD) | Lets hosts and crash logs see firmware level.                       |

We deliberately do **not** default to `0x045E` (Microsoft) or `0x054C` (Sony). Spoofing those VIDs over BLE causes Windows to try to bind the Xbox / DualShock class drivers, which then fail to enumerate because the transport is BLE, not USB. The result is the controller silently fails to appear in Game Controllers despite being paired.

### Product ID options

Pick exactly one default before implementation.

- **Option A — single fixed community PID.** Allocate one PID under pid.codes (e.g. `0x0001` placeholder) and use it for every GP2040-CE BLE build by default. Boards override only if they want a distinct identity.
- **Option B — per-board PID via `BoardConfig.h`.** No firmware-wide default; every board that enables BLE must define `BLE_PRODUCT_ID`. Build error if undefined when `ENABLE_BLUETOOTH` is on. Highest visibility, most friction.
- **Option C — single fixed PID + optional override.** Same as A, but also document the override macro for boards/forks. **Recommended** as v1 default.

### Override surface (macros)

All identity macros are weak: a board can override them in `BoardConfig.h` and the rest of the firmware picks them up. A sibling `BleIdentityConfig.h` next to `BatteryConfig.h` is an option too, but unlike battery sense, identity does not require special header-conflict isolation; the macros are plain integers and short strings, so `BoardConfig.h` is fine.

Proposed macros (defaults applied in a central header, e.g. `headers/BLEHIDManager.h` or a new `headers/ble_identity.h`):

```c
#ifndef BLE_VENDOR_SOURCE_ID
#define BLE_VENDOR_SOURCE_ID  0x02                  // USB-IF
#endif
#ifndef BLE_VENDOR_ID
#define BLE_VENDOR_ID         0x1209                // pid.codes
#endif
#ifndef BLE_PRODUCT_ID
#define BLE_PRODUCT_ID        0x0001                // placeholder, TBD
#endif
#ifndef BLE_PRODUCT_VERSION
#define BLE_PRODUCT_VERSION   0x0100                // 1.0.0 (BCD)
#endif
#ifndef BLE_MANUFACTURER_NAME
#define BLE_MANUFACTURER_NAME "OpenStickCommunity"
#endif
#ifndef BLE_MODEL_NUMBER
#define BLE_MODEL_NUMBER      "GP2040-CE"
#endif
#ifndef BLE_HARDWARE_REVISION
#define BLE_HARDWARE_REVISION GP2040_BOARDCONFIG    // stringified board name
#endif
#ifndef BLE_SOFTWARE_REVISION
#define BLE_SOFTWARE_REVISION GP2040_VERSION_STRING // from version.h
#endif
```

Boards override by defining any subset of these before `BLEHIDManager.h` is reached, via their `BoardConfig.h`.

### Macro Visibility — Translation-Unit Constraint

`src/BLEHIDManager.cpp` deliberately does **not** include `BoardConfig.h` (see [bluetooth-support.md](./bluetooth-support.md#L210)). The reason: `BoardConfig.h` transitively pulls in TinyUSB's `class/hid/hid.h`, whose `hid_report_type_t` collides with BTstack's `hid_report_type_t` in the BLE translation unit.

Two equivalent ways to honor per-board overrides without re-introducing that conflict:

1. **Mirror the `BatteryConfig.h` pattern.** Put identity macro overrides in a new minimal, dependency-free `configs/<Board>/BleIdentityConfig.h`. `BoardConfig.h` includes it (so the rest of the firmware sees the same overrides). `src/BLEHIDManager.cpp` includes it via `__has_include("BleIdentityConfig.h")`. Default values live in a central header that `BLEHIDManager.cpp` already includes (e.g. `headers/BLEHIDManager.h`).
2. **Add a thin `ble_identity.h`** that only includes the per-board override header (when present) and then `#ifndef`-guards every macro to a default. Both `BoardConfig.h` and `BLEHIDManager.cpp` include `ble_identity.h` directly. This keeps the override knobs in one place at the cost of one extra header.

**Recommended:** option 1, to match the existing `BatteryConfig.h` pattern that the team already understands.

## Implementation Plan

1. **Define defaults.** Add the macro block above to a central header (`headers/BLEHIDManager.h` or new `headers/ble_identity.h`).
2. **Per-board overrides (optional).** Document the `configs/<Board>/BleIdentityConfig.h` pattern. No board ships an override in v1 — the community defaults are the visible identity.
3. **Wire identity in `BLEHIDManager::init()`.** Right after the existing DIS calls:
   ```cpp
   device_information_service_server_set_manufacturer_name(BLE_MANUFACTURER_NAME);
   device_information_service_server_set_model_number(BLE_MODEL_NUMBER);
   device_information_service_server_set_hardware_revision(BLE_HARDWARE_REVISION);
   device_information_service_server_set_software_revision(BLE_SOFTWARE_REVISION);
   device_information_service_server_set_firmware_revision(BLE_SOFTWARE_REVISION);
   device_information_service_server_set_pnp_id(BLE_VENDOR_SOURCE_ID,
                                                BLE_VENDOR_ID,
                                                BLE_PRODUCT_ID,
                                                BLE_PRODUCT_VERSION);
   ```
   Drop the hardcoded `"1.0"` firmware revision string.
4. **Verify GATT layout.** No changes required to `src/ble_hid.gatt` — the imported `device_information_service.gatt` already declares the PnP ID characteristic; we are only filling its value at runtime.
5. **Re-pair guidance.** Add a short note to [bluetooth-support.md](./bluetooth-support.md) under "Host Compatibility": hosts cache DIS values per bond, so existing pairings will continue to report `0000`. Unpair and re-pair on every host to refresh PnP ID.

## Verification Plan

Per `clean-build-required.instructions.md`, end with a `--fresh` configure + full Ninja build for `PimoroniPicoLipo2XLW`.

On hardware, after unpair / re-pair on each host:

- **Windows 10/11 Game Controllers:** name is no longer `Unknown Gamepad`; "Properties" shows non-zero VID/PID matching the chosen defaults.
- **Gamepad Tester (web):** Vendor and Product fields are non-zero.
- **Android Bluetooth → device info:** shows `OpenStickCommunity` as manufacturer and `GP2040-CE` as model.
- **macOS `Game Controller` framework:** identifies the device with the new VID/PID.
- **Linux `evdev`:** `/proc/bus/input/devices` shows non-zero Vendor and Product, and `Name=` reflects the BLE-advertised name.

Regression checks:

- HID input still works (battery + 14-button digital report).
- No new ATT errors on connect; `service_server_set_*` calls are pure value-store operations and cannot fail at runtime.

## Risk and Rollback

- **Risk — vendor squatting.** If we pick a default VID that collides with someone else's USB device, Windows may try to load that vendor's driver. Mitigated by using pid.codes `0x1209` (community-managed, conflict-resistant).
- **Risk — stale bonds.** Existing pairings won't see the new PnP ID until re-paired. Documented; not a code problem.
- **Rollback.** Revert the `BLEHIDManager::init()` block and the macro defaults. No persistent storage, no schema impact.

## Resolved Decisions

1. **Product ID strategy:** **Option C** — single fixed firmware-wide default with optional per-board override.
2. **Default Product ID:** `0x0001` placeholder under pid.codes VID `0x1209`. Revisit if/when a coordinated pid.codes allocation is acquired for GP2040-CE.
3. **Per-board override mechanism:** `configs/<Board>/BleIdentityConfig.h`, mirroring the existing `BatteryConfig.h` pattern. `BoardConfig.h` includes it (for the rest of the firmware) and `src/BLEHIDManager.cpp` includes it directly via `__has_include("BleIdentityConfig.h")` to preserve the TinyUSB-conflict isolation. Defaults live in a central header (`headers/BLEHIDManager.h` or a new `headers/ble_identity.h`).
4. **Serial Number characteristic:** populate from the RP2040/RP2350 unique chip ID via `pico_get_unique_board_id()`. Format as uppercase hex, no separators.
