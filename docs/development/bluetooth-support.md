# Bluetooth Support (Consolidated)

**Last updated:** 2026-05-17  
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

