# Review Round 5 — Final Gate
**Reviewer:** Riza Hawkeye  
**Date:** 2026-03-28  
**Branch:** `docs/copilot-instructions`  
**Commit verified:** `43f216d8`

---

## VERDICT: REJECTED ❌

**One remaining issue.** Both of Fortinbra's fixes are correct. All version numbers check out. One internal AI agent name survives in a publicly committed file — same category as the fix just made.

---

## Part 1 — Verifying Fortinbra's Two Fixes

### Fix 1 · `copilot-instructions.md` line 80: Picotool version
**VERIFIED CORRECT ✅**

- **Doc reads:** `**Picotool**: 2.2.0-a4`
- **CMakeLists.txt line 9:** `set(picotoolVersion 2.2.0-a4)`
- Exact match. Previous value `2.2.0` (missing the `-a4` suffix) was wrong. Now correct.

---

### Fix 2 · `dependency-updates.md` line 300: AI agent names removed
**VERIFIED CORRECT ✅**

- **Doc now reads:** `(firmware reviewers for C++ changes, frontend reviewers for web UI changes)`
- No internal AI names (`Edward`, `Winry`, `Riza`) remain. Replacement text is generic, role-appropriate, and contributor-safe.

---

## Part 2 — Final Sweep: All Three Files

### Version cross-checks against CMakeLists.txt

| Version | CMakeLists.txt ground truth | `rp2350-support.md` | `dependency-updates.md` | `copilot-instructions.md` |
|---|---|---|---|---|
| Pico SDK | `set(sdkVersion 2.2.0)` (line 7) | 2.2.0 ✅ (lines 48, 52, 59, 62) | 2.2.0 ✅ (lines 15, 22–25, 127, 241) | 2.2.0 ✅ (line 74) |
| Picotool | `set(picotoolVersion 2.2.0-a4)` (line 9) | not mentioned | not mentioned | 2.2.0-a4 ✅ (line 80) |
| CMake minimum | `cmake_minimum_required(VERSION 3.10...4.0)` (line 52) | "CMake 3.10+" ✅ (line 49) | not mentioned | "CMake: 3.10+" ✅ (line 82) |
| Ninja | not in CMakeLists.txt or CI workflows | not mentioned | not mentioned | v1.12.1 ⚠️ (line 79) |

**Ninja note:** `v1.12.1` in `copilot-instructions.md` line 79 is unverifiable — neither `CMakeLists.txt` nor `.github/workflows/cmake.yml` specify a Ninja version (CI uses Ubuntu system Ninja). This is a pre-existing value unchanged by this commit and was present through rounds 1–4. Not blocking.

---

### Board config names — `rp2350-support.md`

Cross-checked against `configs/` directory:

| Config name in doc | Exists in `configs/`? |
|---|---|
| `Pico2` | ✅ confirmed |
| `FlatboxRev8` | ✅ confirmed |
| `SparkFunProMicroRP2350` | ✅ confirmed |

UF2 filename examples on lines 77–80 match the three confirmed config names. ✅

---

### No-AI-names sweep

Scanning all three files for surviving internal AI agent names:

| File | Finding |
|---|---|
| `rp2350-support.md` | ✅ No AI names |
| `dependency-updates.md` | ✅ No AI names (fix confirmed) |
| `copilot-instructions.md` | ❌ **Line 278: `**Maintained by:** Roy Mustang, Project Lead`** |

---

## The One Remaining Issue

**File:** `.github/copilot-instructions.md`  
**Line:** 278  
**Current value:**
```
**Maintained by:** Roy Mustang, Project Lead
```

**Problem:** `Roy Mustang` is an internal AI squad member name. This is the same category of issue as the fix just applied to `dependency-updates.md` — an internal AI agent name in a publicly committed file. `.github/copilot-instructions.md` is committed to the repository and visible to all contributors and external readers.

**Required fix:**
```
**Maintained by:** GP2040-CE core team
```
(or any equivalent generic reference — "the development team", "GP2040-CE contributors", etc.)

**Scope:** One line, one file. No other changes required.

---

## Summary

| Check | Result |
|---|---|
| Fix 1 — Picotool 2.2.0-a4 | ✅ Correct |
| Fix 2 — AI names out of dependency-updates.md | ✅ Correct |
| SDK 2.2.0 consistent across all three docs | ✅ |
| CMake minimum 3.10 consistent | ✅ |
| Board config names exist in `configs/` | ✅ |
| No AI names in public docs | ❌ — one survivor on line 278 of copilot-instructions.md |

**Once line 278 of `copilot-instructions.md` is updated, this PR is clear to merge.**
