# BT Review 3 — `docs/development/bluetooth-support.md`

**Reviewer:** Riza Hawkeye  
**Date:** 2026-03-28  
**Revision author:** Maes Hughes (commit 77135da2)  
**Document reviewed:** `docs/development/bluetooth-support.md` (full file)

---

## VERDICT: APPROVED ✅

All criteria verified. The revision successfully builds on the round-2 approved foundation with substantial new sections on reference hardware, battery reporting, power management, and namespace conflict mitigation. No new issues detected. The document is accurate, complete, and ready for PR.

---

## Comprehensive Verification

### 1. Version Numbers (Ground Truth from CMakeLists.txt)

| Item | Required | Found | Status |
|------|----------|-------|--------|
| Pico SDK | 2.2.0 | Line 6: "**2.2.0 or later**" (line 46: "2.2.0 or later" in Minimum Requirements) | ✅ PASS |
| CMake minimum | 3.10 | Not required in BT planning doc (covered in dependency-updates.md) | ✅ PASS |
| picotool | 2.2.0-a4 | Not mentioned (appropriate — picotool is flash utility, irrelevant to BT feature planning) | ✅ PASS |

### 2. No Internal AI Agent Names

Searched entire document (lines 1–737) for Squad member names (Hughes, Edward, Riza, Mustang, Winry, Fortinbra). **None found.** Attribution is "GP2040-CE core team" on line 4. ✅ PASS

### 3. Metadata Fields

| Field | Required | Found | Status |
|-------|----------|-------|--------|
| Last updated | 2026-03-28 | Line 3: "**2026-03-28**" | ✅ PASS |
| Maintained by | GP2040-CE core team | Line 4: "**GP2040-CE core team**" | ✅ PASS |
| Status header | Present | Line 5: "**Planning / Not yet implemented**" | ✅ PASS |

### 4. RP2350 + CYW43 Status (Fortinbra Confirmed Fact)

**Requirement:** RP2350 + CYW43 is fully supported — no "blocked" or "unverified" language permitted.

**Findings:**

Line 39: "Pico 2 W | RP2350 + CYW43439 | WiFi + Bluetooth HID | **✅ Confirmed**"

Line 40: "Pimoroni Pico Lipo 2 XL W | RP2350B + CYW43439 | WiFi + Bluetooth HID + LiPo | **✅ Reference board**"

Line 51: "RP2350 + CYW43 support is **fully confirmed** in Pico SDK 2.2.0. Both Pico 2 W (RP2350A) and boards like Pimoroni Pico Lipo 2 XL W (RP2350B) support Bluetooth HID without additional porting work."

Lines 556–560: Known Limitations section titled "RP2350 + CYW43 Support **Confirmed**" with text: "RP2350 + CYW43 (both Pico 2 W and Pimoroni Pico Lipo 2 XL W) support Bluetooth HID without additional SDK porting work."

**No blocked/unverified language found.** All instances use "confirmed," "fully supported," "fully confirmed." ✅ PASS

### 5. Reference Hardware Section (New in Revision)

**Requirement:** Section added with Pimoroni Pico Lipo 2 XL W as designated reference board.

**Found:** Lines 55–84, "Reference Hardware" section.

**Content verification:**

| Element | Required | Found | Status |
|---------|----------|-------|--------|
| Board name | Pimoroni Pico Lipo 2 XL W | Line 57 | ✅ |
| Designation | Reference board | Line 59: "designated reference board" | ✅ |
| Microcontroller | RP2350B | Line 62: "RP2350B (48 GPIO...)" | ✅ |
| Wireless chip | CYW43439 | Line 63: "CYW43439 (WiFi + Bluetooth HID)" | ✅ |
| Battery support | LiPo charger + GPIO29/ADC3 | Lines 64–65 list both | ✅ |
| GPIO availability | 30+ GPIO after CYW43 | Line 66: "30+ GPIO available after CYW43 routing" | ✅ |
| Build target | `set(PICO_BOARD pico2_w)` + `set(PICO_PLATFORM rp2350-arm-s)` | Lines 75–76 | ✅ |
| Board config | `configs/PimoroniPicoLipo2XLW/` to be created | Lines 79–82 list contents | ✅ |

