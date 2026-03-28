# BT Review 2 — `docs/development/bluetooth-support.md`

**Reviewer:** Riza Hawkeye  
**Date:** 2026-03-28  
**Revision author:** Edward  
**Documents reviewed:**
- `docs/development/bluetooth-support.md` (primary)
- `docs/development/rp2350-support.md` (tense fix verification)

---

## VERDICT: APPROVED ✅

All four issues from round 1 are correctly fixed. Full final sweep finds no new issues. Both documents are ready to advance to PR.

---

## Round-1 Fix Verification

### ISSUE 1 — Out-of-scope declaration for GPIO retro console output
**Required:** Explicit statement that GPIO retro console output is out of scope.  
**Status: ✅ FIXED**

Found at `bt-support.md` line 27, bottom of the Overview section:

> **Out of scope for this document:** GPIO output to retro consoles (SNES, N64, Dreamcast, Genesis, etc.). GP2040-CE reads FROM retro controllers via GPIO input add-ons (SNESpadInput, TG16padInput) but does not emit GPIO signals to emulate controllers for retro consoles. Retro console output is a separate architectural concern not addressed here and will be covered in a future document.

Placement is correct (bottom of Overview, before the horizontal rule). Language matches the suggested text precisely. The inclusion of "will be covered in a future document" is a welcome addition not in the suggested text — sets appropriate expectations without scope-creep into this doc.

---

### ISSUE 2 — Missing `pico_cyw43_arch_lwip_threadsafe_background` in CMake linkage
**Required:** Add the missing target to both the Minimum Requirements section and Task 1.1.  
**Status: ✅ FIXED — both locations corrected**

**Location 1 — Minimum Requirements (line 47):**
```
CMake target linkage: `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device`
```
All three targets present. ✅

**Location 2 — Task 1.1 (line 268):**
```
Add `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device` targets to `CMakeLists.txt` (only for Pico W)
```
All three targets present. The parenthetical "(only for Pico W)" is correct — the CYW43 targets must not be linked unconditionally for non-wireless boards. ✅

---

### ISSUE 3 — `const` qualifiers missing in simplified GPDriver listing
**Required:** Restore `const` on both return types in the simplified GPDriver interface block.  
**Status: ✅ FIXED**

Found at lines 88–89:
```cpp
virtual const uint8_t * get_descriptor_device_cb() = 0;
virtual const uint8_t * get_hid_descriptor_report_cb(...) = 0;
```
Matches `headers/gpdriver.h` exactly. API contract is correctly represented. ✅

---

### ISSUE 4 — Cross-document tension with approved `rp2350-support.md`
**Required:** (a) Note in bt-support.md clarifying BT is not yet implemented; (b) tense fix in rp2350-support.md from "was developed for" → "is being developed for."  
**Status: ✅ FIXED — both sub-items addressed**

**(a) bt-support.md Related Documentation note (line 440):**
> **Note:** `rp2350-support.md` references "Bluetooth HID" in the context of the Pico 2 W blocker. As of this writing, Bluetooth HID is **not yet implemented** on any GP2040-CE board, including Pico W. `rp2350-support.md` was written anticipating this feature; this document is the authoritative planning reference for Bluetooth HID.

Present, complete, correctly positioned. ✅

**(b) rp2350-support.md tense fix (line 246):**
> GP2040-CE's wireless feature stack (Bluetooth HID, WiFi-based web configurator access) **is being developed for** the RP2040-based Pico W…

"was developed for" → "is being developed for." Confirmed. ✅

---

## Full Final Sweep

### Version Numbers

| Check | Expected | Found | Result |
|-------|----------|-------|--------|
| Pico SDK version | 2.2.0 | "Pico SDK version: **2.2.0 or later**" (line 44) | ✅ Pass |
| CMake minimum | Not required in this doc (BT planning doc) | Not stated — appropriate; build requirements covered in dependency-updates.md | ✅ Pass |
| picotool version | Not applicable | Not mentioned — appropriate; picotool is a flash tool, irrelevant to BT implementation doc | ✅ Pass |

### Implementation Status Claims

| Check | Result |
|-------|--------|
| Document header: "Status: Planning / Not yet implemented" | ✅ Pass |
| Overview: "has zero wireless capability" | ✅ Pass |
| Related Documentation note: "not yet implemented on any GP2040-CE board, including Pico W" | ✅ Pass |
| Phase 0 (exploration) marked DONE; Phase 1 (implementation) marked NEXT — no BT code claimed | ✅ Pass |
| No present-tense "Bluetooth HID sends…" or "BTDriver handles…" framing outside planning context | ✅ Pass |

### Internal AI Agent Names

Searched entire document for any Squad team member names. None found. Attribution is "GP2040-CE core team" on line 4. ✅ Pass

### CMake Linkage Completeness

Both required locations (Minimum Requirements + Task 1.1) list all three targets in correct order:
1. `pico_cyw43_arch_lwip_threadsafe_background`
2. `pico_btstack_cyw43`
3. `pico_btstack_hid_device`
✅ Pass

### GPIO Retro Output Scoping

Explicitly stated as out of scope in Overview section. Includes:
- Enumerated console examples (SNES, N64, Dreamcast, Genesis)
- Correct characterization of GP2040-CE's actual GPIO input behavior (reads FROM retro controllers)
- Clear architectural separation statement
✅ Pass

### Cross-Document Consistency

| Document pair | Check | Result |
|---------------|-------|--------|
| bt-support.md ↔ rp2350-support.md | SDK version: both 2.2.0 | ✅ Pass |
| bt-support.md ↔ rp2350-support.md | BT status: both "not yet implemented" / "being developed" | ✅ Pass |
| bt-support.md ↔ rp2350-support.md | Pico 2 W blocked: both agree on CYW43 porting blocker | ✅ Pass |
| bt-support.md ↔ dependency-updates.md | bt-support.md links dependency-updates.md in Related Documentation | ✅ Pass |
| bt-support.md ↔ rp2350-support.md | bt-support.md links rp2350-support.md in Related Documentation | ✅ Pass |

### rp2350-support.md Post-Edit Sweep

Verifying the single-line tense change did not introduce any new issues:

| Check | Result |
|-------|--------|
| No agent names introduced | ✅ Pass |
| No new incorrect version numbers | ✅ Pass |
| No new BT-is-implemented claims | ✅ Pass |
| The surrounding Pico 2 W Known Limitations section is internally consistent | ✅ Pass |
| All previously-passing checks from prior approval cycle still hold | ✅ Pass |

### Residual Observations (No Action Required)

- `rp2350-support.md` has no "Maintained by:" field (bt-support.md does). This gap predates Edward's change and was approved in the previous review cycle. Not re-raising.
- bt-support.md's Related Documentation note says rp2350-support.md "was written anticipating this feature" — this phrasing is appropriate: it acknowledges the forward-looking nature of the prior doc without implying current implementation.

---

## Summary

All four round-1 blocking and correctness issues have been correctly addressed. The document accurately represents Bluetooth HID as a planned future feature, properly scopes out GPIO retro output, has complete CMake linkage targets in both locations, and is internally consistent with all three approved sibling documents. The tense fix in rp2350-support.md is correct and introduced no regressions.

**APPROVED: ready to add to PR.**
