# Nintendo Pro Controller NFC Reader

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** NS04. **Priority:** P1. **Milestone:** M3.

## Goal and Current Foundation

Provide physical NFC reader behavior for supported amiibo interactions on
Nintendo Pro gamepad profiles. Existing
[Switch Pro protocol handling](../../src/drivers/switchpro/SwitchProDriver.cpp)
is a reuse point, not evidence of an end-to-end NFC reader. Verify original and
Switch 2 Pro requirements separately before selecting hardware.

## Required Behavior

- Specify reader front-end, antenna, bus, electrical limits, placement, and
  coexistence with the radio, battery, and enclosure. A protocol field is not a reader.
- Implement the required discover/start/stop, tag presence/removal, command,
  response, and error transitions with bounded timeouts and packet sizes.
- Relay the permitted read/write operations needed by supported physical tags
  and games, including write completion/failure semantics. Preserve data integrity
  if the tag is removed or power/link is lost; never claim a failed write succeeded.
- Keep NFC polling and transfers from blocking gamepad input, motion, or feedback.
  Cancel safely on sleep, profile change, console cancellation, or reader failure.
- Expose accurate reader capability and status. Missing hardware must not appear
  to be a working reader with a permanently absent tag.
- Keep tag content and unique identifiers out of routine logs, public presets,
  and firmware releases. Tag emulation, cloning, and distributing proprietary
  content are outside scope; console authentication/licensing are deferred.

## Integration and Dependencies

[Nintendo Pro profiles](nintendo-pro-gamepad-parity.md) own console packets;
a hardware addon owns reader transactions. Reuse board/peripheral validation and
the existing bounded output-dispatch pattern. Define buffers, bus ownership, and
power requirements before committing to a reader library or hardware module.

## Acceptance

1. Test supported physical tags and games on both reference profiles, covering
   discovery, repeated scans, removal/reinsertion, and supported save/write flows.
2. Compare reference behavior for absent, unsupported, and prematurely removed
   tags, reader faults, cancellation, and console disconnect.
3. Measure scan reliability and input timing with simultaneous wireless, motion,
   and feedback; test antenna placement in the intended controller enclosure.
4. Verify no tag/private data leaks to export/logs, preserve non-NFC builds, and
   complete affected clean builds plus hardware qualification.

## Open Decisions

Confirm per-generation NFC commands, supported tag operations, reference fixtures,
reader/antenna availability, dependency suitability, and timing/resource budgets.
Unverified Switch 2 behavior remains a research item, not an implementation fact.
