# BT Review 4 — `docs/development/bluetooth-support.md`

**Reviewer:** Riza Hawkeye  
**Date:** 2026-03-28  
**Revision author:** Fortinbra (commit `09daa13b`)  
**Document reviewed:** `docs/development/bluetooth-support.md` (full file, lines 1–737)

---

## VERDICT: APPROVED ✅

All 7 verification points passed. The battery reporting mechanism has been correctly updated from GATT (BLE-only) to HID descriptor (BT Classic). The forward-looking BLE note is properly scoped and isolated. No prior approved content was disturbed. The document is accurate and ready for publication.

---

## Verification Checklist

### 1. GATT APIs Removed from Battery Section ✅

**Requirement:** `battery_service_server_init()` and `battery_service_server_set_battery_value()` must NOT appear in the active battery section (lines 286–404).

**Findings:**
- Old code references to `battery_service_server_set_battery_value()` have been completely removed from:
  - VBUS detection section (lines 364–384): No GATT API calls in charging logic
  - Polling interval section (lines 387–403): Callback now updates `auxState` directly instead of calling GATT setter
- Both APIs appear ONLY in the "Future: BLE HID Battery Service" note (line 407), explicitly scoped to "future phase"
- String search confirms: No GATT APIs outside the future-phase note

✅ PASS

### 2. Correct BT Classic HID Mechanism Documented ✅

**Requirement:** HID descriptor path with Usage Page 0x06, Usage 0x20, and `hid_device_register_report_request_callback()`.

**Findings:**
- **HID Descriptor subsection (lines 290–336):**
  - Usage Page: `0x06` ✅ (line 304)
  - Usage: `0x20` ✅ (line 305)
  - Report Type: Feature ✅ (line 296)
  - Descriptor bytes include full block ✅ (lines 303–310)
  - GET_REPORT mechanism documented ✅ (line 313: "GET_REPORT(Feature, report_id=0x02)")
  - L2CAP PSM 0x0011 confirmed ✅ (line 313)

- **BTStack API subsection (lines 315–334):**
  - `hid_device_register_report_request_callback()` call present ✅ (line 318)
  - Callback signature correct ✅ (lines 321–325)
  - HID_REPORT_TYPE_FEATURE check present ✅ (line 326)
  - Report ID 0x02 check present ✅ (line 326)
  - VBUS check and battery response logic present ✅ (line 327–328)
  - Return codes documented ✅ (lines 330–332)

- **SDP registration note (lines 336):**
  - Mentions both `hid_device_init()` and `hid_sdp_record_t` ✅
  - Correctly states descriptor is passed to both ✅

✅ PASS

### 3. ADC Hardware Unchanged ✅

**Requirement:** GPIO29, 3:1 voltage divider, 3.0–4.2V LiPo range still present.

**Findings:**
- **GPIO29:** Line 340 confirms "**GPIO29 (ADC3)**" ✅
- **Voltage divider 3.0:** Line 345 shows `BATT_DIVIDER = 3.0f` ✅
- **Voltage range:** Lines 346–347 show `BATT_MIN_V = 3.0f` and `BATT_MAX_V = 4.2f` ✅
- **200kΩ / 100kΩ notation:** Lines 345 inline comment states "200kΩ / 100kΩ divider" ✅
- **ADC calculation:** Lines 352–356 contain unchanged voltage-to-percentage math ✅
- **Schematic verification note:** Line 360 advises "Verify this value from your board's schematic" ✅
- **Non-linearity note:** Line 362 notes LiPo non-linearity and lookup table option ✅

✅ PASS

### 4. VBUS Detection Unchanged ✅

**Requirement:** GPIO24 still documented with proper logic.

**Findings:**
- **GPIO24 confirmation:** Line 366 states "**GPIO24** reads HIGH (via VBUS sense)" ✅
- **Logic in code (lines 369–382):**
  - `gpio_get(24)` check present ✅
  - USB charging → report 100% ✅
  - `gamepad->auxState.power.pluggedIn` set correctly ✅
  - `gamepad->auxState.power.charging` set correctly ✅
  - `gamepad->auxState.power.level = 100` when USB ✅
  - ADC read in battery-only path ✅
- **Callback logic in HID descriptor section (lines 327–328):**
  - `bool usb = gpio_get(24)` ✅
  - `*out_report = usb ? 100 : readBatteryPercent()` ✅

✅ PASS

### 5. Forward-Looking BLE Note Properly Scoped ✅

