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

### 2026-03-28T024429: Round-2 Doc Fixes

Fixed 3 issues flagged by Riza in review round 2: wrong file reference in `rp2350-support.md`, a stale note carried over from Hughes's original draft, and an incorrect year in a document header. Commit `1c910869`. Note: Mustang is locked out from further revisions of these docs during this cycle (reviewer lockout protocol — Winry assigned for round-3 fixes).

### 2026-03-28T024429: Copilot Instructions "Maintained By" (cross-agent update from Scribe)

The `copilot-instructions.md` file that Mustang created originally included "Roy Mustang, Project Lead" in the Maintained-by field. Riza flagged this in round 5 as an internal AI agent name in a public document. Fortinbra corrected it to "GP2040-CE core team". **Pattern:** Never include Squad agent names in public-facing documentation — attribute to the project team instead.

### Copilot Instructions (2025-03-28)
- Created `.github/copilot-instructions.md` on branch `docs/copilot-instructions` (commit `6308f5c0`)
- Documented comprehensive guidance for GitHub Copilot contributions
- Locked Pico SDK version to 2.1.1 as standard
- **Critical branching policy:** Never commit directly to main or upstream; always use feature branches
- Indentation standard: 4 spaces only, never tabs — enforced across all file types
- Covers code style (C/C++, CMake), platform config, build system, project structure, git workflow, documentation, PR guidelines, testing, performance, and hardware constraints

### 2026-03-29T125242: BLE HID Implementation Code Review — APPROVED

**Context:** Edward completed BLE HID implementation audit and fixed 6 critical bugs. Requested architectural review before hardware testing.

**Verdict:** **APPROVED** for hardware testing on Pimoroni Pico Lipo 2 XL W.

**Key Findings:**
1. **Architecture integrity:** BLEHIDManager correctly mirrors BTHIDManager patterns. Singleton, deferred init, CYW43 lifecycle, BTstack run loop, same HID descriptor. Protocol-specific differences (GATT vs SDP, hids_device vs hid_device) are correct.

2. **Namespace isolation:** TinyUSB/BTstack collision avoided correctly. No btstack.h in headers, only forward declarations. All BTstack includes in .cpp files only. Compiled cleanly for both Pico and Pico W.

3. **OutputManager integration:** Mode detection logic correct. useBLE/useClassic scoping verified. Only one manager initialized based on InputMode. HID report packing matches USB HIDDriver pattern.

4. **CMakeLists.txt:** PICO_CYW43_SUPPORTED guard correctly isolates BT to wireless boards. BLE library linkage correct (pico_btstack_ble, pico_btstack_flash_bank). GATT service sources from Pico SDK. Non-wireless builds safe.

5. **Proto definitions:** INPUT_MODE_BLE = 18 valid, no collision. BluetoothOptions structure valid but has unused fields (documented issue).

6. **Dead code identified (LOW PRIORITY):** bt_config_bridge.cpp BLE key save/load functions are never called. BTstack's le_device_db_tlv handles bonding internally via TLV flash storage. Proto fields bleBondedAddr, bleIdentityResolvingKey, bleLongTermKey will never be populated. Impact: zero runtime bugs, just vestigial code from planning phase. Recommendation: add comments explaining this, defer cleanup to future PR.

**Edward's 6 bug fixes were all correct:**
- Namespace collision fix ✅
- Removed non-existent SM event functions ✅
- Removed conflicting ATT callbacks ✅
- Fixed flash bank API to pico_flash_bank_instance() ✅
- Added missing btstack_config.h defines (ENABLE_LE_PERIPHERAL, etc.) ✅
- Added INPUT_MODE_BLE_NAME to MainMenuScreen.h ✅

**Recommendations:**
- Edward: Add comments to dead code in bt_config_bridge.cpp and config.proto explaining BTstack TLV handles BLE bonding
- Hughes: Document in bluetooth-support.md that BLE bonding keys are NOT visible in web configurator (different from BT Classic)
- Future (Phase 4): Optional enhancement to expose BTstack TLV bonded device list in web configurator via read-only API

**Pattern learned:** BTstack BLE bonding via le_device_db_tlv is fully automatic — no manual key extraction needed (unlike BT Classic). GATT services from #import <hids.gatt> are self-contained — no custom ATT callbacks. Always use pico_flash_bank_instance() for Pico SDK flash storage, not generic BTstack examples.

**Next phase:** Hardware testing on Pico Lipo 2 XL W to validate pairing, HID reports, and auto-reconnect.

**Review document:** .squad/decisions/inbox/mustang-ble-review.md
