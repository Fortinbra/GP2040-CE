# pico-pio-usb Upstream Port

## Overview

GP2040-CE uses a fork of pico-pio-usb at version 0.5.3 with 10 custom commits on top, maintained by OpenStickCommunity and pinned to the `dev` branch. These patches implement Pico SDK 2 compatibility by removing obsolete SDK 1 APIs and adjusting host mode hooks. The upstream pico-pio-usb project is at 0.7.2 (2 minor versions ahead). This document outlines the effort to audit the custom patches, verify upstream SDK 2 compatibility, and migrate to the current upstream release.

**Status:** Planned — deferred during automated dependency audit (likely lower effort than TinyUSB port)

## Current State

| Item | Details |
|---|---|
| **Current version** | Fork of pico-pio-usb 0.5.3 |
| **Location** | `lib/pico_pio_usb` (git submodule) |
| **Branch** | `dev` (non-standard branch, not main) |
| **Custom commits** | 10 patches on top of 0.5.3 |
| **Key patches** | Pico SDK 2 compatibility fixes |
| **Upstream version** | 0.7.2 (2 minor versions ahead) |

### Custom Patches Summary

The 10 custom commits address Pico SDK 2 compatibility:

1. **Removed `pio_sm_set_jmp_pin()`** — This function was removed in Pico SDK 2.0.0 and later. The patches remove calls to this function and adjust PIO state machine initialization accordingly.

2. **Reverted Host Mode Hooks** — Changes to host mode callback handling that were necessary for SDK 2.2.0 integration but may have been partially addressed upstream.

3. **GPIO & PIO Configuration Adjustments** — Minor updates to ensure PIO programs compile and run correctly with SDK 2.x API changes.

The motivation for these patches is clear: pico-pio-usb 0.5.3 was released when Pico SDK was at 1.x. When GP2040-CE upgraded to SDK 2.2.0 (per `CMakeLists.txt`), these patches were necessary to bridge the gap.

## Goals

1. Verify that upstream pico-pio-usb 0.7.2 has already incorporated official Pico SDK 2 compatibility (very likely, given it is 2 minor versions ahead)
2. Audit the 10 custom commits to determine if any are still necessary after the SDK 2 upgrade in upstream 0.7.2
3. Compare the fork's `dev` branch against upstream's main branch (or current release branch)
4. If upstream 0.7.2 supports SDK 2.2.0 natively, verify compatibility with existing GP2040-CE board configurations
5. Update `.gitmodules` to point to upstream pico-pio-usb (if custom patches are no longer needed)
6. Verify USB host PIO operation on real hardware (if patches are required, rebase them)

## Approach & Phases

### Phase 1: Compatibility Assessment (1–2 days)
1. Review the upstream pico-pio-usb 0.7.x changelog and release notes for SDK 2 support mentions
2. Check the upstream repository for any issues or PRs related to SDK 2 migration
3. Compile a test project using upstream pico-pio-usb 0.7.2 with Pico SDK 2.2.0 to verify no API errors
4. List all 10 custom commits and cross-reference them against upstream 0.7.2 source code
5. Determine: Is SDK 2 support already built-in? (Expected: yes)

### Phase 2: Patch Audit (1 day)
1. If SDK 2 support is built-in upstream, verify no surviving patches are needed
2. If any patches are still required, audit for potential conflicts or superseded functionality
3. Document findings: patches needed (estimated: 0–2 of 10) vs. absorbed upstream

### Phase 3: Migration (1–2 days)
1. If no patches are needed: update `.gitmodules` to point to upstream pico-pio-usb, update `CMakeLists.txt` version to 0.7.2
2. If patches are needed: rebase onto upstream 0.7.2 following the same process as the TinyUSB port (see `docs/development/tinyusb-upstream-port.md`)
3. Rebuild GP2040-CE firmware with the updated dependency
4. Verify no link errors or new warnings

### Phase 4: Hardware Testing (1–2 days)
1. Flash firmware to RP2040 or RP2350 device
2. Test USB host mode with at least one controller connected via USB
3. If using wireless USB dongle: verify pairing and input recognition
4. Document any timing issues, latency changes, or deviations from current behavior
5. Test on both RP2040 (Pico) and RP2350 (Pico 2) if GPIO count differs

## Risks

**Risk Level: MEDIUM**

USB host functionality depends on this library. Risks are lower than the TinyUSB port because:

- **Likely Upstream Support:** Version 0.7.2 is 2 minor versions ahead of 0.5.3 and was likely released well after Pico SDK 2.0 was stable, increasing the probability of built-in SDK 2 support.
- **Simpler Patch Set:** 10 commits is fewer than TinyUSB's 18, and they are more narrowly scoped (SDK compatibility, not advanced HID features).
- **Fallback Path:** If issues arise, the custom fork can remain in use until upstream is fully proven.

**Specific concerns:**
- **PIO Program Changes:** If upstream 0.7.2 rewrote the PIO USB host program, timing or protocol behavior could shift subtly
- **API Changes:** If upstream's host mode hooks changed their signatures, any callbacks in GP2040-CE would need updates

**Mitigation:**
- Prioritize this port after TinyUSB (they interact, so TinyUSB changes may affect this library)
- Test on diverse hardware (both RP2040 and RP2350)

## Dependencies

- **TinyUSB upstream port:** Coordinate with the TinyUSB 0.20.0 port (see `docs/development/tinyusb-upstream-port.md`). Both libraries handle USB host mode; changes to one may expose incompatibilities in the other.
- **Pico SDK 2.2.0:** Project-wide minimum (enforced in CMakeLists.txt). Upstream 0.7.2 must support this version.

## Success Criteria

1. ✓ Compatibility assessment completed: upstream 0.7.2 SDK 2 support verified (or clearly lacking)
2. ✓ Patch audit finalized with clear decision: patches needed (0–2 of 10) or none needed
3. ✓ If zero patches needed: `.gitmodules` and version references updated to upstream 0.7.2
4. ✓ If patches needed: all surviving patches rebased onto 0.7.2 with conflicts resolved
5. ✓ GP2040-CE firmware compiles without errors
6. ✓ USB host mode functions correctly on hardware (device recognized and input works)
7. ✓ No performance degradation (PIO timing, latency) compared to current fork
8. ✓ Documentation updated in `docs/development/dependency-updates.md`
9. ✓ Pull request merged to main branch with test results

---

**Maintained by:** GP2040-CE core team  
**Last updated:** 2026-03-28  
**References:** `lib/pico_pio_usb`, `CMakeLists.txt`, `docs/development/dependency-updates.md`, `docs/development/tinyusb-upstream-port.md`
