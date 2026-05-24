# Bluetooth Support (Consolidated)

**Last updated:** 2026-05-23  
**Maintained by:** GP2040-CE core team  
**Status:** Active implementation + ongoing refinement  
**SDK baseline:** 2.2.0+

---

## Purpose

This is the single source of truth for Bluetooth support in GP2040-CE.

It consolidates and replaces the following older docs:

- docs/development/ble-hid-support.md
- docs/development/ble-hid-gap-audit-2026-05-15.md
- docs/development/ble-xinput-report.md
- docs/development/btonly-firmware-variant.md
- docs/development/bt-architecture.md

---

## Scope

This document covers:

- Bluetooth support architecture in firmware
- BLE HID behavior and host compatibility
- Pairing, bonding, and operator controls
- HID report format currently used by BLE output
- Battery and power-management behavior
- Experimental Bluetooth-only firmware variant

It does not cover general USB output modes or unrelated wireless planning.

---

## Current State Summary

Bluetooth support is implemented for CYW43-capable boards with an active BLE HID path.

Implemented in firmware:

- BLE mode dispatch path from gamepad state to BLE output
- BLE HID manager initialization and report delivery
- LE Secure Connections + bonding model
- Bond persistence through protobuf-backed BLE config storage
- Runtime BLE control endpoints for status, pairing mode, and bond clear

Still in active refinement:

- Web UI polish for BLE controls and diagnostics
- Cross-host regression coverage matrix (fresh pair/reconnect/power-cycle)
- Optional policy tuning (for example persisted pairing mode defaults)

---

## Supported Hardware

Bluetooth functionality requires CYW43-supported targets.

| Board family | Wireless support | Notes |
|---|---|---|
| Pico W | Yes | BLE-capable target |
| Pico 2 W | Yes | BLE-capable target |
| Pimoroni Pico Lipo 2 XL W | Yes | Recommended battery-backed reference board. Board-specific details are in [Pimoroni Pico Lipo 2 XL W Board Support](./pimoroni-pico-lipo-2xl-w-support.md). |
| Non-CYW43 boards | No | USB only |

Build integration is gated through `PICO_CYW43_SUPPORTED` in CMake.

---

## Output Architecture

GP2040-CE output architecture now supports wireless output paths in addition to USB.

High-level flow:

1. Main loop builds current gamepad state.
2. Output selection chooses active transport.
3. BLE path serializes and pushes HID reports through BTstack.

Important behavior constraints:

- USB and Bluetooth are separate transports with explicit mode behavior.
- Web configuration and Bluetooth coexist on CYW43 targets.
- Non-wireless targets continue to use USB-only behavior.

### Runtime Architecture Reference

BLE runtime path is centered on `BLEHIDManager` in the Core 0 loop, with BTstack work
serviced through the CYW43 async context.

Main data path:

1. `GP2040::run()` reads/processes gamepad state.
2. `OutputManager::dispatch()` maps gamepad state to BLE HID payload.
3. `BLEHIDManager::sendReport()` queues report when connected and notifications are enabled.
4. `BLEHIDManager::process()` requests can-send-now events and pushes reports through BTstack.
5. BTstack sends ATT notifications through CYW43 to the host.

Connection lifecycle:

- Fresh pair: advertise -> connect -> security manager pairing -> bond store update -> encrypted link -> notifications enabled.
- Bonded reconnect: advertise -> connect -> identity resolve -> encryption restore -> notifications enabled.

Power-state behavior tracks BLE activity:

- `ADVERTISING`: discoverable, no report sends.
- `ACTIVE`: normal report flow and periodic battery updates.
- `IDLE`: throttled report cadence until input changes.

Build integration:

- BLE sources and linkage are enabled only when `PICO_CYW43_SUPPORTED` is true.
- `ENABLE_BLUETOOTH=1` is the compile-time guard used in BLE call sites.

---

## BLE HID Implementation Notes

### Stack and Security Requirements

- BLE uses BTstack on CYW43.
- Security profile uses bonding + LE Secure Connections.
- Bond records are stored via the firmware BLE config backend.

### Advertising and Bring-Up

- BLE advertising should only start after the stack reaches working state.
- Initialization order must keep BLE storage and security setup valid before active pairing.

### Runtime Operator Controls

BLE status and controls are exposed through web API endpoints:

- `/api/getBLEHIDStatus`
- `/api/setBLEHIDControls`

Current control surface includes:

- Current BLE enable/connection state visibility
- Pairing-mode toggle
- Clear stored bonds operation

---

## BLE Report Format

BLE report layout uses an XInput-style shape for host friendliness while remaining standard BLE HID.