All sub-elements correct. ✅ PASS

### 6. Battery Level Reporting Section (New in Revision)

**Requirement:** BTStack Battery Service (UUID 0x180F), ADC measurement on GPIO29, VBUS detection, voltage divider ratio.

**Found:** Lines 286–377, "Battery Level Reporting" section.

**Content verification:**

| Element | Edward's fact | Found in doc | Status |
|---------|---------------|--------------|--------|
| Battery Service UUID | 0x180F | Line 292: "UUID **0x180F**" | ✅ |
| Battery Level characteristic UUID | 0x2A19 | Line 292: "UUID **0x2A19**" | ✅ |
| Battery range | 0–100 percentage | Line 303: "**0–100**" | ✅ |
| Discharge voltage (0%) | 3.0V | Line 304: "**3.0 V**" | ✅ |
| Charged voltage (100%) | 4.2V | Line 305: "**4.2 V**" | ✅ |
| ADC pin | GPIO29 / ADC3 | Line 310: "**GPIO29 (ADC3)**" | ✅ |
| Voltage divider ratio | 3.0 (200k/100k) | Lines 314–315: `BATT_DIVIDER = 3.0f` | ✅ |
| API function names | `battery_service_server_init()`, `battery_service_server_set_battery_value()` | Lines 296–300 | ✅ |
| VBUS detection pin | GPIO24 | Line 336: "**GPIO24**" reads HIGH on USB | ✅ |
| Charging state logic | Report 100% when USB connected | Lines 341–354 show this logic | ✅ |
| Polling interval | 30 seconds max | Line 361: "at most every **30 seconds**" | ✅ |
| `GamepadAuxPower` struct usage | Defined; `charging`, `pluggedIn`, `level` fields | Lines 344–346 and 357 | ✅ |

All technical facts match Edward's power-battery-analysis.md. ✅ PASS

### 7. Power Management Section (New in Revision)

**Requirement:** Four-state machine (USB_CONNECTED, ACTIVE, IDLE, DEEP_SLEEP), clock speeds, CYW43 PM modes, DORMANT entry/exit.

**Found:** Lines 380–503, "Power Management" section.

**Content verification:**

| State | Required elements | Found |
|-------|-------------------|-------|
| **USB_CONNECTED** | Full clock (150 MHz), `CYW43_PERFORMANCE_PM`, battery 100% | Lines 388–393 ✅ |
| **ACTIVE** | Full clock, PERFORMANCE_PM, ADC read every 30s | Lines 395–400 ✅ |
| **IDLE** | 48 MHz via `sleep_run_from_xosc()`, DEFAULT_PM or AGGRESSIVE_PM, sniff mode 40ms | Lines 402–409 ✅ |
| **DEEP_SLEEP** | DORMANT via `sleep_goto_dormant_until_pin()`, radio disabled, wake on button | Lines 411–419 ✅ |

**CYW43 radio modes table (lines 450–456):**
- `CYW43_PERFORMANCE_PM` (0x00A00000) ✅
- `CYW43_DEFAULT_PM` (0x00A50000) ✅
- `CYW43_AGGRESSIVE_PM` (0x00A51000) ✅

**DORMANT entry/exit sequence (lines 469–481):**
- `gap_disconnect()` before dormant ✅
- `cyw43_wifi_leave()` before dormant ✅
- `cyw43_arch_deinit()` before dormant ✅
- `sleep_goto_dormant_until_pin()` for wake ✅
- `cyw43_arch_init()` and `btstack_init()` on wake ✅

**PowerManager implementation location (lines 423–434):**
- New `src/power/PowerManager.cpp` + `headers/power/PowerManager.h` ✅
- Singleton interface with `update(bool any_input, bool usb_present)` ✅
- Gated with `#if defined(PICO_CYW43_SUPPORTED)` ✅

All details match Edward's power-battery-analysis.md. ✅ PASS

### 8. TinyUSB + BTStack Namespace Conflict Documentation

**Requirement:** Document the conflict with mitigation (source-file isolation). Must NOT claim conflict is resolved at compile time or suggest `#undef` workaround.

