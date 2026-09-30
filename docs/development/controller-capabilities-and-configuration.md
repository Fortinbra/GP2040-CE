# Controller Capabilities, Configuration, and Recovery

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C10. **Priority:** P0. **Milestones:** M1-M5.

## Goal and Current Foundation

Make richer gamepad features configurable and recoverable without invalid hardware
combinations or loss of existing behavior. Reuse
[unified board selection](unified-board-selection.md), current protobuf settings,
embedded WebUI, migrations, and addons. Do not introduce another board registry,
settings engine, or provisioning contract.

## Required Behavior

- Distinguish compiled capability, configured physical hardware, detected health,
  and active protocol capability. A checkbox cannot create a radio or microphone.
- Validate pin/bus/ADC, DMA, endpoint, memory, and power conflicts device-side;
  show actionable reasons for unsupported combinations in the WebUI.
- Preserve stored input modes, pin maps, addons, profiles, and boot overrides.
  Add versioned settings/defaults/migrations for new hardware without silently
  enabling outputs or changing existing controller behavior.
- Keep private bonds, credentials, identity, and physical calibration separate
  from portable settings. Specify reset/export/import ownership for each new field.
- Define safe activation of hardware-setting changes, including reboot-required
  settings, output shutdown, invalid imports, and interrupted persistence.
- Provide a reliable recovery/configuration entry path independent of user pin
  mappings, functioning wireless, or new peripherals. Document rollback limits;
  never claim arbitrary older firmware can interpret newer settings safely.
- Publish truthful capability/support information per build/profile. Unsupported
  required features cannot be advertised as implemented just to satisfy a handshake.

## Integration and Dependencies

Each feature owns its field semantics and hardware requirements; this workstream
owns the shared validation, lifecycle, UI, and migration contract. Reuse
[post-build provisioning](post-build-controller-provisioning.md) only if that
separate feature ships. Larger storage depends on
[FlashPROM qualification](flashprom-large-flash-support.md); preserve existing
layout and Pimoroni 4 MiB limits until explicitly approved and validated.

## Acceptance

1. Upgrade representative existing configurations and verify all existing feature
   settings, bonds, identities, and calibration retain their intended behavior.
2. Reject conflicting pins/peripherals, absent hardware, malformed imports, and
   unsupported profile combinations without partial activation or driven outputs.
3. Exercise power interruption, reset variants, documented rollback, missing
   peripherals, and corrupted settings; verify deterministic recovery access.
4. Check API/WebUI agreement, private-field exclusion, and non-wireless/low-resource
   targets. Complete clean firmware and web-inclusive builds plus hardware tests.

## Open Decisions

Inventory present validation/migration behavior before selecting an extension.
Define the capability vocabulary, resource budgets, field ownership, transaction
and recovery guarantees, and supported upgrade/rollback matrix before signoff.
Console authentication redesign remains outside this feature.
