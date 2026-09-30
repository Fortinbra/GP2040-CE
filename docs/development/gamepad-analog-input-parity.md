# Complete Gamepad Inputs and Analog Calibration

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C03. **Priority:** P0. **Milestones:** M1-M3.

## Goal and Current Foundation

Carry complete physical gamepad inputs through every supported parity profile:
two independent sticks and clicks, face/shoulder buttons, D-pad, and analog
triggers where the reference has them. Nintendo Pro triggers remain digital.
Reuse [analog input](../../src/addons/analog.cpp), ADC addons, and
[Hall-effect trigger input](../../src/addons/he_trigger.cpp) where suitable.
The current [BLE path](../../src/OutputManager.cpp) intentionally reports a
digital-only gamepad; adding axes needs an explicit compatible profile transition.

## Required Behavior

- Define source ranges, signedness, center, orientation, trigger rest/full scale,
  clipping, resolution, and per-profile quantization without axis cross-coupling.
- Acquire all controls concurrently with bounded sampling/filtering latency.
  Specify arbitration when GPIO, external ADC, USB-host, or other addons compete
  for one logical control, preserving existing defaults and digital emulation.
- Provide calibration for center/endpoints, inversion, deadzones, and supported
  response curves. Keep sensor noise filtering distinct from user deadzones.
- Reject invalid or disconnected sources safely. Do not report arbitrary full
  scale at startup, after calibration failure, or when a peripheral disappears.
- Persist versioned calibration per physical source with deliberate reset/export
  policy. Do not silently overwrite a user's calibration when switching profiles.
- Preserve simultaneous button combinations, SOCD, dual-direction, turbo/macros,
  profile shortcuts, and existing trigger behavior where configured.

## Integration and Delivery

Use existing gamepad state, addon acquisition, configuration APIs, and per-driver
packing. Inventory today's analog paths before adding another abstraction.
Introduce any richer BLE report as an explicit profile/version with migration
and host re-pair guidance; keep the current digital profile working.
Coordinate system-button extensions with the system-control feature rather than
inventing generic button numbers for console-specific commands.

## Acceptance

1. Record calibrated center, full travel, diagonals, endpoints, resolution, and
   noise with both sticks and triggers moving simultaneously on reference hardware.
2. Compare USB and wireless values against the same physical input; check negative
   axes, inversion, saturation, independent triggers, and all button combinations.
3. Exercise missing sensors, invalid calibration, cold boot, save/reboot, settings
   migration, and switching between digital and richer BLE profiles.
4. Measure added sampling/filter latency and preserve existing input/addon modes.
   Complete relevant clean builds and physical tests, not just packet snapshots.

## Open Decisions

Choose reference ADC/sensor hardware, effective resolution, sampling/filter
budgets, calibration UI, and acceptable center/noise tolerances before signoff.
Additional hardware must remain optional for existing digital-only controllers.