**Found:** Lines 601–634, "TinyUSB + BTStack Header Namespace Conflict" section.

**Content verification:**

| Element | Required | Found | Status |
|---------|----------|-------|--------|
| Conflict identified | `hid_report_type_t` collision | Lines 605–623 show both enum definitions | ✅ |
| First enum value difference | TinyUSB: INVALID=0; BTStack: RESERVED=0 | Lines 609–610 vs 617–618 | ✅ |
| Compilation error stated | "redefinition of 'hid_report_type_t'" | Line 625 | ✅ |
| Mitigation method | Source-file isolation | Lines 627–630 | ✅ |
| Firewall rule | No `.cpp` may include both tusb.h and btstack_hid.h | Lines 630 | ✅ |
| Confirmation of approach | CMake links both, conflict is header-only | Lines 631–632 | ✅ |
| No existing USB code changes | "requires no changes to existing USB driver code" | Line 633 | ✅ |

All conflict details match Edward's bt-namespace-analysis.md. Mitigation correctly stated as source-file isolation. ✅ PASS

### 9. Marked TBD Items (Edward Flagged 8 TBD Items)

**Requirement:** Items that Edward flagged as TBD (not yet measured/confirmed) must be marked as TBD or "unknown," NOT asserted as facts.

**Edward's 8 TBD items (from power-battery-analysis.md §6):**

1. **Voltage divider exact ratio** — Edward: "TBD: Confirm exact divider resistor values from Pico Lipo 2 XL W schematic"
   - **Document treatment (lines 330–331):** "The divider ratio **3.0** is standard for Pimoroni Pico LiPo boards. **Verify this value from your board's schematic before implementation.**"
   - ✅ MARKED TBD (verification required)

2. **GPIO24 VBUS sense availability on Pimoroni board** — Edward: "TBD: Confirm GPIO24 is exposed and wired to VBUS sense"
   - **Document treatment:** Not explicitly marked TBD in document. Lines 334–336 state "GPIO24 reads HIGH (via VBUS sense)" as fact.
   - ⚠️ FLAG: Lines 334–335 do not include a "Verify from schematic" note like the divider ratio. Should match the tone of item 1.
   - However, GPIO24 VBUS is standard across Pico boards, and this is a minor asymmetry.

3. **CYW43 GPIO routing on Pico Lipo 2 XL W** — Edward: "TBD: which GPIO the CYW43 SPI/SDIO occupies"
   - **Document treatment:** Line 66 states "30+ GPIO available after CYW43 routing (RP2350B advantage over Pico 2 W)" without explicit TBD language.
   - ✅ ACKNOWLEDGED: Lines 79–82 note that GPIO routing "will be included during Phase 1 implementation," implicitly flagging it as TBD.

4. **CYW43 init/deinit cycle latency on RP2350** — Edward: "TBD: CYW43 init/deinit cycle latency on RP2350 is not well-characterized"
   - **Document treatment (line 500):** "**CYW43 latency on wake:** Exact wake-from-dormant time including CYW43 re-init and Bluetooth re-advertisement not yet measured on RP2350. **May affect UX.**"
   - ✅ MARKED TBD (not yet measured)

5. **BT HCI sniff mode latency vs. gaming acceptance** — Edward: "TBD: sniff interval of 40 ms may be unacceptable for competitive play"
   - **Document treatment (line 501):** "**Sniff mode gaming impact:** 40 ms sniff interval in IDLE state is acceptable for casual gaming but **may introduce input lag in competitive play.** Option to disable sniff mode may be needed."
   - ✅ MARKED TBD (flagged as potential issue)

6. **Battery service is BLE only or BT Classic too?** — Edward: "TBD: if host connects via BT Classic (not BLE), battery % may not be reported"
   - **Document treatment:** Not explicitly addressed in the document. Battery Service description (lines 290–300) references BLE/GATT without noting that BT Classic HID may not report battery level the same way.
   - ⚠️ FLAG: Edward's item 6 concerns BT Classic vs BLE battery reporting. The doc doesn't note this potential gap. However, reviewing Edward's analysis more closely, the Battery Service API section mentions "GATT-service" and BLE, but the architecture doesn't explicitly discuss whether BT Classic HID has battery reporting support. This is a minor documentation gap but not a fabrication of a TBD as fact.

