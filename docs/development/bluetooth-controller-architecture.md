# Bluetooth Controller Architecture: BLE Controller Types, Keyboard/Mouse Modes, and Classic/LE Switching

**Last updated:** 2026-06-04  
**Maintained by:** GP2040-CE contributors  
**Status:** Research + feature design (implementation-ready roadmap)

---

## Purpose

Define a safe architecture for supporting multiple Bluetooth controller types (starting with BLE), adding keyboard/mouse modes, and introducing Classic-vs-LE switching, while preserving all existing GP2040-CE behavior.

This document is based on current repository implementation and is intended to guide future incremental work.

---

## What Exists Today (Verified Findings)

## 1) Runtime mode and transport split

- Active mode selection flows through `InputMode` (`proto/enums.proto`), with `INPUT_MODE_BLE = 18`.
- USB output is handled by `DriverManager` + per-mode `GPDriver` implementations (`headers/gpdriver.h`, `src/drivermanager.cpp`, `src/usbdriver.cpp`).
- BLE output is handled separately by `OutputManager::dispatch()` and `BLEHIDManager` (`src/OutputManager.cpp`, `src/BLEHIDManager.cpp`).
- In `GP2040::run()`, BLE mode is treated as **wireless-only**: TinyUSB device task is skipped and BTstack is pumped via `BLEHIDManager::process()` (`src/gp2040.cpp`).

## 2) BLE is currently LE-only HID gamepad

- Build-time BLE enablement is gated by `PICO_CYW43_SUPPORTED` and `ENABLE_BLUETOOTH` (`CMakeLists.txt`).
- Current BLE link libraries are LE-centric (`pico_btstack_ble`, `pico_btstack_cyw43`) and do not enable an active Classic HID stack path.
- Advertising flags explicitly state BR/EDR not supported (`src/BLEHIDManager.cpp`, `adv_data` flags `0x06`).
- Current BLE HID payload is fixed digital gamepad report (3 bytes) (`headers/BLEHIDManager.h`, `src/BLEHIDManager.cpp`, `src/OutputManager.cpp`).

## 3) BLE management and persistence already exist

- Pairing/bonding: enabled through BTstack SM and protobuf-backed `le_device_db` replacement (`src/BLEHIDManager.cpp`, `src/le_device_db_proto.cpp`).
- Web API control surface exists for status/pairing/clear-bonds (`/api/getBLEHIDStatus`, `/api/setBLEHIDControls`) in firmware and web UI (`src/webconfig.cpp`, `www/src/Services/WebApi.js`, `www/src/Pages/SettingsPage.jsx`).

## 4) USB encapsulation precedent is strong

- USB output modes are encapsulated as independent driver classes selected in one switch (`src/drivermanager.cpp`).
- USB host side also encapsulates controller protocol families in listener code (`src/usbhostmanager.cpp`, `src/addons/gamepad_usb_host_listener.cpp`, `src/addons/keyboard_host_listener.cpp`).
- This existing pattern is the correct model for Bluetooth controller-type encapsulation.

## 5) Keyboard/mouse context today

- There is a USB Keyboard output mode (`src/drivers/keyboard/KeyboardDriver.cpp`).
- There is no dedicated USB Mouse output mode as an `InputMode`.
- USB host keyboard/mouse input exists as addon/listener behavior, mapped into `GamepadState` and aux sensor state (`src/addons/keyboard_host_listener.cpp`).
- BLE currently exposes only gamepad HID over GATT, not keyboard/mouse profiles.

---

## Problem Statement

Current BLE implementation is monolithic and gamepad-specific. We need a structure that:

1. Encapsulates Bluetooth controller types as independently evolvable modules (similar to USB driver model).
2. Adds keyboard and mouse Bluetooth modes safely.
3. Allows LE vs Classic selection without breaking existing LE gamepad behavior.
4. Preserves current config, web controls, pairing persistence, and non-Bluetooth builds.

