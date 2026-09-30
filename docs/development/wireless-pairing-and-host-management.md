# Wireless Pairing, Identity, and Host Management

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C05; supports PS02, XB04, NS05. **Priority:** P0. **Milestones:** M1-M2.

## Goal and Current Foundation

Provide reliable pairing, persistent identity, reconnect, and deliberate host
selection for each gamepad transport. [Bluetooth support](bluetooth-support.md)
already documents BLE bonds, pairing controls, and status APIs. Extend that
working baseline, not the abandoned Classic implementation as if it were qualified.
Console authentication/licensing are deferred; ordinary secure pairing is in scope.

## Required Behavior

- Model unpaired, discoverable, connecting, connected, reconnecting, and error
  states with bounded retries and clear operator-visible status.
- Persist stable device identity and transport/profile-specific host records.
  Separate Classic, LE, and any external radio's identities and credentials.
- Define explicit enter/exit pairing, select host, forget one host, and forget all
  operations. Require deliberate action for destructive operations; never erase
  all bonds as an automatic response to one failed connection.
- Recover stale or mismatched host records without infinite reconnect loops.
  Reconnect to the intended host, not an arbitrary nearby remembered host.
- Match reference-supported host switching without assuming simultaneous multi-host
  reporting. Define record capacity, eviction policy, and UI behavior when full.
- Preserve bonds and identities across normal settings saves and firmware upgrades;
  distinguish controller factory reset, network reset, and public preset import.
- Protect private records from logs and portable presets. Document re-pair needs
  when a profile's report descriptor changes or a host caches stale metadata.

## Integration and Dependencies

Follow [Bluetooth architecture](bluetooth-controller-architecture.md); console
profiles supply transport-specific connection operations. Reuse existing BLE
storage and control endpoints where semantics match. Coordinate handover/sleep
with [power lifecycle](controller-power-lifecycle.md), which owns active transport
selection and neutralization, not bond persistence. No new universal bond format
should be assumed before existing storage and radio APIs are inventoried.

## Acceptance

1. Test fresh pair, reboot/reconnect, host reboot, stale bonds, pairing cancellation,
   host switching, full record capacity, and multiple nearby controllers/hosts.
2. Interrupt persistence updates and firmware upgrades; verify recovery preserves
   valid identities and does not connect to an unintended host.
3. Check private-state exclusion from export/logs and correct WebUI/local status.
   Verify pairing remains accessible without a working host connection.
4. Preserve generic BLE behavior; run affected clean builds and transport-specific
   hardware tests. Record session-dependent console tests as blocked if unavailable.

## Open Decisions

Specify host-slot capacity, persistence bounds, physical control gestures, retry
budgets, and reference-supported switching semantics per console before signoff.