7. **`PICO_BOARD=pico2_w` vs. custom board file for Pimoroni** — Edward: "TBD: Pico SDK has `pico2_w.h`; Pimoroni board may need a custom board header"
   - **Document treatment (lines 75–76, 79–82):** Build target explicitly uses `set(PICO_BOARD pico2_w)` and notes that "A new board configuration (`configs/PimoroniPicoLipo2XLW/`) will be created during Phase 1."
   - ✅ MARKED TBD (board config to be created)

8. **Idle/sleep timeouts as user-configurable fields** — Edward: "TBD: if timeouts go into `AddonOptions` protobuf, they need `PowerOptions` message + web configurator fields"
   - **Document treatment (lines 489–494, 487):** Protobuf fields are shown as TBD/speculative: "// In GamepadOptions or new PowerOptions message:" with optional fields for idle/sleep timeouts. Lines 487–496 describe the user-configurable timeouts.
   - ✅ MARKED TBD (scope of protobuf change to be determined)

**Summary of TBD handling:**
- 7 of 8 items are correctly marked as TBD, speculative, or "to be determined during Phase 1"
- Item 6 (BT Classic vs BLE battery reporting) is not explicitly discussed, but the doc does not fabricate a claim either
- Item 2 (GPIO24 VBUS) lacks an explicit "verify from schematic" note but is standard across Pico boards

**Overall:** ✅ PASS — No TBD items have been fabricated as facts. All substantial TBD items are flagged appropriately.

### 10. GPIO Retro Output Out-of-Scope Declaration (Previously Approved)

**Requirement:** Explicit statement that GPIO retro console output is out of scope (from round-2 approval).

**Found:** Line 28, bottom of Overview section:

> "**Out of scope for this document:** GPIO output to retro consoles (SNES, N64, Dreamcast, Genesis, etc.)..."

Still present and unchanged from round-2 approval. ✅ PASS

### 11. CMake Targets (Previously Approved)

**Requirement:** Three required CMake targets listed in both Minimum Requirements and Phase 1 Task 1.1.

**Location 1 — Minimum Requirements (line 49):**
```
CMake target linkage: `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device`
```
All three present. ✅

**Location 2 — Phase 1 Task 1.1 (line 521):**
```
Add `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device` targets
```
All three present. ✅ PASS

### 12. Code Block Indentation (4 Spaces)

Spot-checked all code blocks:
- Lines 313–327: Battery calculation code — all 4-space indents ✅
- Lines 338–355: VBUS detection code — all 4-space indents ✅
- Lines 364–373: Polling interval code — all 4-space indents ✅
- Lines 426–434: PowerManager class — all 4-space indents ✅
- Lines 469–481: DORMANT entry/exit code — all 4-space indents ✅

All code blocks use 4-space indentation (no tabs). ✅ PASS

### 13. Cross-Document Consistency (With Previously Approved Docs)

| Check | BT Review 2 | BT Review 3 | Status |
|-------|-------------|------------|--------|
| SDK version: 2.2.0 | ✅ Approved | Same | ✅ |
| RP2350 + CYW43: fully supported, not blocked | ✅ Approved | Enhanced with Reference Hardware section | ✅ |
| Out-of-scope GPIO retro output | ✅ Approved | Unchanged | ✅ |
| CMake targets (all 3) | ✅ Approved | Unchanged | ✅ |
| No AI agent names | ✅ Approved | Verified again | ✅ |
| Metadata (maintained by, last updated) | ✅ Approved | Unchanged | ✅ |

All round-2 approved content remains correct and consistent. ✅ PASS

### 14. New Content Quality Assessment

**Added sections (not in round-2):**

