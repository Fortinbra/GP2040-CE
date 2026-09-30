# Gamepad Motion Sensors and Calibration

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C08, PS03, NS02. **Priority:** P1. **Milestone:** M3.

## Goal and Current Foundation

Deliver real calibrated accelerometer and gyroscope data for PlayStation and
Nintendo Pro profiles. The standard Xbox reference does not require motion.
[P5GeneralDriver](../../src/drivers/p5general/P5GeneralDriver.cpp) and other
drivers already consume auxiliary sensor fields; the Nintendo Pro report's zeroed
IMU data is not motion support. Audit existing acquisition before adding a driver.

## Required Behavior

- Select supported six-axis IMU hardware and document electrical limits, bus,
  address/pins, mounting orientation, units, range, resolution, and sample timing.
- Acquire gyro and acceleration with consistent timestamps, bounded buffering,
  and explicit overrun handling. Match each protocol's batching/cadence without
  presenting repeated or missing samples as newly measured data.
- Apply mounting transforms, scale, bias, and calibration in a common physical
  representation; keep protocol-specific units/packing in the console profile.
- Support required calibration queries and console calibration workflows using
  real device data. Distinguish factory calibration, user zeroing, and drift
  correction; define reset, persistence, and invalid-calibration behavior.
- Report sensor failure safely and expose diagnostics without flooding logs or
  blocking input. Do not claim a present working sensor when hardware is absent.
- Preserve legacy motion mappings and existing addon behavior. Motion transport
  is not permission to replace the game's own sensor fusion with an invented model.

## Integration and Dependencies

Use existing sensor/auxiliary state, addons, board capabilities, and configuration
storage. Coordinate bus ownership with analog inputs, displays, and other addons.
The [console profiles](roadmap-to-1.0.md) own on-wire reports; this feature owns
acquisition, physical transforms, calibration, and sensor health. Touchpad
coordinates remain a separate feature despite sharing PS03.

## Acceptance

1. Use stationary, known-orientation, and repeatable rotation tests to check axes,
   sign, scaling, timing, bias, noise, and drift against the selected reference.
2. Exercise real motion-dependent games and calibration menus on PlayStation and
   both Nintendo Pro generations over their supported transports.
3. Test bus contention, missing sensors, sample overrun, sleep/wake, temperature
   variation, save/reboot, and calibration migration with bounded recovery.
4. Measure CPU/bus/input-latency impact and preserve non-motion configurations;
   complete affected clean builds and hardware qualification.

## Open Decisions

Choose IMU models, mounting conventions, sample/range requirements, timestamp
mapping, calibration fixture, and numerical noise/drift tolerances before signoff.
Do not assume original Switch sampling requirements apply unchanged to Switch 2.
