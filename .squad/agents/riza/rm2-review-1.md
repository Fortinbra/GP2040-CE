# Riza Review: rm2-module-support.md (Round 1)
Date: 2026-03-28
Status: REJECTED

---

## Issues

### Blocking Issue 1 — SDK Version String (Checklist item 1)

**File:** `docs/development/rm2-module-support.md`, line 5  
**Found:** `**SDK version:** 2.2.0+`  
**Required:** `2.2.0` (exact — no `+` suffix)  
**Ground truth:** `CMakeLists.txt` line 7 — `set(sdkVersion 2.2.0)` — no `+`

The `+` suffix is not part of the version string in the build system. All prior approved docs use `2.2.0` without qualification. Consistent with the pattern established in Review Cycle 2–6 (exact version strings are enforced strictly). The `+` implies "or later," which is not what the project's ground truth states.

**Fix:** Change line 5 from `2.2.0+` to `2.2.0`.

---

## Non-Blocking Checks (All Passed)

| Check | Result |
|-------|--------|
| Picotool version `2.2.0-a4` | ✅ Line 274 mentions exact string |
| CMake minimum version | ✅ Not mentioned — no wrong version present |
| GPIO 23 = WL_REG_ON (power enable) | ✅ Line 47, matches Edward's rm2-analysis.md |
| GPIO 24 = WL_DATA half-duplex (data + host wake) | ✅ Lines 48, 52–55, matches analysis |
| GPIO 25 = WL_CS (chip select) | ✅ Line 49, matches analysis |
| GPIO 29 = WL_CLK + VSYS shared | ✅ Lines 50, 56–58, matches analysis |
| Future feature status declared | ✅ Line 9: "Status: Planned — not yet implemented" |
| No fabricated hardware claims | ✅ All unconfirmed items marked TBD |
| No AI agent names | ✅ Clean sweep — none found |
| Maintained by = GP2040-CE core team | ✅ Line 4 |
| 4-space indentation in code blocks | ✅ CMake and C blocks use 4 spaces |
| Cross-reference to bluetooth-support.md | ✅ Lines 17, 231, 235, 250, 272 |
| Cross-reference to rp2350-support.md | ✅ Lines 76, 273 |
| Pin mapping table present | ✅ Lines 45–51 (GPIO table with SDK defines, functions, notes) |
| GPIO availability by chip (RP2040/RP2350A/RP2350B) | ✅ Lines 70–76 (three-row table) |
| SDK/CMake configuration section | ✅ Lines 90–184 (board headers, defines, link libraries) |
| Known constraints / TBD section | ✅ Lines 254–267 (7-item table, all marked TBD) |

---

## Verdict

**REJECTED** — one blocking issue: `2.2.0+` on line 5 must be `2.2.0` to match the ground truth in `CMakeLists.txt` line 7.

This is a minimal, single-character fix. All 14 other checklist items pass. The document is technically accurate, well-structured, and complete. Once the version string is corrected it is expected to pass Round 2 without further issues.

> **Lockout rule:** The author of this document must not perform the Round 1 fix. Assign revision to another team member.