Current intended payload profile:

- 13-byte input report
- Named primary button mapping (A/B/X/Y style mapping)
- Hat switch for D-pad
- Separate trigger bytes
- Signed 16-bit stick axes

Operational notes:

- Report ID remains defined by HID descriptor/Report Reference, not prepended in ATT payload.
- Descriptor/report changes require host-side unpair/re-pair to refresh cached HID metadata.

---

## Host Compatibility

### BLE HID

- Windows 10/11: supported (generic HID gamepad behavior)
- Android: supported
- macOS: generally supported as generic HID gamepad
- Nintendo Switch: BLE HID gamepad not supported

### BT Classic HID

BT Classic remains relevant for platforms requiring classic profile behavior.

---

## Battery Reporting

Battery reporting is integrated into BLE path behavior for supported battery-backed boards.

Key points:

- Battery percentage is exposed to connected hosts through BLE battery service behavior.
- Board battery sense macros determine whether real ADC-based values are available.
- If board-level battery sense definitions are absent, fallback behavior applies.
- For Pimoroni Pico Lipo 2 XL W (`PimoroniPicoLipo2XLW`), the canonical board mapping is documented in [Pimoroni Pico Lipo 2 XL W Board Support](./pimoroni-pico-lipo-2xl-w-support.md) (battery sense on GP43 / ADC 3).

Recommended validation:

1. Verify board ADC battery definitions.
2. Verify measured voltage versus reported percent at low/mid/high points.
3. Validate host-observed battery changes during discharge/charge states.

### ADC Threshold Values

The ADC raw-count thresholds used for battery percentage mapping (currently hardcoded as `1241` and `1737`) must be derived from board config macros — specifically `BATTERY_VOLTAGE_DIVIDER`, `BATTERY_MIN_VOLTAGE`, and `BATTERY_MAX_VOLTAGE` — rather than literal constants. This change is required before battery boards other than the Pimoroni reference can be declared correct.

### RP2350B ADC Channel Mapping

For the Pimoroni Pico Lipo 2 XL W, battery sense is documented as ADC channel 3 → GPIO 43. This mapping must be verified against the RP2350B datasheet and the Pimoroni schematic before the battery read path is considered validated on that board.

---

## Display Integration

BLE connection state and battery level must be surfaced on the OLED status bar when BLE mode is active. All BLE display code must be wrapped in `#ifdef ENABLE_BLUETOOTH` guards.

### Status Bar (OLED)

Authorized change: add an `INPUT_MODE_BLE` case to `generateHeader()` in `src/display/ui/screens/ButtonLayoutScreen.cpp`.

Required behavior:

- BLE connected, battery hardware present: display `CHAR_BT` + `"BLE "` + battery percentage (e.g. `"BLE 87%"`) — total token must not exceed the 21-character row limit.
- BLE connected, no battery hardware: display `CHAR_BT` + `"BLE"` only.
- BLE advertising / not yet connected: display `CHAR_BT` + `"BLE..."` or `CHAR_BT` + `"BLE"` (advertising indicator).
- Whether battery hardware is present is determined by the board config battery sense macros (`BATTERY_VOLTAGE_DIVIDER`, etc.). Boards lacking these macros are treated as "no battery hardware".
- The display code reads battery level through `BLEHIDManager::getBatteryLevel()` (see below); it must not trigger an ADC read directly.

### Bluetooth Glyph Character

Authorized change: add a Bluetooth symbol glyph as custom character `\x94` (`CHAR_BT`) in the following font files:

- `GP_Font_Standard`
- `GP_Font_Basic`
- `GP_Font_Big`

`CHAR_BT` must be defined as `"\x94"` in `headers/display/GPGFX_core.h`. The glyph is used as a prefix in the BLE status bar token described above.

Glyph form is the stylised "runic B" silhouette used by the standard Bluetooth logo (two stacked triangles sharing a vertical bar), rendered in the 5×7 cell shared by these fonts. The original placeholder "bowtie" glyph that shipped in `GP_Font_Standard` has been replaced with the runic-B form to match the other two fonts. All three fonts must remain visually consistent at the `CHAR_BT` codepoint.

### BLEHIDManager Public Getter

Authorized change: expose `getBatteryLevel()` as a public method on `BLEHIDManager`, returning the cached `_lastBatteryLevel` value without triggering an ADC read.

This getter is required by the display code so that `ButtonLayoutScreen` can read current battery state through the manager interface rather than accessing hardware directly.

Affected files:

- `headers/BLEHIDManager.h` — public method declaration
- `src/BLEHIDManager.cpp` — implementation (return `_lastBatteryLevel`)