**Requirement:** GATT Battery Service (UUID 0x180F) appears ONLY in a future-phase BLE note, clearly separated from the BT Classic section.

**Findings:**
- **New subsection "Future: BLE HID Battery Service" (lines 405–407):**
  - Titled explicitly as "Future:" ✅
  - Condition stated: "If BLE HID mode is added in a future phase" ✅
  - Parenthetical: "(in addition to or instead of BT Classic)" clarifies it's a variant, not immediate ✅
  - UUID 0x180F documented ✅
  - Characteristic UUID 0x2A19 documented ✅
  - Both GATT APIs mentioned with full function names ✅
  - Clarification: "This is a BLE GATT construct and has no effect over a BT Classic HID connection" ✅
  - ADC unchanged note: "The ADC reading logic and VBUS detection would remain unchanged" ✅
  - Transport note: "only the delivery mechanism (HID descriptor vs. GATT) would differ" ✅

- **Separation:** Future note is clearly separated from the BT Classic battery section by a horizontal rule (line 404) ✅

✅ PASS

### 6. Nothing Else Disturbed ✅

**Requirement:** Power Management, Reference Hardware, namespace conflicts, GPIO retro out-of-scope declaration all intact.

**Findings:**
- **Power Management section (lines 411–503):** Completely unchanged from round-3 approval ✅
  - Four-state machine present ✅
  - Clock speeds documented ✅
  - CYW43 PM modes intact ✅
  - DORMANT entry/exit sequence intact ✅
  
- **Reference Hardware section (lines 55–84):** Completely unchanged from round-3 approval ✅
  - Pimoroni Pico Lipo 2 XL W designated as reference board ✅
  - RP2350B + CYW43 specs intact ✅
  - GPIO availability noted ✅
  - Build target correct ✅

- **TinyUSB + BTStack Namespace Conflict section (lines 632–667):** Unchanged from round-3 approval ✅
  - Conflict described with enum comparison ✅
  - Source-file isolation mitigation documented ✅
  - No false resolution claims ✅

- **GPIO retro output out-of-scope (line 28):** Unchanged from round-3 approval ✅
  - Statement: "Out of scope for this document: GPIO output to retro consoles..." ✅

✅ PASS

### 7. Version Accuracy, No Agent Names, Metadata Correct ✅

**Requirement:** Same standards as prior reviews — SDK 2.2.0, no Squad member names, clean metadata.

**Findings:**
- **SDK version:** Line 6 states "**2.2.0+**" ✅
- **Last updated:** Line 3 shows "**2026-03-28**" ✅
- **Maintained by:** Line 4 shows "**GP2040-CE core team**" (no agent names) ✅
- **Status header:** Line 5 shows "**Planning / Not yet implemented**" ✅
- **Agent name search:** No internal Squad members (Hughes, Edward, Riza, Mustang, Winry, Fortinbra) found in public text ✅
- **Code block indentation:** Spot-checked battery descriptor (lines 303–310), callback (lines 321–333), ADC logic (lines 350–356), VBUS logic (lines 369–382) — all 4 spaces, no tabs ✅

✅ PASS

---

## Diff Review Summary

**Commit:** `09daa13b`  
**Author:** Fortinbra  
**Changes:** 53 insertions, 22 deletions in `docs/development/bluetooth-support.md`

### Key Changes (All Correct)

1. **Battery section title changed:** "Bluetooth Battery Service" → "HID Descriptor Battery Strength Feature Report" — accurately reflects the corrected mechanism
2. **GATT description removed:** Old text explaining UUID 0x180F, 0x2A19, and `battery_service_server_*` APIs removed from active section
3. **HID descriptor added:** Complete 8-byte descriptor block with Usage Page 0x06/Usage 0x20 added (lines 301–311)
4. **Callback registered:** `hid_device_register_report_request_callback()` now documented with full callback implementation (lines 316–333)
5. **VBUS logic corrected:** Two calls to `battery_service_server_set_battery_value()` removed; USB logic now updates `auxState.power` directly
6. **Polling interval rationale updated:** Old text about "flooding GATT notifications" replaced with correct statement that "host decides when to poll"
7. **Future BLE note added:** New subsection (lines 405–407) scopes GATT Battery Service to future BLE phase only

### No Regressions

- ADC calculation logic preserved ✅
- VBUS detection logic preserved ✅
- `GamepadAuxPower` struct integration intact ✅
- Reference Hardware section untouched ✅
- Power Management section untouched ✅
- Namespace conflict section untouched ✅

