# TinyUSB Upstream Port

## Overview

GP2040-CE currently uses a fork of TinyUSB 0.17.0 with 18 custom commits on top, maintained by OpenStickCommunity. These patches implement critical HID host functionality and parameter overrides essential to gamepad operation. The upstream TinyUSB project is at 0.20.0, three major versions ahead. This document outlines the effort required to audit the custom patches, rebase them onto upstream, and migrate GP2040-CE to the current TinyUSB release.

**Status:** Planned — deferred during automated dependency audit (requires significant manual effort)

## Current State

| Item | Details |
|---|---|
| **Current version** | Fork of TinyUSB 0.17.0 |
| **Location** | `lib/tinyusb` (git submodule) |
| **Custom commits** | 18 patches on top of 0.17.0 |
| **Key patches** | HID host support, SOF callback, interval override |
| **Upstream version** | 0.20.0 (3 major versions ahead) |
| **Remote mismatch** | `.gitmodules` points to OpenStickCommunity fork; actual `origin` remote is `hathach/tinyusb` (upstream) |

### Custom Patches Summary

The 18 custom commits in the fork address three main areas:

1. **HID Host Patches** — Enhance the HID host implementation to support greater controller diversity. These patches extend device compatibility beyond the basic HID spec and are critical for gamepad detection and enumeration.

2. **SOF (Start of Frame) Callback** — Adds a callback mechanism triggered on USB SOF events. GP2040-CE uses this for high-precision timing and state synchronization with the host.

3. **Interval Override Changes** — Allows the firmware to tune USB polling intervals per endpoint, overriding host defaults. Necessary to optimize latency and responsiveness for gamepad input.

All three areas are specific to GP2040-CE's gamepad use case. None are generic Pico SDK conveniences — they are functional requirements.

## Goals

1. Audit the 18 custom commits to determine which are still necessary (have they been absorbed upstream?) and which have been superseded
2. Test each surviving patch against upstream 0.20.0 to identify merge conflicts and API changes
3. Rebase or cherry-pick surviving patches onto upstream TinyUSB 0.20.0
4. Update `.gitmodules` to point to the canonical upstream repository (`hathach/tinyusb`)
5. Verify USB HID host functionality (device enumeration, controller detection, input routing) on real hardware with a variety of controllers
6. Update `CMakeLists.txt` and build system references to point to upstream 0.20.0

## Approach & Phases

### Phase 1: Patch Audit (2–3 days)
1. List all 18 custom commits with their original authors and descriptions
2. Cross-reference against upstream TinyUSB 0.20.0 changelog and release notes
3. For each commit: determine if the feature is still relevant (yes/no/partial), if it's been merged upstream (yes/no), or if it's superseded
4. Document findings in a patch-by-patch breakdown table
5. Identify which commits must survive (estimated: 8–12 of 18)

### Phase 2: Conflict Resolution & Rebase (3–5 days)
1. Set up a clean fork of upstream TinyUSB 0.20.0
2. Attempt to cherry-pick surviving patches; document merge conflicts
3. For each conflict: analyze upstream changes and resolve (may require code-level understanding of HID host mechanics)
4. Test compilation at each rebase step to catch regressions early
5. Run GP2040-CE CMake build with the rebased TinyUSB to verify no link errors

### Phase 3: Hardware Regression Testing (3–5 days)
1. Flash firmware to an RP2040 device (e.g., Pico or standard controller board)
2. Test USB connection and device enumeration (verify controller is recognized by host)
3. Test HID input with at least 5 different controller models (wired USB and wireless USB dongles)
4. Verify polling intervals and latency (use USB analyzer or latency test app if available)
5. Test edge cases: rapid reconnection, sleep/wake on USB host, bus suspend/resume
6. Document any regressions or deviations from baseline behavior

### Phase 4: Remote Fix & Documentation (1 day)
1. Update `.gitmodules` to point to `hathach/tinyusb` (upstream)
2. Update `CMakeLists.txt` version reference to 0.20.0
3. Update `docs/development/dependency-updates.md` with new TinyUSB version and migration notes
4. Create a changelog entry documenting the upstream migration and any behavior changes

## Risks

**Risk Level: HIGH**

USB HID host operation is **core to gamepad functionality**. Any regression could render the device non-functional or incompatible with certain controllers. Specific concerns:

- **HID Host Complexity:** The custom patches likely exist because TinyUSB's generic HID host handling does not cover all real-world gamepad protocols (especially wireless USB dongles with proprietary extensions)
- **Interval Override Dependencies:** If this feature is not in 0.20.0, gamepad latency may increase, affecting user experience in fast-paced games
- **SOF Callback Timing:** Precise timing requirements for gamepad input. Even small changes in TinyUSB's SOF handling could introduce jitter
- **Conflict Resolution:** Complex HID code makes merge conflicts difficult to resolve safely without deep domain knowledge

**Mitigation:**
- Test with diverse hardware (at least 5 different controller models)
- Have a fallback plan to revert to the fork if blocking issues are discovered
- Consider reaching out to TinyUSB maintainers (hathach) for guidance on the patches

## Dependencies

- **pico-pio-usb port:** Closely related. If USB host handling is refactored, pico-pio-usb may also require updates to coordinate with TinyUSB. Evaluate both together (see `docs/development/pico-pio-usb-upstream-port.md`)
- **Pico SDK 2.2.0:** Project-wide minimum (enforced in CMakeLists.txt). TinyUSB 0.20.0 must support this SDK version

## Success Criteria

1. ✓ Patch audit completed with clear assessment of which 18 commits are necessary
2. ✓ All surviving patches rebased onto TinyUSB 0.20.0 with conflicts resolved
3. ✓ GP2040-CE firmware compiles without errors or warnings
4. ✓ Device enumerates on host and is recognized as a controller
5. ✓ Gamepad input works correctly with at least 5 different controllers
6. ✓ No increase in USB latency compared to baseline fork (measure with USB analyzer if available)
7. ✓ Remote mismatch fixed: `.gitmodules` points to upstream; `origin` remote is consistent
8. ✓ Documentation updated in `docs/development/dependency-updates.md` and changelog
9. ✓ Pull request merged to main branch with full regression test results

---

**Maintained by:** GP2040-CE core team  
**Last updated:** 2026-03-28  
**References:** `lib/tinyusb`, `CMakeLists.txt`, `docs/development/dependency-updates.md`, `docs/development/pico-pio-usb-upstream-port.md`
