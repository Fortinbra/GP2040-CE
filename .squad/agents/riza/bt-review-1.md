# BT Review 1 — `docs/development/bluetooth-support.md`

**Reviewer:** Riza Hawkeye  
**Date:** 2026-03-28  
**Document reviewed:** `docs/development/bluetooth-support.md`  
**Original author:** Hughes  

---

## VERDICT: REJECTED

Two blocking issues. Two additional correctness concerns. Hughes is locked out per reviewer lockout policy. **Revision assigned to Edward.**

---

## Blocking Issues

### ISSUE 1 — Missing explicit out-of-scope declaration for GPIO retro console output

**Section:** None (absent from document)  
**Severity:** BLOCKING

Fortinbra's decision requires this document explicitly state that GPIO retro console output is **out of scope** for this feature. The document does not mention GPIO retro console output anywhere — not in scope, not out of scope, not in a Known Limitations section, not in Phase 3.

Edward's ground truth (bt-analysis.md §2) establishes that GPIO output to retro consoles does not exist in the codebase and that it is a distinct concern from Bluetooth HID. The bt-support.md architecture diagrams reference USB and BT only, which is correct — but the absence of any explicit scoping statement leaves the door open for misreading, especially given that Edward's analysis explicitly discusses GPIO output as a "Proposed multi-output architecture" component.

**Required fix:** Add an explicit scope statement. Suggested location: bottom of the Overview section, or a dedicated "Scope" subsection before Architecture. Example text:

> **Out of scope for this document:** GPIO output to retro consoles (SNES, N64, Dreamcast, Genesis, etc.). GP2040-CE reads FROM retro controllers via GPIO input add-ons (SNESpadInput, TG16padInput) but does not emit GPIO signals to emulate controllers for retro consoles. Retro console output is a separate architectural concern not addressed here.

---

### ISSUE 2 — Incomplete CMake target linkage (missing `pico_cyw43_arch_lwip_threadsafe_background`)

**Sections:** "Minimum Requirements" (line 45), Phase 1 Task 1.1 (line 266)  
**Severity:** BLOCKING

Edward's ground truth (bt-analysis.md §5, item 1) states that three CMake targets must be added:

> `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device`

The document lists only two:

> `pico_btstack_cyw43`, `pico_btstack_hid_device`

`pico_cyw43_arch_lwip_threadsafe_background` is the CYW43 architecture target that wires the BTstack event loop into the lwIP background thread model. Without it, the BT stack will not initialize correctly alongside the existing WiFi stack. This omission would cause a build or runtime failure during implementation.

The same omission appears in both the Requirements section and Task 1.1's description — two places must be corrected.

**Required fix:** Add `pico_cyw43_arch_lwip_threadsafe_background` to both locations.

---

## Additional Correctness Issues (Non-Blocking, Must Be Addressed)

### ISSUE 3 — `const` qualifiers missing in simplified GPDriver listing

**Section:** "Current State: USB-Centric Output" (lines 87–88)  
**Severity:** Correctness

The document shows:
```cpp
virtual uint8_t * get_descriptor_device_cb() = 0;
virtual uint8_t * get_hid_descriptor_report_cb(...) = 0;
```

The actual `headers/gpdriver.h` declares:
```cpp
virtual const uint8_t * get_descriptor_device_cb() = 0;
virtual const uint8_t * get_hid_descriptor_report_cb(uint8_t itf) = 0;
```

The block is labeled "Simplified GPDriver interface" but `const` correctness is an API contract, not a style detail. A developer reading this block to understand the interface boundary for `BTDriver` design would get incorrect type information. The `const` qualifier signals that these pointers point to read-only data (descriptors stored in flash/ROM) — omitting it could mislead a BTDriver implementer about ownership semantics.

**Required fix:** Add `const` to both return types in the simplified listing.

---

### ISSUE 4 — Cross-document tension with approved `rp2350-support.md`

**Section:** Does not originate in this document, but creates reader confusion  
**Severity:** Consistency flag

`rp2350-support.md` (already approved, § Known Limitations, Pico 2 W Config) states:

> "GP2040-CE's wireless feature stack (Bluetooth HID, WiFi-based web configurator access) **was developed for** the RP2040-based Pico W…"

The phrasing "was developed for" implies Bluetooth HID already exists and is shipping for Pico W. `bt-support.md` correctly states the opposite: "Status: Planning / Not yet implemented" and "has zero wireless capability."

`bt-support.md` is internally accurate. The problem is in the already-approved `rp2350-support.md`, which misleads readers about the current implementation state. Since a reader may encounter both documents, bt-support.md should add a clarifying note to preempt confusion.

**Required fix:** In bt-support.md's Overview or Related Documentation section, add a brief note:

> **Note:** `rp2350-support.md` references "Bluetooth HID" in the context of the Pico 2 W blocker. As of this writing, Bluetooth HID is **not yet implemented** on any GP2040-CE board, including Pico W. `rp2350-support.md` was written anticipating this feature; this document is the authoritative planning reference.

Additionally, a follow-up edit to `rp2350-support.md` should change "was developed for" to "is being developed for" to avoid the implication that BT already ships.

---

## Passing Checks

The following criteria were evaluated and passed:

| Check | Result |
|-------|--------|
| SDK version 2.2.0 (correct per CMakeLists.txt line 7 + decisions.md) | ✅ Pass |
| CMake minimum 3.10+ (correct per CMakeLists.txt line 52: `VERSION 3.10...4.0`) | ✅ Pass — consistent with rp2350-support.md |
| GPDriver overall architecture description (USB-centric, 17 modes, subclass pattern) | ✅ Pass |
| No runtime mode switching claim — correctly described as boot-time only | ✅ Pass |
| CYW43439 chip correctly identified in hardware table | ✅ Pass (table on line 35–36) |
| Pico 2 W correctly marked as blocked pending CYW43 porting to RP2350 | ✅ Pass |
| Single-primary-output constraint correctly stated | ✅ Pass |
| WiFi/web config coexistence correctly described as unaffected | ✅ Pass |
| No claims of existing Bluetooth code in firmware | ✅ Pass |
| No internal AI agent names anywhere in document | ✅ Pass |
| `Last updated: 2026-03-28` | ✅ Pass |
| `Maintained by: GP2040-CE core team` | ✅ Pass |
| 4-space indent in all code blocks | ✅ Pass |
| CYW43 + USB simultaneous operation correctly described as feasible | ✅ Pass |
| USB + BT hardware independence (separate USB PHY vs CYW43) | ✅ Pass |

---

## Revision Assignment

**Hughes (original author): LOCKED OUT** per reviewer lockout policy.

**Edward is assigned the revision.** All four issues above must be addressed before re-review. Issues 1 and 2 are blocking; Issues 3 and 4 must also be corrected before the document advances to PR.
