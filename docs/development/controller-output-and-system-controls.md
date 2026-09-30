# Controller System Controls and Host Output Routing

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C04, PS06, XB04, NS05, NS06. **Priority:** P1. **Milestones:** M1-M3.

## Goal and Current Foundation

Complete console-specific system controls, indicators, and host-command routing
without conflating them with ordinary gameplay buttons. Existing console drivers,
[player LEDs](../../src/addons/playerleds.cpp), and gamepad auxiliary state are
reuse points. Presence of a host command parser does not prove a physical effect.

## Required Behavior

- Support PlayStation Create/Options/PS/touchpad-click/mute, Xbox Share/View/Menu/
  Xbox, Nintendo Home/Capture/Plus/Minus, and Switch 2 Pro C and GL/GR semantics.
- Verify short/long presses and console-driven assignments where applicable.
  Distinguish rear-button local mappings from the console's assignment protocol;
  avoid promising separately addressable game buttons without evidence.
- Route player assignment, connection/status indicators, host colors/brightness,
  mute indicators, and reference-supported locate-controller commands correctly.
- Define precedence between host-controlled status and user RGB/player LED
  profiles. Preserve existing behavior outside parity profiles; privacy/safety
  indicators must not be silently hidden by cosmetic effects.
- Validate command length, mode, range, and sequencing before dispatch. Use bounded
  work queues and per-feature capability checks; do not block input for output.
- Separate transient host state from saved user settings, and reset stale host
  effects on disconnect. Do not write flash for every LED or feedback command.

## Integration and Dependencies

Use per-console decoders and shared auxiliary state/addon outputs, extending
[Bluetooth output channels](bluetooth-controller-architecture.md) consistently.
This feature owns dispatch and system semantics; haptics, adaptive triggers, audio,
pairing, battery, and lifecycle own their physical behavior and persistence.
Console GameChat and capture services are not implemented inside the controller.
Licensing/authentication are deferred without removing existing paths.

## Acceptance

1. Test all listed controls in console UI and games, including combined inputs,
   long presses, profile changes, and persisted rear-button assignments where supported.
2. Verify player changes, colors, mute status, and locate behavior against each
   reference; test hardware with missing or differently wired indicators.
3. Stress unknown/truncated/repeated commands and simultaneous output features;
   measure input timing and verify no unnecessary flash writes or stale effects.
4. Preserve existing LED, hotkey, remapping, and addon behavior outside new profiles.
   Run affected clean builds and physical console tests where sessions are available.

## Open Decisions

Verify Switch 2 Pro C/GL/GR configuration and indicator semantics on reference
hardware. Specify host/local precedence, supported control gestures, dispatch
budgets, and clear UI for unavailable physical capabilities before signoff.
