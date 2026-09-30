# Nintendo Pro Gamepad Profiles and Native Wireless

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C01, C02, NS01. **Priority:** P0. **Milestones:** M0-M2.

## Goal and Boundaries

Support original Switch Pro and Switch 2 Pro gamepad behavior as independently
qualified profiles. Preserve existing Switch compatibility. Joy-Con identities,
split operation, mouse sensing, attachment, and specialty accessories are excluded.
Licensing and console authentication are deferred under the roadmap's policy.

## Current Foundation

[SwitchProDriver](../../src/drivers/switchpro/SwitchProDriver.cpp) implements
wired inputs and protocol handling, with IMU data initialized to zero. Existing
report fields and handshakes do not demonstrate real motion, HD rumble, NFC, or
Switch 2 Pro compatibility. Generic BLE is not a native Nintendo profile.

## Required Behavior

- Document per-generation identification, USB and wireless initialization,
  input modes, feature/subcommands, sequencing, report cadence, and error behavior.
  Do not infer Switch 2 Pro packets from original Switch compatibility alone.
- Preserve full sticks, digital shoulder/trigger semantics, system buttons,
  calibration data, device identity, and host-requested operating modes.
- Provide generation-aware adapters for motion, rumble, NFC, battery, indicators,
  and Switch 2 Pro rear/C buttons and audio. Physical functions belong to the
  corresponding shared feature specifications.
- Define stable calibration/identity persistence and bounded emulation of
  protocol-visible storage; never expose arbitrary firmware flash through reads.
- Gate profiles and commands by actual hardware. Missing NFC, motion, or audio
  hardware must not be hidden behind synthetic success responses.

## Delivery and Dependencies

Start with accessible official specifications, reference-controller captures, and
an original Switch Pro wired regression baseline. Specify Switch 2 Pro differences
before implementing its profile. Reuse
[Bluetooth controller architecture](bluetooth-controller-architecture.md) for
wireless separation and existing USB drivers for wired integration. Pairing,
power, calibration, feedback, NFC, and audio are coordinated workstreams, not
duplicate implementations in this driver.

## Acceptance

1. Qualify original Switch Pro behavior and Switch 2 Pro behavior separately on
   recorded console/controller revisions, including menu and game operation.
2. Test handshake retries, report-mode changes, storage bounds, unknown/truncated
   commands, reconnection, and simultaneous input/output on each transport.
3. Verify motion, feedback, NFC, controls, and applicable audio using their feature
   tests; an older controller accepted by Switch 2 is not a full-parity result.
4. Preserve existing Switch modes and generic BLE; complete affected clean builds
   and the physical regression matrix. Record unavailable sessions as blocked.

## Open Decisions

Switch 2 Pro specifications and exact USB/wireless feature availability need
reference-hardware confirmation; the initial roadmap could not retrieve official
Nintendo pages. Record verified requirements before selecting report layouts or
hardware. No Joy-Con feature is added by inference.