---

## Ground Truth Cross-Check

Verified against Edward's authoritative analysis (`bt-battery-protocol-analysis.md`):

| Element | Edward's Fact | Document says | Status |
|---------|---------------|---------------|--------|
| BT Classic battery transport | HID descriptor Feature report | Lines 290–313: Correct ✅ | ✅ |
| Usage Page / Usage | 0x06 / 0x20 | Lines 294–295: Correct ✅ | ✅ |
| Report type | Feature (responds to GET_REPORT) | Line 296: Correct ✅ | ✅ |
| BTStack API | `hid_device_register_report_request_callback()` | Line 318: Correct ✅ | ✅ |
| GATT Battery Service scope | BLE only; NOT for Classic HID | Line 407: "would then be the correct mechanism for battery reporting over BLE" ✅ | ✅ |
| `battery_service_server_*` calls | Dead code in Classic HID | Removed from active path ✅ | ✅ |
| ADC hardware | GPIO29, 3.0V–4.2V LiPo, 3:1 divider | Lines 340–347: Unchanged ✅ | ✅ |
| VBUS detection | GPIO24 HIGH = USB | Line 366: Unchanged ✅ | ✅ |

---

## Edge Cases Checked

1. **Does commit `09daa13b` reference the wrong author?**  
   No. Author shown as Fortinbra. This is correct per task description.

2. **Is this a re-revision after Edward's analysis, or Hughes's first pass?**  
   This is Fortinbra's correction of Hughes's prior round-3 content. Hughes wrote the round-3 battery section with GATT APIs; Fortinbra corrected it based on Edward's protocol analysis. Hughes is locked out of revising his own work per governance.

3. **Does the BLE future note mention any near-term timeline?**  
   No. Correctly says "if...in a future phase." No timeline given. ✅

4. **Are there any remaining references to BLE APIs in the active (non-future) battery section?**  
   No. Only appear in line 407 "Future" note. ✅

5. **Does the document still claim Bluetooth is implemented?**  
   No. Line 5 says "Planning / Not yet implemented." Line 238 says same. ✅

6. **Is the CMake target list updated to match the correction?**  
   Not addressed in this commit (battery section focus). CMake targets mentioned line 49 remain as-is (inherited from round-3). Not a regression; likely a separate PR.

---

## Minor Observations (Not Blocking)

None. All content is accurate, well-scoped, and ready for publication.

---

## Summary

Commit `09daa13b` successfully corrects the battery reporting mechanism from GATT (incorrectly applied to BT Classic HID) to the correct HID descriptor path (Feature report, Usage 0x06/0x20, `hid_device_register_report_request_callback()`). The GATT Battery Service is properly relegated to a future-phase note for potential BLE support, with clear language that it has "no effect over a BT Classic HID connection." All ADC hardware, VBUS detection, and power management content remains unchanged and correct. No new issues introduced. The document is technically accurate and ready for PR.

---

## Final Checklist

- [x] ✅ GATT Battery Service APIs removed from active battery section
- [x] ✅ HID descriptor path documented (Usage 0x06, Usage 0x20, Feature report)
- [x] ✅ `hid_device_register_report_request_callback()` callback documented with correct signature
- [x] ✅ GET_REPORT mechanism documented (L2CAP PSM 0x0011)
- [x] ✅ ADC hardware unchanged (GPIO29, 3:1 divider, 3.0–4.2V)
- [x] ✅ VBUS detection unchanged (GPIO24, report 100% when USB connected)
- [x] ✅ Forward-looking BLE note added, clearly scoped to future phase
- [x] ✅ GATT Battery Service UUIDs documented ONLY in future note
- [x] ✅ Power Management section untouched
- [x] ✅ Reference Hardware section untouched
- [x] ✅ Namespace conflict section untouched
- [x] ✅ GPIO retro output out-of-scope declaration intact
- [x] ✅ No internal AI agent names in public text
- [x] ✅ SDK version 2.2.0+
- [x] ✅ Metadata: Last updated 2026-03-28, Maintained by GP2040-CE core team
- [x] ✅ Code blocks: 4-space indentation, no tabs
- [x] ✅ Ground truth matches Edward's bt-battery-protocol-analysis.md
- [x] ✅ No fabricated TBD items as facts
- [x] ✅ No regressions from round-3 approval

---

**APPROVED: Ready to add to PR.**
