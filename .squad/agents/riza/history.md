## Project Context (Day 1)

**Project:** GP2040-CE — RP2040 firmware for gamepads and game controllers  
**Stack:** C/C++, CMake, Raspberry Pi Pico SDK 2.1.1, Protobuf, React (web configurator)  
**Goal:** Create feature documentation for the firmware  
**User:** Fortinbra  
**Repo root:** C:\ws\GP2040-CE

**Key directories:**
- src/ — firmware C++ implementation
- headers/ — data structures and interfaces
- lib/ — third-party libraries
- configs/ — board configurations (default: Pico)
- proto/ — protobuf config protocol definitions
- www/ — React web configurator
- docs/ — existing documentation

## Learnings

### Review Cycle 1: RP2350 & Dependency Docs (2026-03-28T210547Z)

**Reviewed:**
- `docs/development/rp2350-support.md` (by Hughes, based on Edward's analysis)
- `docs/development/dependency-updates.md` (by Hughes)

**Verdict:** Both documents **REJECTED**

**Root Issue:** SDK Version Inconsistency
- Both new docs claim Pico SDK **2.2.0** required
- Project's `copilot-instructions.md` documents default as **2.1.1**
- This contradiction must be resolved before publication
- Edward (Firmware Dev) reassigned for revision — he has authority to clarify actual minimum SDK

**Specific Gaps Identified:**
1. RP2350 doc needs context on Pico2W blocker (WiFi/CYW43439 not yet ported)
2. RP2350 doc shows wrong SDK version verification method (shows cmake version, not SDK version)
3. Dependency doc missing callout that web configurator requires SKIP_WEBBUILD=FALSE (the default)
4. Both docs need to clarify whether 2.2.0 is RP2350-specific or project-wide requirement

**Positive Findings:**
- Technical accuracy of hardware specs (GPIO counts, PIO blocks, board configs) is solid
- Build commands and structures match actual codebase
- Vendored library list comprehensive and correct
- Testing checklists are actionable
- Markdown formatting consistent

### Review Cycle 2-6: Full Gauntlet (2026-03-28T024429Z)

**Rounds completed:** 6 total (round 1 was prior session)  
**Final verdict:** ✅ APPROVED (round 6)

**Round-by-round summary:**

| Round | Issues Found | Fixed By |
|-------|-------------|----------|
| 2 | Wrong file ref; stale note; wrong year | Mustang |
| 3 | CMake min version wrong; 2nd pico_sdk_import.cmake path | Winry |
| 4 | Picotool `2.2.0` → `2.2.0-a4`; AI agent names in public doc | Fortinbra |
| 5 | "Roy Mustang, Project Lead" in Maintained-by field | Fortinbra |
| 6 | No issues — APPROVED | — |

**Reviewer lockout chain triggered:**  
Hughes (original author) → locked round 1  
Edward (round-1 fixer) → did not trigger lockout (round-2 issues were new, not regressions in Edward's work; Mustang assigned)  
Mustang (round-2 fixer) → locked round 3 → Winry assigned  
Rounds 4 & 5 escalated to Fortinbra (human governance decisions)

**Patterns established for future reviews:**
- Never include Squad agent names in public docs; use "GP2040-CE core team" attribution
- Verify exact version strings against CMakeLists.txt — not docs, not memory
- `cmake --version` ≠ Pico SDK version — always catch this as a review error
- Round-4+ issues that are pure governance decisions escalate to Fortinbra, not agents

### Review Cycle: BT Support Doc Round 2 (2026-03-28)

**Reviewed:**
- `docs/development/bluetooth-support.md` (revision by Edward, commit ba070e0c)
- `docs/development/rp2350-support.md` (tense fix by Edward)

**Verdict:** ✅ APPROVED

**All 4 Round-1 Issues Resolved:**
1. Out-of-scope GPIO retro console declaration added to Overview (line 27) — exact language matches requirement
2. `pico_cyw43_arch_lwip_threadsafe_background` added to both locations: Minimum Requirements (line 47) and Task 1.1 (line 268)
3. `const` qualifiers restored on both GPDriver method return types (lines 88–89)
4. Cross-document tension resolved: clarifying note in bt-support.md Related Documentation + rp2350-support.md tense changed from "was developed for" → "is being developed for"

**Full sweep clean:**
- No agent names in either document
- SDK 2.2.0 consistent across bt-support.md and rp2350-support.md
- No false claims of BT being implemented
- CMake targets complete at both required locations
- rp2350-support.md tense fix introduced no regressions

**Findings written to:** `.squad/agents/riza/bt-review-2.md`

**BT Docs Approved:** All four documentation files ready for PR #7 (copilot-instructions.md, rp2350-support.md, dependency-updates.md, bluetooth-support.md)
