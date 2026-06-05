# nanopb Stable Release Migration

## Overview

GP2040-CE vendored `lib/nanopb` is currently pinned to version `0.4.8-dev`, a development snapshot rather than a stable release. Development snapshots lack stability guarantees, security patch tracking, and clear versioning. This makes them risky for production firmware. This document outlines the migration from the dev snapshot to a stable nanopb release (0.4.8 or later in the 0.4.x series).

**Status:** Planned — deferred during automated dependency audit (requires careful verification of proto compatibility)

## Current State

| Item | Details |
|---|---|
| **Current version** | 0.4.8-dev (development snapshot) |
| **Location** | `lib/nanopb` (fully vendored source, not a submodule) |
| **Type** | Vendored source files (not a git submodule) |
| **Usage** | Protocol Buffer serialization for firmware ↔ web configurator communication and config storage |
| **Proto files** | Located in `proto/` directory |
| **Latest stable** | 0.4.8 stable (or later in 0.4.x series if available) |

### Why This Matters

Using development snapshots in production firmware introduces several risks:

1. **No Stability Guarantees** — Dev snapshots may contain experimental features or incomplete implementations that change between commits
2. **No Security Tracking** — Security patches in stable releases may not be backported to dev snapshots
3. **Version Ambiguity** — It is unclear which exact commit the dev snapshot corresponds to, making bug triage and patch backports difficult
4. **Long-term Supportability** — If a bug is discovered, it is harder to determine if it was present in the dev snapshot used

nanopb is used for:
- **Configuration Protocol** — Messages exchanged between the RP2040 and the web configurator (e.g., loading/saving gamepad settings)
- **Config Storage** — Persistent configuration stored in flash on the RP2040 (using the binary format that nanopb generates)

Any instability or unexpected serialization behavior could corrupt saved settings or break host-device communication.

## Goals

1. Identify the exact commit/tag of the current 0.4.8-dev snapshot
2. Determine the closest stable release (0.4.8 stable or newer)
3. Compare the dev snapshot and stable release: identify any API or behavior differences
4. Verify that all `.proto` files in `proto/` still compile correctly with the stable release
5. Test proto serialization/deserialization with the stable version (ensure no subtle behavior changes)
6. Replace vendored files in `lib/nanopb/` with stable release files
7. Update documentation to reflect the new stable version

## Approach & Phases

### Phase 1: Snapshot Identification (1 day)
1. Check `lib/nanopb/` for any version file, `CMakeLists.txt`, or `README` that identifies the exact commit
2. If the exact commit is unknown, inspect the nanopb repository changelog or releases page to find the closest matching snapshot date
3. Document: snapshot commit/tag, date, and the corresponding stable release (likely 0.4.8 stable)

### Phase 2: Compatibility Analysis (1–2 days)
1. Obtain the stable nanopb 0.4.8 release (from GitHub releases or official repository)
2. Create a comparison table:
   - **API Functions:** Did any protoc-gen-nanopb plugin API change? (e.g., function signatures, macro names, field access)
   - **Generated Code:** Compare output of nanopb code generator on sample `.proto` files between dev and stable
   - **Serialization:** Check if wire format or encoding changes (should be stable, but verify)
   - **Memory Layout:** Do struct field offsets remain the same? (impacts flash compatibility for saved configs)
3. Run a sanity check: compile a simple test program that uses both versions and compare generated output

### Phase 3: Proto Compilation & Testing (1–2 days)
1. Compile all `.proto` files in `proto/` using the stable nanopb plugin
2. Verify that the generated C code compiles with no errors or new warnings
3. Compare generated code against current snapshots (should be identical or very similar)
4. If any behavioral differences are found, document them and assess impact:
   - Can existing configs (stored in flash) still be deserialized with the new version? (must be yes)
   - Will new configs generated with stable version serialize identically? (should be yes)
5. Test round-trip serialization: generate a proto message, serialize it, deserialize it, compare (on both RP2040 and PC if possible)

### Phase 4: File Replacement (1 day)
1. Backup the current `lib/nanopb/` directory
2. Replace with stable 0.4.8 release files
3. Update any version references in `CMakeLists.txt` or build scripts
4. Rebuild the firmware and web configurator
5. Verify no new compile errors or warnings

### Phase 5: Integration Testing (1–2 days)
1. Build full GP2040-CE firmware with stable nanopb
2. Flash to an RP2040 device
3. Test with web configurator:
   - Load an existing configuration from flash (ensure stable version can read it)
   - Create a new configuration and save it
   - Reload the saved configuration (round-trip test)
   - Modify multiple settings and verify they persist correctly
4. If time permits: wipe config, set defaults via web configurator, verify round-trip
5. Document any issues or deviations from expected behavior

## Risks

**Risk Level: LOW–MEDIUM**

The 0.4.x series has a stable API. Migration from a dev snapshot to stable 0.4.8 should be straightforward if both use the same wire format. Risks are:

1. **Config Flash Incompatibility** — If the dev snapshot's generated code has subtle differences in memory layout or padding, existing configs stored in flash may no longer deserialize correctly. *Mitigation:* Phase 3 proto testing must verify this does not happen. If it does, a config migration script may be needed.

2. **Behavioral Differences** — Rare but possible: the dev snapshot might have optimizations or quirks that differ from stable 0.4.8. *Mitigation:* Round-trip serialization testing in Phase 5 will catch this.

3. **Plugin Output Format** — The nanopb plugin (protoc-gen-nanopb) generates C code. If the stable version generates significantly different code, it could expose latent bugs in how GP2040-CE uses the generated types. *Mitigation:* Compare generated code in Phase 2.

**How to avoid rollback:**
- Complete Phase 3 verification (proto compatibility) before committing to the migration
- Keep the old dev snapshot files in a backup branch until Phase 5 (integration testing) passes

## Dependencies

None directly. nanopb is vendored and standalone. However:
- Ensure Pico SDK 2.2.0 can compile the new nanopb files (should be automatic)
- The web configurator (React frontend) communicates with firmware via protobuf — ensure protocol messages remain compatible

## Success Criteria

1. ✓ Exact commit/tag of 0.4.8-dev snapshot identified
2. ✓ Stable release (0.4.8 or later) selected and justified
3. ✓ Compatibility analysis completed: no breaking API or serialization changes found
4. ✓ All `.proto` files compile without errors using stable release plugin
5. ✓ Generated code matches or is semantically equivalent to dev snapshot output
6. ✓ Round-trip serialization test passes (serialize → deserialize → compare)
7. ✓ Existing flash configs can be deserialized with stable version
8. ✓ New configs generated with stable version serialize and deserialize correctly
9. ✓ Firmware and web configurator both compile without errors
10. ✓ Integration test on hardware passes: load, save, reload config works correctly
11. ✓ Documentation updated in `docs/development/dependency-updates.md`
12. ✓ `lib/nanopb/` replaced with stable files; version references updated
13. ✓ Pull request merged to main branch with test results

---

**Maintained by:** GP2040-CE core team  
**Last updated:** 2026-03-28  
**References:** `lib/nanopb`, `proto/`, `CMakeLists.txt`, `docs/development/dependency-updates.md`
