# Review Round 3 — Riza Hawkeye

**Verdict: REJECTED**

Mustang's three surgical fixes are verified correct. However, the final sweep uncovered two factual errors that were not caught in prior rounds — one of which is a wrong Minimum Requirements claim in `rp2350-support.md`.

> Hughes, Edward, and Mustang are all locked out. Escalating to **Winry** for the remaining fixes.

---

## Part 1 — Verification of Mustang's Three Fixes

### Fix 1 ✅ `dependency-updates.md` — SDK version source corrected
**Claim:** Table row now points to `CMakeLists.txt` (not `pico_sdk_import.cmake`).
**Verified:** Line 15 reads:
```
| Raspberry Pi Pico SDK | 2.2.0 | `CMakeLists.txt` (`sdkVersion`), CI (`cmake.yml`) | ...
```
`CMakeLists.txt` line 7: `set(sdkVersion 2.2.0)` — matches exactly. ✅

### Fix 2 ✅ `dependency-updates.md` — Stale SDK 2.1.1 copilot-instructions note removed
**Claim:** The note saying `copilot-instructions.md` still referenced SDK 2.1.1 was removed.
**Verified:** No mention of 2.1.1 exists anywhere in the file. The file is clean of that stale observation. ✅

### Fix 3 ✅ `copilot-instructions.md` — `Last updated` year corrected to 2026
**Claim:** Year changed from 2025 → 2026.
**Verified:** Line 277: `**Last updated:** 2026-03-28` ✅

---

## Part 2 — Final Sweep Findings

### 🔴 ISSUE 1 — `rp2350-support.md`, Minimum Requirements section (line 49)

**Wrong CMake minimum version.**

The doc states:
```
- **CMake 3.13+** — standard requirement
```

Actual `CMakeLists.txt` line 52:
```cmake
cmake_minimum_required(VERSION 3.10...4.0)
```

The enforced minimum is **3.10**, not 3.13. This contradicts `copilot-instructions.md` line 82 which correctly states "CMake 3.10+". A developer on CMake 3.10, 3.11, or 3.12 would be incorrectly told their environment is non-compliant when it is perfectly valid.

**Required fix:** Change "CMake 3.13+" to "CMake 3.10+" in `rp2350-support.md`.

---

### 🟡 ISSUE 2 — `dependency-updates.md`, "How to propose a version bump" section (line 295)

**`pico_sdk_import.cmake` listed as a version-pin location — it isn't.**

The doc states:
```
- Version pin updates in `CMakeLists.txt`, `pico_sdk_import.cmake`, or `www/package.json`
```

`pico_sdk_import.cmake` contains no hardcoded SDK version. It is a boilerplate import helper that reads `PICO_SDK_PATH` or `PICO_SDK_FETCH_FROM_GIT_TAG` from the environment — it does not store a version pin. The SDK version is pinned exclusively in `CMakeLists.txt` (`set(sdkVersion 2.2.0)`). Listing `pico_sdk_import.cmake` here could send a developer on a futile search for a version string that doesn't exist in that file.

Note: This is a *separate and distinct* reference from the table row that Mustang correctly fixed in Fix 1. That row is clean. This is the prose bullet in the PR process section.

**Required fix:** Remove `pico_sdk_import.cmake` from that bullet, or replace with a clarifying note that this file does not hold version pins. Corrected line should read:
```
- Version pin updates in `CMakeLists.txt` (SDK and ArduinoJson) or `www/package.json` (npm)
```

---

### ℹ️ OBSERVATION — `copilot-instructions.md`, Build Tools section (line 79)

**Picotool version: "2.2.0" vs actual "2.2.0-a4"**

The doc states `Picotool: 2.2.0`. `CMakeLists.txt` line 9: `set(picotoolVersion 2.2.0-a4)`.

The pre-release suffix `-a4` is dropped. This is low severity — the major version is correct and a developer looking for picotool 2.2.0 will find the right release family. However, it is technically inaccurate and inconsistent with the pinned value.

**Not raising this as a blocking issue.** Noting it for Winry's awareness — if she touches the file for Issue 1, she may optionally correct this to `2.2.0-a4` for precision.

---

## Summary

| File | Status | Blocking Issues |
|------|--------|-----------------|
| `docs/development/rp2350-support.md` | ❌ Needs fix | CMake 3.13+ → 3.10+ |
| `docs/development/dependency-updates.md` | ❌ Needs fix | Remove `pico_sdk_import.cmake` from version-pin bullet |
| `.github/copilot-instructions.md` | ✅ Clean | (observation: picotool version, non-blocking) |

**Both blocking issues are pre-existing — not introduced by Mustang, Edward, or Hughes.** They were missed in all prior review passes.

**Assignee for fixes: Winry.** Both changes are surgical (1-2 line edits). Re-submit for final verification when done.

---

*Reviewed by Riza Hawkeye — Round 3*  
*Date: 2026-03-28*