---

## Proposed Target Architecture

## A) Separate concerns into three layers

1. **Bluetooth Transport Layer**  
   Handles radio bring-up, stack init, advertising/discovery, connection lifecycle, security, and bond storage.

2. **Bluetooth Controller Profile Layer**  
   One module per controller type/protocol shape (Gamepad LE, Keyboard LE, Mouse LE, future Classic HID variants).

3. **Bluetooth Mode Policy Layer**  
   Decides which transport/profile is active from persisted config + runtime constraints.

This mirrors existing USB separation:
- transport plumbing (`usbdriver.cpp` / TinyUSB callbacks),
- per-mode protocol drivers (`drivers/*Driver.cpp`),
- centralized mode selection (`DriverManager::setup`).

## B) Introduce a Bluetooth profile interface

Define a profile contract (conceptually parallel to `GPDriver`), e.g.:

- `init(profileConfig)`
- `onConnected/onDisconnected`
- `buildReport(GamepadState, AuxState)`
- `sendIfChanged()`
- `supportsPairingModeControl()`
- `getIdentity/Appearance metadata`

Then implement:

- `BleGamepadProfile` (maps current behavior 1:1 first)
- `BleKeyboardProfile` (HOGP keyboard)
- `BleMouseProfile` (HOGP mouse)
- future: `ClassicHidGamepadProfile`, `ClassicHidKeyboardProfile`, `ClassicHidMouseProfile`

## C) Keep `BLEHIDManager` as compatibility shell initially

To avoid regression risk:

- Keep current `BLEHIDManager` public API stable for current web/UI call sites.
- Internally delegate report descriptor + report packing + service registration to active profile module.
- Stage 1 should keep default profile = current gamepad LE report exactly.

---

## Keyboard and Mouse Mode Design

## LE keyboard mode (HOGP keyboard)

- Add keyboard report map and input reports through HIDS over GATT.
- Reuse existing keyboard mapping semantics from USB keyboard mode where possible.
- Ensure modifier/media key handling is explicit; keep parity with `KeyboardDriver` behavior.

## LE mouse mode (HOGP mouse)

- Add mouse report map (buttons, X/Y, wheel).
- Define mapping source: either direct aux mouse sensor state or mapped gamepad-state policy.
- Reuse existing `KeyboardHostListener` mouse movement mapping logic as behavioral reference.

## Multi-profile advertising policy

- Prefer single active Bluetooth profile per selected input mode to minimize host confusion and cache issues.
- Avoid simultaneous mixed HID persona until profile-switching and host-cache behavior is fully validated.

---

## Classic vs LE Switching Design

## Build-time switch

Current: LE-only link path in `CMakeLists.txt`.  
Future:

- Add explicit compile options for:
  - LE only
  - Classic only
  - Dual-mode build
- Keep current default as LE-only to preserve existing functionality.

## Runtime switch

Add persisted Bluetooth mode policy (separate from USB `InputMode` semantics), e.g.:

- `BT_MODE_LE`
- `BT_MODE_CLASSIC`
- `BT_MODE_AUTO` (optional future)

Rules:

- On unsupported board/build combos, fall back deterministically to current BLE LE behavior or disable Bluetooth with clear status.
- Switching mode triggers controlled disconnect + stack/profile reinit.
- Preserve bond/identity stores per transport type to avoid cross-mode corruption.

## Why this matters

Classic HID and LE HOGP differ heavily in discovery, security, descriptor handling, and host caching behavior. Treating mode switching as a full transport policy change avoids hidden state bugs.

---

## Protocol Research Baseline (for upcoming implementations)

## LE HID (HOGP) essentials

- ATT/GATT services:
  - HID Service (0x1812)
  - Battery Service (0x180F)
  - Device Information Service (0x180A)
- Report identity and host cache behavior are tied to Report Map/Report Reference.
- Pairing/bonding and encryption sequencing is critical before notifications.

