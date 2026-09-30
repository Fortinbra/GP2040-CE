# Xbox Gamepad Profiles and Native Wireless

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C01, C02, XB01, XB02. **Priority:** P0. **Milestones:** M0-M2.

## Goal and Boundaries

Provide standard Xbox Wireless Controller behavior on Series X|S and Xbox One,
including native Xbox Wireless. Licensing and console authentication are deferred;
preserve existing integrations and record blocked session-dependent tests.
Bluetooth PC/mobile operation is a separate compatibility path, not console
wireless parity. Elite-specific features are excluded.

## Current Foundation

[XBOneDriver](../../src/drivers/xbone/XBOneDriver.cpp) supplies an existing wired
protocol and rumble handling. Existing XInput and original Xbox drivers must
remain available. [Bluetooth support](bluetooth-support.md) does not establish
an Xbox Wireless radio implementation.

## Required Behavior

- Inventory wired message framing, initialization, input, output, feature/status
  commands, sequencing, acknowledgements, and device lifecycle independently of
  deferred console-authentication internals.
- Establish the radio/module, firmware interface, pairing primitives, bus wiring,
  bandwidth, latency, and power requirements for native Xbox Wireless. CYW43 BLE
  or a PC receiver's existence is not evidence of a usable controller-side radio.
- Preserve distinct console and Bluetooth host modes. Delegate host selection to
  the shared pairing feature, with explicit transport capabilities and identities.
- Carry ordinary analog controls, Share/system controls, body/impulse feedback,
  battery status, and audio/accessory messages without lossy generic-HID translation.
- Keep input delivery responsive during acknowledgements, output bursts, module
  restart, and link loss. Define bounded queues and deterministic recovery.

## Delivery and Dependencies

First produce a technical radio feasibility report and a wired protocol inventory.
Then prove bidirectional controller-side transport with a test fixture before
integrating a console profile. Reuse existing driver/state conventions; if an
external radio needs a new adapter, document that boundary and resource ownership
instead of forcing a proprietary transport through a Bluetooth-specific API.
Coordinate with [board selection](unified-board-selection.md), shared pairing,
power, feedback, audio, and capability specifications.

## Acceptance

1. Exercise standard input, system menus, and ordinary games on recorded Xbox One
   and Series X|S versions where a valid session is available.
2. Demonstrate direct console wireless traffic and distinguish it from converter
   or PC Bluetooth tests in every result and support claim.
3. Test packet loss, radio reset, multiple controllers, reconnect, USB insertion,
   and simultaneous feedback/audio without stuck controls or unbounded buffering.
4. Preserve wired Xbox One, Xbox 360/XInput, original Xbox, and generic BLE
   behavior. Complete affected clean builds and hardware regression qualification.

## Open Decisions

Controller-side radio availability and its technical interface remain research
gates. Record measured transport limits and exact supported controller revisions.
If no viable hardware path exists, mark this feature blocked, not satisfied by
Bluetooth. Authentication/licensing remain a separate deferred concern.
