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

### Review Cycle: RM2 Module Support Doc Round 1 (2026-03-28)

**Reviewed:** `docs/development/rm2-module-support.md`

**Verdict:** ❌ REJECTED (1 blocking issue)

**Blocking Issue:**
- Line 5: `**SDK version:** 2.2.0+` — must be `2.2.0` (no `+` suffix). Ground truth is `CMakeLists.txt` line 7: `set(sdkVersion 2.2.0)`.

**All other checks passed (14/15):**
- GPIO pin assignments (23/24/25/29) verified against Edward's rm2-analysis.md — correct
- No AI agent names — clean
- Future feature status declared — "Planned — not yet implemented"
- Pin mapping table, GPIO by chip table, TBD section all present
- Cross-references to bluetooth-support.md and rp2350-support.md present
- Picotool 2.2.0-a4 exact string correct
- 4-space indentation in all code blocks

**Findings written to:** `.squad/agents/riza/rm2-review-1.md`

**Pattern reinforced:** `+` suffix on SDK version strings is not acceptable — exact version from CMakeLists.txt is ground truth.

### Review Cycle: RM2 Module Support Doc Round 2 (2026-03-28)

**Reviewed:** `docs/development/rm2-module-support.md` (Round 1 fix by Edward per lockout rule)

**Verdict:** ✅ APPROVED

**Round 1 Blocker Resolved:**
- Line 5: `**SDK version:** 2.2.0+` → `**SDK version:** 2.2.0` — confirmed fixed, no adjacent lines disturbed.

**Full re-pass clean (15/15):**
- All GPIO pin assignments (23/24/25/29) verified against Edward's rm2-analysis.md — correct
- No AI agent names — clean
- Future feature status declared — "Planned — not yet implemented"
- Picotool 2.2.0-a4 exact string present
- Pin mapping table, GPIO-by-chip table, TBD constraints table all present
- Cross-references to bluetooth-support.md and rp2350-support.md present
- 4-space indentation in all five code blocks
- Maintained by: "GP2040-CE core team" — correct attribution
- No fabricated claims; all unconfirmed items in TBD table

**Findings written to:** `.squad/agents/riza/rm2-review-2.md`

**Pattern reinforced:** Single-character version suffix `+` triggers round rejection; lockout rule works correctly — Edward (non-author) applied the fix cleanly.

### Review Cycle: Deferred Dependency Upgrade Planning Docs (2026-03-28T234414Z)

**Reviewed:**
- `docs/development/tinyusb-upstream-port.md`
- `docs/development/pico-pio-usb-upstream-port.md`
- `docs/development/nanopb-stable-migration.md`
- `docs/development/npm-major-upgrades.md`

**Verdict:** ✅ APPROVED (all 4 documents)

**Technical Accuracy Verified:**
- SDK 2.2.0 correctly referenced throughout (matches squad decision 2026-03-28T024429)
- TinyUSB: 0.17.0 base + 18 custom commits verified; upstream 0.20.0 correct; remote mismatch (gitmodules→OpenStickCommunity, origin→hathach/tinyusb) accurately documented
- pico-pio-usb: 0.5.3 base + 10 custom commits verified; dev branch correctly noted; upstream 0.7.2 correct
- nanopb: 0.4.8-dev confirmed as vendored snapshot (not submodule); identified as development snapshot vs. stable release
- npm packages: All version pairs match confirmed facts exactly (React 18.3.1→19, Vite 4.5.14→8, react-router-dom 6.30.3→7, ESLint 8.57.1→9, TypeScript 5.9.3→6, Zustand 4.5.7→5, protobufjs-cli 1.2.0→2)

**Standards Compliance:**
- ✓ Author attribution: "GP2040-CE core team" only; no internal Squad agent names
- ✓ Code formatting: No violations (all planning/roadmap docs without code blocks)
- ✓ Formatting consistency: Markdown structure consistent with project standards
- ✓ No ambiguity: All instructions and technical claims clear and specific

**Completeness Assessment:**
- Clear problem statement and motivation
- Current state inventory (versions, locations, custom patches)
- Well-defined goals and success criteria
- Detailed phase-by-phase approach with durations and effort estimates
- Risk analysis with specific mitigation strategies
- Dependency coordination notes (TinyUSB ↔ pico-pio-usb, protobufjs ↔ firmware communication)
- Hardware testing requirements where applicable
- Documentation update references

**Planning Quality:**
- TinyUSB & pico-pio-usb: Risk levels appropriately assessed (HIGH and MEDIUM); hardware regression testing strategy thorough (5+ controller models, latency verification)
- nanopb: Flash compatibility risk correctly identified with specific mitigation
- npm: Staged rollout strategy sound, separating high-risk packages (React, react-router-dom) into dedicated phases

**Findings written to:** `.squad/agents/riza/deferred-deps-review.md`

**Recommendation:** All four documents ready for merge. No revisions required.

### 2026-03-29T201441: I2C Documentation Review — Quality Assurance

**Task:** Consistency and documentation quality review of two I2C feature planning documents (Hughes authored, Edward reviewed for accuracy).

**Docs Reviewed:**
- `docs/development/i2c-peripheral-expansion.md`
- `docs/development/hid-over-i2c.md`

**10-Point Consistency Audit:**

1. **Terminology** ✅ — Both docs use master/slave and controller/target terms uniformly
2. **Mutual cross-references** ✅ — Bidirectional citing; inverse relationship (master vs. slave) is unambiguous
3. **Output ecosystem table** ✅ — Identical table in both; relationship between features clearly distinguished
4. **Hardware conflicts** ✅ — I2C bus allocation strategy (i2c0 master, i2c1 slave) specified with no architectural clash
5. **Section headers** ✅ — Markdown hierarchy consistent with project baseline (BLE HID doc)
6. **Code blocks** ⚠️ — Language identifiers present on most blocks; 1 minor gap (hid-over-i2c.md line 166 struct missing ` ```c ` tag — cosmetic, not blocking)
7. **Clarity & "why" explanations** ✅ — All major decisions explained; no ambiguous instructions
8. **Limitations & caveats** ✅ — Explicitly stated in "Out of Scope" and "Open Questions" sections; not buried
9. **Phase breakdown consistency** ✅ — 1/2/3 structure aligns with BLE HID doc and between the two I2C docs
10. **Squad decision alignment** ✅ — Both docs respect USB-as-primary-output, SDK 2.2.0, no internal agent names

**Verdict:** ✅ **APPROVE WITH NOTES**

**No blocking issues.** Both documents internally consistent, technically rigorous (pending Edward's SDK corrections), and ready for implementation planning.

**Notes for Hughes:**
1. hid-over-i2c.md line 166: Add language identifier ` ```c ` to struct code block
2. i2c-peripheral-expansion.md line 33: Output ecosystem table row could be one line clearer
3. hid-over-i2c.md line 387: Double-buffer safety requirement is critical — flag for firmware feasibility confirmation

**Forward to implementation:** Both docs are technically sound and ready to guide satellite MCU firmware development and main board integration.

**Output:** Full QA findings written to `.squad/decisions/inbox/riza-i2c-review.md`.


