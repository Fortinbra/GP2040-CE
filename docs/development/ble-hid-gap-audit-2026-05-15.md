# BLE HID Gap Audit (PC + Android)

Date: 2026-05-15

## Scope

This audit reviews current BLE HID readiness for connecting GP2040-CE to:

- Windows 10/11 PCs
- Android phones/tablets

Focus areas:

- Runtime mode selection and BLE initialization
- Advertising, pairing, security, and report delivery
- Bond persistence and stale-bond recovery
- WebConfig/operator control coverage

## Current State

### Implemented and Working in Firmware

- BLE mode dispatch path is integrated (`INPUT_MODE_BLE` via `GP2040::run`).
- BLE HID manager initializes CYW43, ATT/GATT, HIDS, and SM.
- Security uses bonding + LE Secure Connections.
- Report pipeline from `GamepadState` -> BLE HID input report is implemented.
- Bond storage is backed by protobuf config (`bleConfig`) via custom `le_device_db` backend.
- Disconnect/re-advertise and stale-bond mitigation logic are present.

### Gaps Identified

1. Missing operator controls for BLE from WebConfig:
	- No endpoint/UI for live pairing-mode control.
	- No endpoint/UI for clearing stored BLE bonds.
2. Visibility gap:
	- No API that reports BLE status (connected/notifying/bond count) for diagnostics.
3. Documentation drift:
	- Planning docs still imply not implemented while firmware now contains substantial BLE HID implementation.

## Work Started in This Session

### Backend API Added (Gap Closure Start)

Added two WebConfig endpoints in `src/webconfig.cpp`:

- `/api/getBLEHIDStatus`
  - Returns: `supported`, `enabled`, `connected`, `notifying`, `hasBondedPeers`, `bondCount`.
- `/api/setBLEHIDControls`
  - Inputs:
	 - `pairingMode` (bool): enables/disables BLE pairing mode at runtime.
	 - `clearBonds` (bool): clears all stored BLE bonds.
  - Returns same status payload as `getBLEHIDStatus`.

Implementation notes:

- Endpoints are compiled behind `ENABLE_BLUETOOTH`.
- Non-BLE builds return `supported=false` payloads.
- Bond clear uses `le_device_db_remove()` across `le_device_db_max_count()` slots.

## Remaining Work (Next Steps)

1. Web UI wiring:
	- Add BLE status panel + controls to settings page.
	- Add explicit warnings/UX around clearing bonds.
2. Persisted BLE policy:
	- If desired, add persisted pairing-mode default in config schema.
3. Cross-host validation matrix:
	- Fresh pair + reconnect + power cycle tests on Windows and Android.
	- Verify button/axis mapping parity and idle reconnect behavior.
4. Documentation alignment:
	- Update BLE support docs from planning status to implementation status.

## Exit Criteria for "usable BLE HID"

- User can select BLE mode and pair from WebConfig/workflow docs.
- User can inspect BLE connection state and bond count.
- User can clear stale bonds without reflashing.
- Verified reconnect after reboot on both Windows and Android.
