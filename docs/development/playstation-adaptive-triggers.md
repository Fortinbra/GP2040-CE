# PlayStation Adaptive Trigger Feedback

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C08, PS05. **Priority:** P1. **Milestone:** M3.

## Goal and Current Foundation

Provide independent programmable trigger resistance/effects comparable to a
standard DualSense. Existing analog trigger inputs measure position; they do not
generate force. Xbox impulse-trigger vibration belongs to
[haptics](controller-haptics.md), not this feature.

## Required Behavior

- Document left/right effect commands, parameters, modes, activation ranges,
  host intensity/disable controls, and stop/reset behavior from reference evidence.
- Select actuator, gearbox/linkage, position sensing, driver, mechanical travel,
  and protection suitable for a user-operated trigger. Define force/current/thermal
  limits and a mechanically safe unpowered state before enabling motion.
- Keep measured trigger input independent from requested resistance so feedback
  does not fabricate a press or change configured input calibration.
- Validate effect parameters and bound the control loop. Reject unsupported or
  unsafe requests instead of applying arbitrary motor values.
- Define safe stop/release on link loss, sleep, watchdog expiry, sensor failure,
  stalled actuator, or power fault. Never rely solely on the host sending stop.
- Preserve ordinary analog triggers and explicitly report absent adaptive hardware
  rather than advertising effects that cannot be produced.

## Integration and Dependencies

[PlayStation profiles](playstation-gamepad-parity.md) decode native effects;
[system routing](controller-output-and-system-controls.md) dispatches validated
requests. A hardware-specific output addon owns actuation and protection using
existing subsystem conventions. Coordinate ADC/pin/power budgets with
[analog input](gamepad-analog-input-parity.md) and lifecycle safe-stop hooks.
Implementation needs an accepted mechanical/electrical design, not only firmware.

## Acceptance

1. Compare both triggers independently and simultaneously against reference
   effects across travel and intensity settings, including disable and release.
2. Measure force/travel, response time, current, temperature, and sensor accuracy
   with a safe fixture and component-rated limits before user handling.
3. Test missing/failed sensors, stalled movement, packet loss, disconnect, sleep,
   brownout, and reboot. Confirm safe release/stop and no phantom analog inputs.
4. Preserve ordinary trigger use and other haptic channels; run affected clean
   builds and simultaneous-feature hardware tests.

## Open Decisions

Select obtainable mechanics/actuators, feedback sensors, safety interlocks, effect
coverage, and measurable force/timing tolerances. Licensing/authentication are
deferred; this specification does not assume access to proprietary hardware APIs.
