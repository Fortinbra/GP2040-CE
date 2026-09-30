# PlayStation Gamepad Profiles and Native Wireless

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C01, C02, PS01, PS02. **Priority:** P0. **Milestones:** M0-M2.

## Goal and Boundaries

Provide a standard DualSense-style gamepad protocol on PS5 over USB and native
wireless while preserving existing PS3, PS4, and PS5 accessory modes. Licensing
and console authentication are deferred; preserve existing integrations and mark
tests requiring unavailable sessions blocked. Do not equate that deferral with
working console support. DualSense Edge extras are excluded.

## Current Foundation

[P5GeneralDriver](../../src/drivers/p5general/P5GeneralDriver.cpp) already has a
general PS5 report path, while [PS4Driver](../../src/drivers/ps4/PS4Driver.cpp)
also supplies a specialty-controller compatibility path. These are reuse points,
not evidence of full DualSense compatibility. [Bluetooth support](bluetooth-support.md)
is currently a generic digital BLE gamepad, not a native PS5 profile.

## Required Behavior

- Inventory wired and wireless initialization, descriptors, input/output/feature
  messages, session states, sequencing, timestamps, integrity checks, and timing.
  Do not assume the USB packet format can be sent unchanged over Bluetooth.
- Carry complete analog and button state, sensor/touch data, output commands, and
  status through the existing gamepad/auxiliary-state model and per-driver pattern.
- Define protocol adapters for system controls, haptics, triggers, audio, power,
  and pairing. Those features own their physical behavior; this feature owns
  encoding, decoding, negotiation, and transport delivery.
- Expose a distinct profile only when it is usable. Preserve explicit access to
  older compatibility modes, stored selections, and non-wireless builds.
- Bound malformed packet handling, retry queues, and output processing. Report
  unsupported commands honestly rather than inventing successful device behavior.

## Delivery and Dependencies

Reuse [Bluetooth controller architecture](bluetooth-controller-architecture.md)
for transport/profile separation. First document report fixtures and verify wired
state translation, then implement wireless framing and bidirectional delivery.
Coordinate pairing and lifecycle hooks with their shared specifications before
adding feature-specific output streams. A simulated peer can unblock development,
but cannot close console acceptance.

## Acceptance

1. Compare captures and behavior with a recorded standard DualSense firmware on
   PS5 menus and ordinary PS5 games, separately from specialty-controller games.
2. Verify full simultaneous input, output/feature traffic, reconnection, console
   resume, and long-session operation for USB and native wireless separately.
3. Test truncated packets, unknown commands, link loss, backpressure, and multiple
   controllers; confirm neutral recovery and no regression to input scheduling.
4. Preserve PS3/PS4/PS5 legacy profiles and generic BLE. Run affected clean builds
   and the roadmap's physical regression gates; label unavailable tests blocked.

## Open Decisions

Determine exact reference firmware, native transport framing, report coverage,
hardware/resource budgets, and session-access fixtures before implementation.
Publish protocol evidence without credentials. No authentication redesign is
authorized by this specification.