## Classic HID essentials

- BR/EDR stack path with SDP + L2CAP + HID Control/Interrupt channels.
- Different host-driver binding behavior vs BLE HOGP.
- Different reconnect/latency and sniff/idle power behavior.

## BTstack + Pico SDK integration implications

- Library selection in CMake determines available protocol surfaces.
- Event-loop ownership (current CYW43 async context + `cyw43_arch_poll`) must remain single-owner and non-blocking.
- Header/type conflicts already exist between TinyUSB and BTstack (`hid_report_type_t`), so profile modules must keep strict include hygiene (as done in `BLEHIDManager.cpp`).

---

## Backward-Compatibility Requirements (Non-Negotiable)

1. Existing BLE gamepad mode behavior must remain default and unchanged until explicitly switched.
2. Existing USB modes and `DriverManager` behavior must remain unchanged.
3. Existing web API BLE endpoints must continue to function during transition.
4. Existing bond persistence (`le_device_db_proto`) must remain valid for LE path.
5. Non-CYW43 boards must retain USB-only build/runtime behavior with zero new overhead.

---

## Recommended Phased Implementation

## Phase 0: Safety scaffolding

- Introduce Bluetooth profile interface + adapter in manager.
- Route current gamepad LE path through new interface with zero payload/descriptor changes.

## Phase 1: LE profile expansion

- Add LE keyboard profile.
- Add LE mouse profile.
- Add profile-specific validation matrix for pairing, reconnect, and report correctness.

## Phase 2: Mode policy and persistence

- Add persisted Bluetooth transport/profile config.
- Add UI/API controls for selecting Bluetooth profile and transport policy.
- Keep defaults pinned to current LE gamepad behavior.

## Phase 3: Classic transport bring-up

- Add Classic build and runtime path behind feature guards.
- Implement first Classic HID profile (gamepad) with isolated bond/session handling.
- Validate coexistence and fallback behavior.

## Phase 4: Dual-mode hardening

- Optional auto-selection policy, only after deterministic host interoperability evidence.
- Add migration and troubleshooting docs for host cache clearing and re-pair requirements.

---

## Validation Matrix (Required Before Broad Enablement)

- Build matrix:
  - non-CYW43 board (must remain unaffected)
  - CYW43 LE-only
  - CYW43 Classic-only (future)
  - CYW43 dual-mode (future)
- Runtime:
  - fresh pair / bonded reconnect / reboot reconnect
  - descriptor change with mandatory re-pair
  - web pairing toggle + clear bonds
- Host matrix:
  - Windows, Android, macOS, Linux
  - keyboard and mouse host behavior where supported
- Regression:
  - USB mode behavior unchanged
  - existing BLE gamepad latency and stability not degraded

---

## Risks and Mitigations

- **Risk:** Host cache and identity mismatches across profile changes.  
  **Mitigation:** explicit re-pair guidance, profile/versioned identity strategy.

- **Risk:** TinyUSB/BTstack type/header conflicts during modularization.  
  **Mitigation:** preserve dependency-isolated headers (BatteryConfig/BleIdentityConfig pattern).

- **Risk:** Runtime switch race conditions in transport restart.  
  **Mitigation:** single state machine owner, queued transitions, no blocking in event handlers.

- **Risk:** Regressing current BLE mode while adding new profiles.  
  **Mitigation:** adapter-first migration where current gamepad LE module is baseline reference implementation.

---

## Immediate Next Actions

1. Approve this architecture split (transport/profile/policy) as the implementation contract.
2. Define the concrete Bluetooth profile interface in headers.
3. Refactor current LE gamepad path into `BleGamepadProfile` with byte-for-byte behavioral parity.
4. Add design docs for LE keyboard and LE mouse report maps and mapping policy.
5. Decide Classic enablement strategy (LE default vs dual-mode opt-in) before CMake/link changes.

