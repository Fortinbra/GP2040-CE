# Gamepad Parity Qualification and Regression Preservation

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C11; validates all active roadmap IDs. **Priority:** P0. **Milestones:** M0-M5.

## Goal and Current Foundation

Make parity and preservation claims reproducible across real hardware and console
versions. Reuse the [latency testing framework](latency-testing-framework.md) for
capture and analysis rather than inventing another latency rig. Its historical
timing estimates and source references must be revalidated before use as limits.
This feature adds controller qualification, not game/display latency attribution.

## Required Evidence

- Freeze a versioned inventory of all existing modes, boards, settings, addons,
  authentication integrations, recovery paths, and WebUI behavior. Preserve their
  established constraints rather than promising every possible addon combination.
- Record each reference model/firmware, console/version, GP2040 commit/build,
  hardware BOM/wiring, peripherals, transport, configuration, test procedure,
  measurement equipment, raw results, and exercised requirement IDs.
- Mark results pass, fail, blocked, or not applicable with reasons. Separate
  simulation, packet tests, clean build results, and real console/hardware evidence.
- Set numerical latency/jitter/loss, range/interference, reconnect/wake, runtime,
  sensor, feedback, audio, and endurance thresholds before accepting each feature.
  Compare equivalent modes/workloads with recorded first-party references.
- Test multiple controllers, RF coexistence, output bursts, and worst-case supported
  concurrent features. Preserve existing wired targets without assuming wireless
  can universally provide sub-millisecond input latency.
- Test migrations, persistence interruption, recovery, missing peripherals,
  malformed traffic, safe actuator/audio shutdown, and privacy boundaries.
- Publish at least one reproducible parity-capable configuration per console,
  clearly distinguished from compatible but physically reduced legacy builds.

## Delivery and Dependencies

First establish current measurements and the preservation inventory. Add tests
alongside each feature rather than leaving all hardware work to M5. Use existing
repository test/build conventions; extend the latency framework only with its
documented ownership intact. Store reviewed procedures and result artifacts with
versioned provenance; choose the exact schema/location before implementation.

Licensing and console-authentication implementation are deferred. Preserve current
paths and mark inaccessible native tests blocked, not passed through a simulated
peer. No authentication work item is introduced by this qualification document.
Joy-Con/specialty-controller parity is excluded; existing addon behavior stays
in the preservation inventory.

## Acceptance

1. Every active roadmap ID has an owning feature and versioned results; NS08 is
   explicitly out of scope and cannot generate an unrequested implementation task.
2. Every inventoried existing feature passes its regression gate or has an explicit
   unresolved defect; new parity features cannot silently replace old capabilities.
3. Run fresh Ninja configure/full builds for relevant registered targets, including
   Standard Pico and required web-inclusive paths. Record build and hardware
   results separately, retaining warnings/failures rather than hiding them.
4. Repeat measurements with controlled fixtures and publish sufficient raw data
   for independent reproduction. Block full-parity claims while mandatory evidence
   is missing, including tests blocked on a deferred external prerequisite.

## Open Decisions

Choose reference units, console versions, numerical tolerances, endurance durations,
test fixture extensions, result storage, and release requalification cadence.
Assign implementation/test owners when work is scheduled; this draft supplies
document ownership without inventing team assignments or completion dates.
