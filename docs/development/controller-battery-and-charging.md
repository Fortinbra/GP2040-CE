# Controller Battery Reporting and Safe Charging

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C07; supports PS02, XB04, NS05. **Priority:** P1. **Milestones:** M1-M3.

## Goal and Current Foundation

Provide trustworthy battery/charging status, charge-and-play, and safe low-power
behavior on documented hardware. [Bluetooth support](bluetooth-support.md)
includes board-specific battery reporting, but that does not qualify arbitrary
battery wiring, chemistry, charge status, or runtime accuracy.

## Required Behavior

- Describe battery chemistry, cell count, voltage/divider limits, ADC/reference
  calibration, charger, protection, power-path circuitry, and available status pins
  per supported configuration. Firmware cannot replace electrical protection.
- Distinguish absent, unknown, discharging, charging, full, and fault states using
  actual hardware evidence. USB power alone is not proof a battery is charging.
- Estimate percentage with appropriate voltage/load or fuel-gauge information,
  bounded filtering, and calibration; avoid misleading fixed-full readings.
- Map status and low-battery alerts into each console's protocol and local UI.
  Specify thresholds/hysteresis and hand low-power shutdown requests to lifecycle.
- Support safe charge-and-play without brownouts or invalid source switching.
  Replaceable primary cells must never be charged; detect or explicitly configure
  supported pack types rather than assuming every Xbox-style supply is rechargeable.
- Version electrical settings and reject unavailable ADC/status pins or unsafe
  overrides. Preserve compile-time board defaults until runtime equivalents are
  validated; portable presets must not overwrite private measurement calibration.

## Integration and Dependencies

Use [board selection](unified-board-selection.md) as the hardware identity source
and reuse current battery acquisition where suitable. Coordinate settings with
[provisioning](post-build-controller-provisioning.md) if adopted, without making
that proposal a prerequisite. [Power lifecycle](controller-power-lifecycle.md)
owns sleep/shutdown actions; profiles own status encoding.

## Acceptance

1. Measure reported voltage/percentage and charging flags against instruments
   through discharge, load changes, recharge, full charge, and battery absence.
2. Test radio/audio/actuator peak loads, charge-and-play, unplugging supplies, and
   low-voltage recovery on electrically reviewed hardware.
3. Compare runtime/standby with reference gamepads in the same workloads; agree
   accuracy and runtime thresholds before acceptance rather than after measurement.
4. Verify migrations, reset/calibration behavior, local/host displays, non-battery
   boards, and clean builds. Follow safe battery test procedures, not deliberate
   unprotected overcharge or short-circuit experiments.

## Open Decisions

Choose reference packs, charger/protection/power-path hardware, fuel-gauge strategy,
calibration method, allowed settings, and error budgets. No new flash layout or
Pimoroni flash-size change is authorized by this feature.
