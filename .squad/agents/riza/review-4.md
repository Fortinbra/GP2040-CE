# Riza — Review Round 4
**Date:** 2026-03-28  
**Reviewer:** Riza Hawkeye  
**Branch:** `docs/copilot-instructions`  
**Commit reviewed:** `a23b72ee`

---

## VERDICT: REJECTED

Two issues found in the final sweep. Winry's two fixes are both confirmed correct, but pre-existing content in two files contains errors that must be corrected before this goes to PR.

All original authors (Hughes, Edward, Mustang, Winry) are locked out.  
**This escalates to Fortinbra.**

---

## Winry's Two Fixes — Both Confirmed Clean ✅

### Fix 1: `rp2350-support.md` line 49 — CMake minimum version
- **Was:** `CMake 3.13+` (per round-3 rejection)
- **Now:** `CMake 3.10+`
- **Verified against:** `CMakeLists.txt` line 52: `cmake_minimum_required(VERSION 3.10...4.0)`
- **Status:** ✅ Correct

### Fix 2: `dependency-updates.md` line 295 — Removed `pico_sdk_import.cmake` from version-bump list
- **Was:** Version-bump instructions included `pico_sdk_import.cmake` as a file to update when bumping the SDK
- **Now:** Line 295 reads: `Version pin updates in \`CMakeLists.txt\` (SDK and ArduinoJson) or \`www/package.json\` (npm packages)`
- **Verified:** `pico_sdk_import.cmake` no longer appears as a version-pin file. The separate description of what `pico_sdk_import.cmake` *does* (lines 17–21, SDK sourcing modes) is factually correct and appropriately distinct from the version-bump instructions.
- **Status:** ✅ Correct

---

## Issues Requiring Correction Before PR

### ❌ Issue 1: `copilot-instructions.md` line 81 — Picotool version wrong

**Location:** Section "Build Tools" under "Platform and SDK Configuration"

**Current text:**
```
- **Picotool**: 2.2.0 (for flashing and device operations)
```

**Actual value in `CMakeLists.txt` line 9:**
```cmake
set(picotoolVersion 2.2.0-a4)
```

The project pins picotool `2.2.0-a4`. The documentation states `2.2.0`. These are different version identifiers. If a contributor or Copilot attempts to match this version, they will use the wrong one.

**Required fix:** Change to `2.2.0-a4`.

---

### ❌ Issue 2: `dependency-updates.md` line 300 — Squad agent names in public-facing documentation

**Location:** Section "How to propose a version bump", step 3

**Current text:**
```
3. **Code review** by the team (especially Edward for firmware, Winry for web UI, Riza for stability)
```

"Edward," "Winry," and "Riza" are internal AI agent names for this squad. They are not GP2040-CE project maintainers, do not have GitHub identities on the project, and will be meaningless to any real contributor reading this documentation. Submitting a PR with these names in the process docs would be confusing and inappropriate for a public open-source repository.

**Required fix:** Replace with role-based language (e.g., "reviewed by the core team (firmware, web UI, and stability areas)") or omit the parenthetical entirely. Do not use squad agent names in contributor-facing documentation.

---

## Previously Identified Issues — All Previously Cleared ✅

| Round | Issue | Status |
|-------|-------|--------|
| R2 | RP2040 SRAM claim in copilot-instructions.md | Cleared R2 |
| R2 | Board config name consistency | Cleared R2 |
| R2 | npm command accuracy | Cleared R2 |
| R3 (then R4) | CMake version 3.13 → 3.10 | Fixed by Winry, confirmed clean |
| R3 | pico_sdk_import.cmake in version-bump list | Fixed by Winry, confirmed clean |

---

## Full Sweep Results — All Sections Checked

