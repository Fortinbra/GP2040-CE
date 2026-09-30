# PlayStation Two-Contact Touchpad

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C08, PS03. **Priority:** P1. **Milestone:** M3.

## Goal and Current Foundation

Provide a real two-contact touch surface and physical click for standard
PlayStation gamepad behavior. Existing
[PS4](../../src/drivers/ps4/PS4Driver.cpp) and
[P5General](../../src/drivers/p5general/P5GeneralDriver.cpp) drivers pack touchpad
state. Centered/default coordinates or a touchpad-click shortcut do not supply
the physical two-contact experience.

## Required Behavior

- Acquire at least two simultaneous contacts with independent identity, position,
  touch/release state, and required report timing from documented touch hardware.
- Define dimensions, coordinate transforms, mounting orientation, clipping,
  calibration, and contact-ID reuse/wrap semantics for each PlayStation profile.
- Preserve contact continuity when fingers cross or lift independently. Report
  all releases correctly after bus failure, profile switch, disconnect, or wake.
- Handle mechanical click independently of contacts; clicking must not require
  fabricating a center touch. Preserve existing mapped-click configurations.
- Transmit contact trajectories that allow games to interpret taps, swipes, and
  multi-touch gestures. Do not substitute generic gesture buttons for coordinates.
- Keep touch acquisition and debounce bounded under radio, display, and IMU load;
  handle missing hardware and startup calibration without spurious touches.

## Integration and Dependencies

Reuse auxiliary touch state and existing report packing; add a physical input
addon only if no suitable acquisition path exists. Coordinate click/system-button
mapping with [system controls](controller-output-and-system-controls.md), and
timing with [PlayStation profiles](playstation-gamepad-parity.md). USB-host mapped
mouse input can remain a convenience but is not a two-contact reference device.

## Acceptance

1. Sweep edges/corners, tap, drag, cross two contacts, independently lift fingers,
   and click with zero/one/two contacts. Inspect reports and game behavior.
2. Compare coordinate range, tracking stability, gestures, and latency with a
   standard reference controller over wired and wireless paths.
3. Test bus interruption, malformed device data, held contact through sleep/wake,
   ID rollover, settings changes, and transport handover for stuck touches.
4. Preserve existing touchpad-click shortcuts and non-touch builds; perform clean
   builds and physical touchpad/game tests before qualification.

## Open Decisions

Select obtainable touch hardware, electrical interface, surface dimensions,
mechanical click design, calibration needs, and acceptable tracking/latency bounds.
Console authentication/licensing remain deferred under the roadmap policy.
