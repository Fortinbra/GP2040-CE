# Orchestration Log — Edward (Firmware Dev)

**Session:** 20260330T033352Z  
**Agent:** Edward

## Decisions Logged

### 1. BLE HID Documentation Revision — 6 Blockers Fixed
**Status:** ✅ Complete  
**Timestamp:** 2026-03-29  
**Commit:** ff1fa570  
**Branch:** feature/ble-hid-v2

Fixed all 6 blocking issues from Riza's Round 1 review of `docs/development/ble-hid-support.md`:

**Issues Fixed:**

1. **SDK version:** Changed `2.2.0+` to `2.2.0` exactly throughout document (ground truth: CMakeLists.txt line 7)
2. **TLV configure signature:** Corrected to 2-arg form `le_device_db_tlv_configure(tlv_impl, &tlv_context)` with `tlv_impl` from TLV flash bank init
3. **TLV flash init call:** Corrected to `btstack_tlv_flash_bank_init_instance(&tlv_context, pico_flash_bank_instance(), NULL)` (no file path parameter)
4. **BLEHIDReport struct:** Removed `report_id` field, applied `__attribute__((packed))`, verified 9-byte layout
5. **GATT database:** Replaced fabricated `gatt_char_t` C struct with correct `.gatt` DSL file format + CMake build snippet
6. **Battery service API:** Changed to correct `battery_service_server_set_battery_value(uint8_t)` pattern (no separate init call)

**Additions:**
- Added "Critical API Checklist" subsection with all 6 blockers as implementer checkboxes

**Non-blocking Items (deferred per Riza):**
- `att_server_client_is_subscribed` API name verification
- Phase dependency clarification (BLE Phase 1 vs Classic Phase 1 sequencing)
- TinyUSB/BTstack pitfall alignment
- CCCD subscription check placement in positive code path

---

### 2. BLE HID Documentation Polish — Duplicate setupSM() Call Removed
**Status:** ✅ Complete  
**Timestamp:** 2026-03-29  
**Commit:** fe041066  
**Branch:** feature/ble-hid-v2

Riza's re-review identified non-blocking issue: duplicate `le_device_db_tlv_configure()` call in `setupSM()` with `tlv_impl` out of scope (declared locally in `setupBLE()` only).

**Fix:** Removed duplicate call from `setupSM()`. Correct pattern retained in `setupBLE()` (line 142) and pitfall #1 example (line 653). TLV configuration happens once per boot in `setupBLE()`; subsequent `sm_init()` call uses the configured context implicitly.

**Impact:** Document now matches exact implementer code path — no scope errors, no duplicate configuration.

---

### 3. Phase 1 Implementation Ready
**Status:** Ready to begin  
**Timestamp:** 2026-03-29

Document revision complete and approved by Riza. All critical BTstack API patterns documented correctly:
- TLV initialization (proper 3-arg and 2-arg calls)
- GATT database (`.gatt` DSL + CMake compilation)
- Report structure (correct 9-byte layout)
- Battery service (correct update pattern)
- SMP/pairing (SM_AUTHREQ_BONDING + SECURE_CONNECTIONS)
- Advertising timing (HCI_STATE_WORKING guard)
- Platform linkage (CMake target list)
- All 10 pitfalls with correct patterns

Edward cleared to begin Phase 1 firmware implementation.