### `rp2350-support.md`
| Check | Finding |
|-------|---------|
| RP2350A GPIO count (30, pins 0–29) | ✅ |
| RP2350B GPIO count (48, pins 0–47) | ✅ |
| SRAM 520KB both variants | ✅ |
| Max clock 150 MHz | ✅ |
| PIO blocks: 3 (PIO0/1/2) | ✅ |
| Board table: Pico2 / RP2350A / `Pico2` / 30 GPIO | ✅ |
| Board table: FlatboxRev8 / RP2350A / `FlatboxRev8` / 30 GPIO | ✅ |
| Board table: SparkFunProMicro / RP2350B / `SparkFunProMicroRP2350` / 48 | ✅ |
| SDK minimum: Pico SDK 2.2.0 | ✅ |
| CMake minimum: 3.10+ | ✅ (Winry's fix) |
| `set(sdkVersion 2.2.0)` in CMakeLists.txt — cited correctly | ✅ |
| FATAL_ERROR cmake snippet — matches CMakeLists.txt lines 62–63 | ✅ |
| Build commands: `cmake -DGP2040_BOARDCONFIG=... -B build -S .` | ✅ |
| `PICO_SDK_PATH` flag syntax | ✅ |
| `PICO_BOARD pico2` for RP2350A — matches Pico2.cmake | ✅ (verified) |
| `PICO_BOARD sparkfun_promicro_rp2350` for RP2350B — matches SparkFunProMicroRP2350.cmake | ✅ (verified) |
| `PICO_PLATFORM rp2350-arm-s` — matches both cmake files | ✅ (verified) |
| GPIO validation ranges cited correctly (RP2350A 0–29, RP2350B 0–47) | ✅ |
| Web configurator GPIO auto-detect note | ✅ |
| Clock difference: 150 MHz (RP2350) vs 133 MHz (RP2040) | ✅ |
| RISC-V disclaimer — targets ARM (`rp2350-arm-s`) | ✅ |
| UF2 file naming pattern cross-file | ✅ |
| SDK datasheet/resource links point to correct URLs | ✅ (format correct) |

### `dependency-updates.md`
| Check | Finding |
|-------|---------|
| SDK version 2.2.0 throughout | ✅ |
| ArduinoJson v6.21.2 — matches CMakeLists.txt line 148 | ✅ (verified) |
| `pico_sdk_import.cmake` role described as sourcing, not pinning | ✅ |
| Version-bump file list: only `CMakeLists.txt` and `www/package.json` | ✅ (Winry's fix) |
| npm commands: `npm update`, `npm ci`, `npm install package-name@version` | ✅ |
| `npm run build` — exists in www/package.json scripts | ✅ (verified) |
| `npm start` — exists in www/package.json scripts | ✅ (verified) |
| `npm run lint` — exists in www/package.json scripts | ✅ (verified) |
| React `^18.2.0` — matches package.json | ✅ (verified) |
| Vite `^4.3.9` — matches package.json | ✅ (verified) |
| TypeScript `^5.3.3` — matches package.json | ✅ (verified) |
| Version matrix table combinations | ✅ |
| Squad agent names in line 300 | ❌ **Issue 2 (above)** |

### `.github/copilot-instructions.md`
| Check | Finding |
|-------|---------|
| SDK version 2.2.0 | ✅ |
| CMake 3.10+ | ✅ |
| Picotool version | ❌ **Issue 1 (above)** — says 2.2.0, CMakeLists.txt has 2.2.0-a4 |
| OpenOCD 0.12.0+dev | ✅ (unverifiable from repo, reasonable) |
| Build command `cmake -B build -S .` | ✅ |
| `SKIP_WEBBUILD=TRUE` in env example | ✅ |
| `GP2040_BOARDCONFIG=Pico` default | ✅ |
| `PICO_BOARD=pico` default | ✅ |
| RP2040 SRAM 264KB | ✅ |
| Header guard convention `#ifndef HEADER_NAME_H` | ✅ |
| Directory structure — all named paths exist | ✅ |
| Branch naming conventions | ✅ |
| `git checkout -b feature/your-feature` from main | ✅ |

---

## Cross-File Consistency

| Item | rp2350-support.md | dependency-updates.md | copilot-instructions.md | Consistent? |
|------|-------------------|----------------------|------------------------|-------------|
| Pico SDK version | 2.2.0 | 2.2.0 | 2.2.0 | ✅ |
| CMake minimum | 3.10+ | — | 3.10+ | ✅ |
| `SKIP_WEBBUILD=TRUE` | — | TRUE (CI default) | TRUE | ✅ |
| `pico_sdk_import.cmake` role | sourcing | sourcing | sourcing | ✅ |
| ArduinoJson version | — | v6.21.2 | — | ✅ |
| Default board | Pico (for RP2040 ref) | — | Pico | ✅ |

---

## Summary

Winry's two fixes are clean and correct. Two pre-existing errors were found in the final sweep and must be resolved before this PR ships:

1. **Picotool version** in `copilot-instructions.md` — fix `2.2.0` → `2.2.0-a4`
2. **Squad agent names** in `dependency-updates.md` line 300 — replace with role-based language

Both are straightforward single-line changes. Escalating to Fortinbra.