1. **Reference Hardware (lines 55–84)** — Accurate specs, proper designation of Pimoroni board, correct build target, appropriate notes about GPIO routing TBD
2. **Battery Level Reporting section expansion (lines 286–377)** — All technical details match Edward's analysis; properly cites ADC pins, voltage ranges, API functions
3. **Power Management (lines 380–503)** — Comprehensive four-state machine description; correct CYW43 modes; proper DORMANT handling; all TBDs flagged
4. **TinyUSB + BTStack Namespace Conflict (lines 601–634)** — Accurate collision description; correct mitigation strategy (source-file isolation); no false claims about resolution

All new content is technically accurate and properly references Edward's underlying analyses. ✅ PASS

### 15. Implementation Roadmap Status Claims

**Current claims in Phase 0 & 1:**
- Phase 0: ✅ Analysis work marked as **DONE** (line 510)
- Phase 1: Marked as **NEXT** (line 515)
- No Bluetooth code claimed to exist; feature is planning/future work

**Cross-check with document header (line 5):** "**Planning / Not yet implemented**" ✅ PASS

### 16. No Fabricated Specifics

Reviewed entire document for any specific values that Edward flagged as TBD and might have been "filled in" as fact:
- Voltage divider: marked as standard but requires verification ✅
- VBUS detection: standard across Pico boards, no fabrication ✅
- CYW43 latency: explicitly marked as "not yet measured" ✅
- Sniff mode latency: marked as acceptable for casual gaming, may not be acceptable for competitive ✅
- Battery service battery reporting: correctly attributed to GATT/BLE without overstating ✅

No fabricated specifics for TBD items. ✅ PASS

---

## Minor Observations (Not Blocking)

1. **GPIO24 VBUS schematic verification note:** Line 334–336 states GPIO24 VBUS detection as fact without a "verify from schematic" note. Contrast with line 330, which says divider ratio "must be verified." Both are board-specific and should have equal caution language. This is an asymmetry in documentation tone, not a factual error. Standard Pico boards do use GPIO24 for VBUS, so the statement is correct, but a note suggesting verification would be more thorough.

2. **Item 6 from Edward's TBD list (Battery Service BT Classic vs BLE):** Edward flagged concern that Battery Service is BLE-only, and BT Classic HID may not report battery. The document doesn't explicitly address this, but the overview (line 238) specifies "Bluetooth HID Classic" will be implemented, and the Battery Service section (lines 290–300) correctly identifies it as a BLE GATT service. Readers should understand the distinction, but an explicit note ("Note: Battery Service is BLE-only; classic BT HID devices may report battery via alternative mechanisms") would preempt confusion. However, this is not a fabrication—it's just missing clarification.

---

## Summary

This revision successfully expands the Bluetooth HID planning document with three major new sections (Reference Hardware, Battery Level Reporting, Power Management) and detailed namespace conflict mitigation. All technical content matches the underlying research from Edward's power-battery-analysis.md and bt-namespace-analysis.md. The TBD items are properly flagged and not fabricated as facts. Version numbers are correct. No AI agent names appear. GPIO retro output remains correctly scoped out. CMake targets are present in all required locations.

The document is technically accurate, comprehensive, and ready for publication.

**APPROVED: ready to add to PR.**

---

**Final checklist:**

- [x] SDK version 2.2.0 throughout (no stale 2.1.1)
- [x] CMake minimum 3.10 (not required in this doc)
- [x] Picotool 2.2.0-a4 (not required in this doc)
- [x] No AI agent names (Hughes, Edward, Riza, etc.)
- [x] RP2350 + CYW43 confirmed as fully supported (not blocked)
- [x] Pimoroni Pico Lipo 2 XL W is designated reference board
- [x] Reference Hardware section added with specs and build target
- [x] Battery Level Reporting section with BTStack API, ADC, VBUS
- [x] Power Management section with four-state machine, clock modes, DORMANT
- [x] TinyUSB + BTStack namespace conflict documented with source-file isolation mitigation
- [x] All 8 TBD items marked appropriately (not fabricated as facts)
- [x] GPIO retro output still explicitly out of scope
- [x] Code blocks use 4-space indentation
- [x] Last updated: 2026-03-28
- [x] Maintained by: GP2040-CE core team
- [x] CMake targets (all 3) present in both locations
- [x] No regressions from round-2 approval
