# Gamepad Rumble, High-Definition Haptics, and Impulse Triggers

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C08, PS04, XB03, NS03. **Priority:** P1. **Milestone:** M3.

## Goal and Current Foundation

Deliver reference-equivalent physical feedback: PlayStation high-definition
haptics, Xbox body plus independent left/right impulse triggers, and Nintendo
HD rumble/HD rumble 2. Existing output handlers and
[DRV8833 rumble](../../src/addons/drv8833_rumble.cpp) provide a basic-actuator
foundation, not proof of waveform or four-channel parity.

## Required Behavior

- Inventory each protocol's effect encodings, channels, timing, gain, duration,
  update/stop behavior, and physical actuator requirements before designing a
  shared representation. Do not reduce richer effects to two amplitude values.
- Preserve independent Xbox body and trigger channels. Distinguish impulse
  vibration from PlayStation adaptive resistance, owned by its separate feature.
- Deliver required PlayStation and Nintendo waveform/frequency behavior with
  suitable actuators/drivers, calibration, and synchronized scheduling.
- Define host intensity/disable controls and safe local output limits. Prevent
  stale effects after disconnect, sleep, malformed traffic, or stream starvation.
- Bound buffers and producer/consumer rates with explicit underrun/overflow
  behavior. Input delivery must not block on actuator processing or output queues.
- Specify power/thermal/current limits, boot state, watchdog stop, bus/PWM/DMA
  allocation, and resource conflicts. Gate capabilities by installed hardware.
- Retain the basic rumble addon and fallback mappings for existing controllers,
  but label degraded output as compatibility rather than full haptic parity.

## Integration and Dependencies

Extend existing output/auxiliary-state and addon conventions only where they can
represent the required effects faithfully. Document any necessary richer channel
model instead of duplicating protocol decoding inside actuator drivers.
[System output routing](controller-output-and-system-controls.md) owns dispatch.
Coordinate waveform transport and buffering with audio when the native protocol
shares that path; this does not make game/headset audio the haptics implementation.

## Acceptance

1. Test independent channels, combined effects, effect replacement, gain/disable,
   timed stop, and continuous updates against each reference controller.
2. Capture electrical/mechanical output with appropriate instrumentation and
   repeatable game effects; protocol acknowledgements alone cannot pass.
3. Test worst-case simultaneous input/radio/sensor/audio activity, dropped commands,
   underrun, unplug, sleep, and brownout. Verify safe output and bounded latency.
4. Measure power/thermal behavior within component ratings; preserve existing
   rumble paths and run affected clean builds plus physical qualification.

## Open Decisions

Choose reference actuators/drivers, output metrics/tolerances, scheduling budgets,
effect representations, and shared audio-path dependencies. Verify HD rumble 2
requirements independently; no unverified waveform equivalence is assumed.