### DriverManager Dispatch for INPUT_MODE_BLE

Authorized change: `DriverManager::setup()` must accept `INPUT_MODE_BLE` as a first-class mode.

BLE HID is a wireless-only output transport; it has no TinyUSB driver instance. The previous switch in `src/drivermanager.cpp` fell through to the `default:` arm for `INPUT_MODE_BLE` and returned without recording the active mode, so `DriverManager::getInputMode()` continued to report whatever USB mode was previously selected. Consumers that branch on `getInputMode()` — most notably the OLED status bar in `ButtonLayoutScreen::generateHeader()` — therefore never saw `INPUT_MODE_BLE` and could not render the BLE token.

Required behavior in `DriverManager::setup()` when `mode == INPUT_MODE_BLE` (guarded by `#ifdef ENABLE_BLUETOOTH`):

- Record the mode (`inputMode = mode`) so that subsequent `getInputMode()` calls report BLE.
- Do **not** instantiate any TinyUSB driver.
- Return without touching `driver`.

This is the minimal, surgical change required to let display and other consumers detect BLE mode. Wider driver-manager refactoring is out of scope here.

Affected files:

- `src/drivermanager.cpp` — add `INPUT_MODE_BLE` case to the mode switch.

---

## Battery Reporting — Translation-Unit Macro Visibility

`src/BLEHIDManager.cpp` deliberately does **not** include `BoardConfig.h`. The per-board `BoardConfig.h` transitively includes TinyUSB's `class/hid/hid.h`, which declares `hid_report_type_t` — a name that also exists in BTstack and is required by this translation unit. Including both causes a hard compile error in `BLEHIDManager.cpp`.

Consequence (historical bug): without `BoardConfig.h`, the battery-sense macros (`BATTERY_ADC_GPIO`, `BATTERY_ADC_CHANNEL`, `BATTERY_VOLTAGE_DIVIDER`, `BATTERY_MIN_VOLTAGE`, `BATTERY_MAX_VOLTAGE`) were undefined inside `BLEHIDManager.cpp`, and `_readBatteryPercent()` silently fell through to the `#else` branch which returns `100`. The OLED and BLE host both reported a stuck 100 % indefinitely.

Authorized fix: per-board battery-sense macros live in a minimal, dependency-free header `configs/<Board>/BatteryConfig.h` co-located with `BoardConfig.h`. The board's `BoardConfig.h` includes `BatteryConfig.h` (so the full firmware build still sees the macros), and `src/BLEHIDManager.cpp` pulls the same file in through `__has_include("BatteryConfig.h")`. The header must include nothing else — in particular it must not include `class/hid/hid.h`, `enums.pb.h`, or any other header that introduces TinyUSB/BTstack-conflicting typedefs.

Boards without battery sense hardware simply omit `BatteryConfig.h`; `_readBatteryPercent()` then keeps its existing `100 %` fallback.

Reference implementation: `configs/PimoroniPicoLipo2XLW/BatteryConfig.h`.

---

## Power Management

Bluetooth-enabled targets require explicit power-state behavior.

Typical model:

- USB-powered/full-performance behavior when external power is present
- Active, idle, and deeper sleep states for battery scenarios
- BLE reconnect behavior after wake as part of overall user-experience tuning

Power tuning remains board- and use-case-sensitive and should be validated per target.

---

## Experimental Bluetooth-Only Variant

A Bluetooth-only firmware variant is still considered experimental.

Concept:

- BLE HID for gameplay
- WiFi web config for management
- No USB HID/RNDIS path

Use cases:

- End-to-end wireless validation
- Battery-focused experiments

This remains a dedicated variant track and not a replacement for standard USB-capable builds.

---

## Operational Checklist

When validating Bluetooth functionality on a target board:

1. Build on a CYW43-capable board config.
2. Confirm BLE mode selection and advertising behavior.
3. Pair from at least one Windows host and one Android host.
4. Verify input reports, reconnect behavior, and bond persistence across reboot.
5. Exercise pairing mode toggle and clear-bonds controls.
6. Re-pair after descriptor/report changes.

---

## Future Work

- Expand and harden BLE UI controls in Web Config.
- Complete host matrix regression checklists.
- Continue tightening documentation around mode selection and troubleshooting.
- Derive ADC battery thresholds from board config macros (`BATTERY_VOLTAGE_DIVIDER`, `BATTERY_MIN_VOLTAGE`, `BATTERY_MAX_VOLTAGE`) and remove hardcoded raw-count literals.
- Verify RP2350B ADC channel 3 → GPIO 43 mapping against datasheet and Pimoroni schematic.

