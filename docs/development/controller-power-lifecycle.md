# Controller Power Lifecycle and Transport Handover

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C06; supports PS02, XB04, NS05. **Priority:** P1. **Milestones:** M1-M3.

## Goal and Current Foundation

Provide reference-equivalent sleep, wake, shutdown, and USB/wireless transitions.
[Bluetooth support](bluetooth-support.md) describes advertising/active/idle states;
that is not proof of deep sleep, console wake, or seamless transport handover.
Existing USB operation and boot access to configuration must remain reliable.

## Required Behavior

- Define a state-transition table for boot, disconnected, pairing, active, idle,
  sleep, charging-only, low-battery shutdown, and fault recovery. Distinguish
  controller sleep from console sleep and console soft-off from full power removal.
- Specify which physical controls wake the controller and which reference-supported
  console states can be remotely woken. Do not advertise unsupported console wake.
- Quiesce input/output streams, stop actuators, mute audio, and handle pending
  persistence safely before sleep or shutdown. Wake without phantom button presses.
- Set deterministic USB/wireless priority and handover policy. Release controls on
  the old session, initialize the new session, and avoid duplicate controllers.
- Distinguish data-capable USB from charge-only power; adding a charging cable
  must not unnecessarily drop wireless play or enter configuration mode.
- Preserve deliberate boot overrides, web configuration/recovery, and USB-only
  operation. Avoid blocking radio/USB services while awaiting a transition.

## Integration and Dependencies

Coordinate mode policy through the existing output dispatch and per-transport
hooks, extending the accepted [Bluetooth architecture](bluetooth-controller-architecture.md).
[Pairing](wireless-pairing-and-host-management.md) owns host records;
[battery/charging](controller-battery-and-charging.md) owns measurements and power
hardware status. This feature consumes those facts and owns lifecycle decisions.
Specify radio wake sources and retained memory before choosing SDK sleep modes.

## Acceptance

1. Exercise every state transition with and without USB data, a charger, a valid
   bond, console power, and low battery. Compare wake behavior with the reference.
2. Hold inputs during disconnect/handover; verify no stuck controls, duplicate
   devices, unintended pairing, or unexpected mode changes after reconnect.
3. Test sleep during feedback/audio/settings activity and rapid repeated USB
   hotplug; measure wake time, standby current, and input loss on resume.
4. Confirm physical recovery/config entry and non-wireless builds still work.
   Run clean builds and board-level power tests; simulation alone is insufficient.

## Open Decisions

Select supported sleep modes, idle timers, wake sources, handover guarantees,
shutdown persistence policy, and quantitative wake/standby targets for each board.
Console-authentication changes are outside this feature's scope.
