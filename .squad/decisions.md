# Squad Decisions

## Active Decisions

### 2026-03-28T015800: Copilot instructions created
**By:** Roy Mustang  
**What:** Created `.github/copilot-instructions.md` covering code style (4-space indent, no tabs), Pico SDK 2.1.1, build config, project structure, and strict branching policy (no direct main commits, no upstream commits).  
**Why:** Requested by Fortinbra to guide Copilot contributions and establish consistent coding standards for AI-assisted development.

### 2026-03-28T015800: User directive
**By:** Fortinbra (via Copilot)  
**What:** All changes must be made on a branch. Never commit directly to main. Absolutely never commit to upstream.  
**Why:** User request — captured for team memory

### 2026-03-28T021300: Dependency management doc created
**By:** Maes Hughes
**What:** Created docs/development/dependency-updates.md covering all GP2040-CE dependencies (firmware + web configurator) with update procedures and compatibility notes.
**Why:** Feature doc requested by Fortinbra before PR

### 2026-03-28T024429: SDK version 2.2.0 confirmed as project-wide ground truth

**By:** Edward  
**What:** CMakeLists.txt line 7 sets `sdkVersion 2.2.0` and lines 62-63 enforce a FATAL_ERROR if SDK < 2.2.0. CI (`cmake.yml`) checks out pico-sdk at tag `2.2.0` explicitly. This is the project-wide minimum for ALL builds — RP2040 and RP2350 alike. All documentation must reference 2.2.0, not 2.1.1.  
**Impact:** `copilot-instructions.md`, `rp2350-support.md`, and `dependency-updates.md` all updated to 2.2.0. Picotool version confirmed as `2.2.0-a4` (exact CMakeLists.txt value).  
**Why:** Riza review rejection — SDK version conflict between docs and copilot-instructions.md required resolution. Ground truth is the build system and CI, not developer guidance docs.

### 2026-03-28T024429: All internal AI agent names removed from public-facing docs

**By:** Fortinbra  
**What:** Internal Squad agent names (e.g., "Roy Mustang", team member names) must not appear in any public-facing documentation. Maintainer fields should attribute "GP2040-CE core team" or similar project-level attribution.  
**Why:** Riza review round 4 & 5 — AI tooling references are inappropriate in public docs. Governance decision by Fortinbra.

### 2026-03-28T021300: RP2350 support doc created

**By:** Maes Hughes (from Edward's analysis)

**What:** Created `docs/development/rp2350-support.md` covering:
- Hardware comparison table (RP2350A vs RP2350B)
- Supported boards and their configurations
- Minimum requirements and Pico SDK version
- Build instructions for pre-built releases and from source
- Pin mapping and GPIO validation
- Complete guide for creating custom RP2350 board configurations
- Migration path for users upgrading from RP2040
- Known limitations and testing notes

**Finding:** RP2350 support is already partially implemented in the codebase:
- Three board configs exist and are CI-tested: `Pico2`, `FlatboxRev8`, `SparkFunProMicroRP2350`
- Firmware source code is platform-agnostic (zero `#ifdef` guards for chip-specific logic)
- GPIO validation uses `NUM_BANK0_GPIOS` SDK macro—automatically adapts at compile time
- Web configurator auto-detects GPIO count from running firmware

Documented gaps:
1. **Pico2W config missing** — Raspberry Pi Pico 2 W (RP2350A + WiFi) not yet supported
2. **No RP2350B GPIO 30–47 configs** — RP2350B capabilities documented but no board configs currently leverage extra pins
3. **PIO USB timing** — Tested in CI but hardware pass-through testing recommended at 150 MHz clock

**Why:** Feature doc requested by Fortinbra to explain RP2350 support to end users and guide board creators on custom configurations.

**Commit:** `bf3d2f4d` (docs/copilot-instructions branch)

### 2026-03-28T024429: Bluetooth and Multi-Output Architecture — Constraints from Edward's Survey

**By:** Edward  
**What:** Deep codebase survey identified 8 architectural constraints and findings for multi-output support:
1. GPDriver is USB-only; output transport abstraction needed above it
2. No runtime output switching exists; mode selection is boot-time only
3. USB HID and CYW43 Bluetooth CAN coexist on PicoW (separate hardware)
4. GPIO retro console OUTPUT does not exist (only INPUT adapters present)
5. Zero Bluetooth code in firmware; feature is entirely new
6. Pico2W (RP2350 + CYW43) missing pending CYW43 wireless stack porting to RP2350
7. Output transport abstraction layer required before BT or GPIO can be peer output modes
8. InputMode enum is source of truth for output modes; new mode requires protobuf change + web configurator cascade

**Why:** Requested by Fortinbra as technical foundation for Bluetooth feature planning documentation. Findings form the basis for Hughes's feature doc and Edward's final approval by Riza.

### 2026-03-28T024429: GPIO Output Architecture Constraints

**By:** Edward (via survey for Hughes's GPIO output feature doc)  
**What:** Deep technical survey of GPIO retro console OUTPUT mode established the following architectural constraints and decisions:

1. **GPIOOutputAddon is a `GPAddon` subclass, not a `GPDriver`** — enables simultaneous USB HID + GPIO retro output. GPIO output runs in `addon->process()` after USB report is sent per main loop iteration.

2. **No new `InputMode` enum value required** — GPIO output protocol is selected within the addon config, not at the DriverManager/InputMode level. Avoids cascade changes to protobuf, DriverManager, and web configurator mode selector.

3. **Level shifting is mandatory for NES, SNES, Genesis/MD, TurboGrafx-16/PC Engine** — these consoles use 5V TTL. RP2040/RP2350 GPIO inputs are NOT 5V tolerant. Hardware requirement: 74AHCT125 (output) + 74LVC245 or resistor divider (input). This is a hardware design constraint to document.

4. **N64 and Dreamcast are 3.3V native** — no level shifting needed. RP2040/RP2350 GPIO compatible out of the box.

5. **N64 output requires PIO — non-negotiable** — 1 MHz NRZ serial, 500 ns/bit tolerance. Bit-bang and interrupt approaches fail at this tolerance. One PIO state machine needed.

6. **Dreamcast MAPLE bus requires PIO — non-negotiable** — ~2 Mbps, bidirectional, CRC-checked frames, <200 µs response window. Two PIO SMs needed (TX + RX). Most complex retro console target. No Dreamcast code exists in the project today.

7. **SNES/NES output: interrupt-driven is viable, PIO is optional** — timing window is 6–12 µs per clock edge. GPIO IRQ response latency on RP2040 is typically <1 µs. Both approaches achievable; PIO provides jitter-free guarantee.

8. **Genesis/MD and TurboGrafx-16: bit-bang fully sufficient** — parallel protocols with slow console polling rates (60 Hz). No PIO needed.

9. **PIO state machines are not a bottleneck** — RP2040 has 8 total. Worst case (PIO-USB passthrough + WS2812 + N64 + Dreamcast) = 8 SMs fully consumed. More typical retro-output board (no USB passthrough) uses at most 4. RP2350B has 12 SMs.

10. **Pico W loses GPIO 23–25 to CYW43** — pins are used for CYW43 SPI/SDIO bus. Only GPIO 22, 26, 27, 28 are free candidates (4 pins). Sufficient for NES/SNES/N64/TG16/Dreamcast; tight for Genesis 6-button (needs 9 signal pins).

11. **SNES protocol timing ground truth** — from `lib/SNESpad/SNESpad.cpp`: Latch HIGH = 12 µs, setup delay = 6 µs, clock LOW = 6 µs, clock HIGH = 6 µs. Frame = 16 bits (SNES) / 8 bits (NES). Data active-low.

12. **New protobuf fields needed** — `GPIOOutputOptions` message in `proto/config.proto` with fields: enabled, protocol enum, per-protocol pin assignments. Wire into `AddonOptions` alongside existing `SNESOptions`, `TG16Options`.

**Why:** Requested by Fortinbra as technical foundation for Hughes's GPIO output feature documentation. Full analysis in `.squad/agents/edward/gpio-analysis.md`.

### 2026-03-28T04:14: RP2350 + BTStack confirmed working

**By:** Fortinbra  
**What:** RP2350 with CYW43 wireless fully supports BTStack — confirmed by Fortinbra who has built a working BT controller project on the Pimoroni Pico Lipo 2 XL W (RP2350 + CYW43). The previous note that RP2350 CYW43 BT was blocked/unverified is incorrect and must be removed from docs.  
**Also:** TinyUSB and BTStack have conflicting namespaces — this must be documented as a known integration concern in bluetooth-support.md.  
**Why:** User-confirmed hardware fact — supersedes prior analysis uncertainty.

### 2026-03-28T04:20: Pimoroni Pico Lipo 2 XL W as BT reference board

**By:** Fortinbra  
**What:** The Pimoroni Pico Lipo 2 XL W (RP2350 + CYW43 + onboard LiPo charger) is the designated reference board for initial Bluetooth development and testing.  
**Why:** User-confirmed working BT target with battery hardware present.

### 2026-03-28T04:20: BT must include battery level reporting

**By:** Fortinbra  
**What:** Bluetooth feature must include battery level reporting (BT HID Battery Service, UUID 0x180F). Battery percentage must be reported to the connected host over BT.  
**Why:** Wireless devices must report battery state; this is expected by all modern OSes.

### 2026-03-28T04:20: Power management required for BT/battery builds

**By:** Fortinbra  
**What:** GP2040-CE must implement power management for wireless (battery-powered) builds. USB builds always have VBUS so power management was never needed. BT+battery builds require: sleep/dormant modes when idle, CYW43 radio power saving, VBUS detection to switch power profiles between USB and battery operation.  
**Why:** First time the firmware must manage its own power budget — foundational for any battery-powered use.

### 2026-03-28T17:05: User directive — RM2 module support for custom boards (future feature)
**By:** Fortinbra (via Copilot)
**What:** Future feature — custom RP2040/RP2350 boards should be able to use the Pimoroni RM2 module (CYW43439 standalone wireless module) for BT/WiFi, provided they wire the RM2 to the same GPIO pins that Pico W / Pico 2 W use for their onboard CYW43. This enables BT on custom boards without requiring a Pico W form factor.
**Reference:** https://shop.pimoroni.com/products/rm2-breakout?variant=53492995719547
**Why:** Expands wireless capability to the broader ecosystem of custom GP2040-CE boards.

### 2026-03-28T234414: Deferred Dependency Upgrade Planning Documentation

**By:** Maes Hughes (authored), Riza (approved)  
**What:** Four feature planning documents created to guide future contributors through deferred dependency upgrades that require significant manual effort:

1. **tinyusb-upstream-port.md** — Plan to port TinyUSB fork (0.17.0 + 18 commits) to upstream 0.20.0
2. **pico-pio-usb-upstream-port.md** — Plan to port pico-pio-usb fork (0.5.3 + 10 commits) to upstream 0.7.2
3. **nanopb-stable-migration.md** — Plan to migrate vendored nanopb from 0.4.8-dev to 0.4.8 stable
4. **npm-major-upgrades.md** — Five-phase roadmap for npm major version upgrades in web configurator

**Technical Facts Verified:**
- SDK 2.2.0 referenced throughout (matches squad decision 2026-03-28T024429)
- TinyUSB: 0.17.0 base + 18 custom commits verified; upstream 0.20.0 correct
- pico-pio-usb: 0.5.3 base + 10 custom commits verified; upstream 0.7.2 correct
- nanopb: 0.4.8-dev confirmed as vendored snapshot (not submodule)
- npm packages: All version pairs match confirmed facts (React 18→19, Vite 4→8, react-router-dom 6→7, ESLint 8→9, TypeScript 5→6, Zustand 4→5, protobufjs-cli 1→2)

**Standards Compliance:**
- No internal Squad agent names in any document (uses "GP2040-CE core team" only)
- Documentation complete with problem statements, inventories, goals, phased approaches, risk analysis, and testing requirements
- Ready for merge to main

**Why:** Automated dependency audit identified deferred items that could not be automatically updated due to manual effort requirements. Without clear documentation, these items risk being forgotten, tackled haphazardly, or duplicated. These docs provide clear roadmaps, timelines, and success criteria.

**Commit:** `49bd6797` (feature/dependency-updates branch)

### 2026-03-29T201441: I2C Peripheral Expansion & HID over I2C — Technical Accuracy Verified

**By:** Edward (Firmware Dev)  
**What:** Comprehensive technical accuracy review of two I2C feature planning documents:
- `docs/development/i2c-peripheral-expansion.md` — GP2040-CE as I2C master to satellite MCUs
- `docs/development/hid-over-i2c.md` — GP2040-CE as I2C slave to host (e.g., Raspberry Pi)

**Findings:** Both docs architecturally feasible. 6 critical SDK/API corrections required:
1. **i2c_slave_init() does not exist** — Use `i2c_set_slave_mode()` + manual IRQ handler via `irq_set_exclusive_handler()`
2. **No high-level I2C slave callback API in Pico SDK** — Callback pattern (I2C_SLAVE_RECEIVE, etc.) must be implemented manually by inspecting hardware registers
3. **Proto field number miscalculation** — Next available is 32 (fields 28–31 occupied), not "28+"
4. **GPIO open-drain emulation** — RP2040 has no true open-drain; use direction-toggle (GPIO_IN to release) or output-enable override
5. **Atomic primitives unavailable** — C11 `_Atomic` and `__atomic_store` not suitable for RP2040; use `critical_section_t` or `spin_lock_t`
6. **I2C timing** — 530µs claim is slightly high; actual ~500µs at 400 kHz (minor correction)

**Impact:** All corrections applied by Hughes on develop branch (commit cde93e8b). Documents now accurate and implementation-ready.

**Why:** Technical accuracy review requested by Fortinbra to validate planning docs before implementation phase.

### 2026-03-29T201441: I2C Documentation Quality Review — Consistency APPROVED

**By:** Riza (QA)  
**What:** 10-point consistency review across `i2c-peripheral-expansion.md` and `hid-over-i2c.md`:
- Terminology (master/slave, controller/target usage)
- Mutual cross-references and architectural relationship clarity
- Hardware resource conflicts (I2C bus allocation: i2c0 vs i2c1)
- Code block formatting and language identifiers
- Clarity and "why" explanations for all architectural decisions
- Limitations and caveats (explicitly stated, not buried)
- Phase breakdown consistency (1/2/3 alignment with BLE HID baseline)
- Squad decision alignment (USB-as-primary-output, SDK version 2.2.0, no internal agent names)

**Verdict:** ✅ APPROVE WITH NOTES. No blocking issues. Minor formatting note (1 code block missing language identifier in hid-over-i2c.md — cosmetic, fixed by Hughes).

**Why:** Quality assurance review to ensure both docs meet project documentation standards before merge.

### 2025-01-27: BLE Bonding Persistence — Optimistic Reconnect Gate

**By:** Edward (BLE/BTstack firmware engineer)  
**What:** Implemented bonded device reconnection optimizations in `src/BLEHIDManager.cpp`:
1. `SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED` (BTstack confirms peer IRK match) → set `_notificationsEnabled = true` optimistically. Bonded hosts cache CCCD subscription server-side; this gate ensures reports flow immediately on reconnect without waiting for host CCCD rewrite.
2. `SM_EVENT_PAIRING_COMPLETE` → set `_hasBondedPeers = true` for public API immediately.
3. At startup, check `le_device_db_count() > 0` after `le_device_db_tlv_configure()` to populate `_hasBondedPeers`.
4. Restored proper `_notificationsEnabled` guard in `process()` and `sendReport()` (removed nuclear debugging bypass).
5. Added public APIs: `isNotifying()` and `hasBondedPeers()`.
6. Declared `_hasBondedPeers` volatile (written from both IRQ and main context).

**Safety:** IRK resolution only succeeds with real long-term key material from bonded database — not a security regression.

**Why:** Power-cycle reconnect to Windows/macOS: bonded peers cache CCCD, no re-subscription on reconnect. Without optimistic gate, reports blocked indefinitely. Proper SM event handling restores connectivity.

**Commit:** feature/ble-hid-v2 (clean build ✅)

### 2025-01-31: Gate BLE Notification Enable on HCI_EVENT_ENCRYPTION_CHANGE (Disconnect Loop Fix)

**By:** Edward (BLE/BTstack firmware engineer)  
**What:** Root cause identified and fixed for BLE disconnect/reconnect loop on bonded power-cycle:
- **Root cause:** `SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED` fires before LTK encryption handshake completes. Setting `_notificationsEnabled = true` there allowed ATT notifications on unencrypted link → ATT security error → disconnect → Windows retry loop.
- **Fix:** Moved notification gate from `SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED` to `HCI_EVENT_ENCRYPTION_CHANGE` (status==ERROR_CODE_SUCCESS && connected). This event fires only after LTK handshake succeeds.
- **SM Handler Update:** Now only sets `_hasBondedPeers = true` for informational use (identity confirmed).
- **Coverage:** Two correct paths now: fresh pair (HCI_EVENT_ENCRYPTION_CHANGE sets flag, then HIDS_SUBEVENT_INPUT_REPORT_ENABLE harmlessly re-sets it) and bonded reconnect (HCI_EVENT_ENCRYPTION_CHANGE arms notifications immediately, no CCCD write expected).

**Technical:** Correct event sequence for bonded BLE reconnect is: (1) HCI_SUBEVENT_LE_CONNECTION_COMPLETE (link exists, unencrypted), (2) SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED (identity confirmed, still unencrypted), (3) HCI_EVENT_ENCRYPTION_CHANGE status=SUCCESS (link now encrypted ← safe for ATT), (4) SM_EVENT_PAIRING_COMPLETE (fresh pair only), (5) HIDS_SUBEVENT_INPUT_REPORT_ENABLE (fresh pair only).

**Learning:** Never gate ATT/GATT operations on SM identity events. Use encryption-complete as the authoritative gate.

**Why:** Fix resolves the disconnect loop that occurred when bonded hosts reconnected after power cycle. SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED is purely identity confirmation, not an encryption event.

**Commit:** feature/ble-hid-v2 (clean build ✅)

### 2026-03-29T201441: Dead Code Pattern Documented — BTStack Key Management

**By:** Edward (Firmware Dev)  
**What:** `src/bt_config_bridge.cpp` contains `i2c_key_save()`, `i2c_key_load()`, and `i2c_key_clear()` functions flagged as **unused**. BTstack handles Bluetooth bonding internally via TLV (tag-length-value) flash storage (`.pico-sdk/btstack_priv.tlv`). These functions were intended for manual bonding management but are superseded by BTstack's built-in persistence.

**Status:** Documented for future code cleanup phase. Not an immediate fix — intended as a note for dead code removal pass.

**Why:** Codebase survey for architectural clarity. Helps future contributors understand BTstack integration points.

### 2026-03-29: BLE HID Feature Documentation Complete & Approved

**By:** Maes Hughes (authored), Edward (revised), Riza (reviewed)  
**What:** Comprehensive BLE HID (Bluetooth Low Energy HID) feature documentation for Phase 1–3 implementation on feature/ble-hid-v2.

**Deliverable:** `docs/development/ble-hid-support.md` (679 lines, commits aa32e09c, ff1fa570, fe041066)

**Content:**
- Overview, scope, and board requirements (CYW43-only)
- Architecture and OutputManager integration
- 6 critical BTstack requirements from failed first attempt
- Complete GATT service structure and HID report descriptor
- Pairing, bonding, and SM configuration with Windows SC requirement
- USB config fallback mechanism (S2 button hold)
- Battery Service integration (GATT 0x180F)
- 10 known pitfalls with corrections and code examples
- Phased implementation plan (Phase 1: 10–15 days, Phase 2: 5–7 days, Phase 3: 8–10 days)
- BLE vs. BT Classic comparison table

**Review Cycle:**
1. **Round 1 (Riza):** Rejected with 6 blocking API errors (SDK version, TLV signatures, GATT struct, battery API)
2. **Round 1 Revision (Edward):** Fixed all 6 blockers + added Critical API Checklist
3. **Round 2 (Riza):** Approved with 1 non-blocking note (duplicate setupSM call)
4. **Polish (Edward):** Removed duplicate setupSM call, document finalized

**Key Technical Decisions:**
1. Windows 10/11 requires Secure Connections (`ENABLE_LE_SECURE_CONNECTIONS`)
2. Advertising starts after `HCI_STATE_WORKING` event (not immediately on boot)
3. TLV flash context initialized before `sm_init()`
4. HID Report characteristics exclude Boot Keyboard/Mouse
5. USB fallback via S2 button preserves web configurator access
6. iOS/iPad support deferred to Phase 3 (pairing complexity)
7. Nintendo Switch incompatible with BLE HID (Switch-only BT Classic support via bluetooth-support.md)

**Branch Strategy:** New clean feature/ble-hid-v2 from develop (previous feature/ble-hid abandoned after connection failures). Documentation written and approved BEFORE implementation.

**Status:** ✅ Complete. Edward cleared to begin Phase 1 implementation with high-confidence API guidance.

**Why:** Feature planning document required comprehensive technical accuracy review to prevent implementation errors. Documentation-first methodology ensures design clarity before coding begins.

### 2026-03-29: BLE Pairing & Report ID Alignment — GATT Encryption + Descriptor Fix

**By:** Edward (Firmware Dev)  
**What:** Resolved BLE pairing failure root causes in feature/ble-hid during Phase 1 attempt.

**Findings & Fixes:**

1. **ENCRYPTION_KEY_SIZE_16 on HID Report characteristics (Issue 1)**
   - BTstack's standard `hids.gatt` marks Report characteristics with `ENCRYPTION_KEY_SIZE_16`
   - Modern BLE hosts (Windows, Android) perform GATT discovery before pairing
   - Hosts encounter encrypted characteristics, cannot read before bond, abandon discovery
   - Result: "try connecting again" without pairing completion
   - **Fix:** Replaced `#import <hids.gatt>` with inline HID service; removed `ENCRYPTION_KEY_SIZE_16` from Report characteristics
   - All other HID structure (UUIDs, Report References, HID Information) preserved

2. **Report ID mismatch (Issue 2)**
   - GATT Report Reference declared Input Report ID 1, type Input
   - HID Report Descriptor had no Report ID tag (`0x85, 0x01`)
   - Windows maps GATT characteristic to HID report by Report ID
   - Mismatch caused HID driver rejection
   - **Fix:** Added `0x85, 0x01` (Report ID 1) to `hid_descriptor_gamepad[]` in first COLLECTION

3. **gap_set_bondable_mode(1) investigation (Issue 3)**
   - Suggestion: add bondable mode enable call
   - Investigation: `hci_init()` sets `bondable = 1` by default unconditionally
   - `gap_set_bondable_mode()` is `#ifdef ENABLE_CLASSIC` only (BLE-only builds have no symbol)
   - Build confirmed: linker error on call
   - **Decision:** Do not call. `SM_AUTHREQ_BONDING` alone is correct (matches BTstack hog_keyboard_demo)

**Files Changed:**
- `src/ble_hid.gatt` — Inline HID service without encryption on Report characteristics
- `src/BLEHIDManager.cpp` — Report ID tag in HID descriptor

**Commit:** 32e0447e on feature/ble-hid

**Status:** Addressed; awaiting hardware verification (bonding key storage and reconnection untested until device pairing)

**Why:** Phase 1 attempt revealed critical BTstack integration issues during pairing. Documenting root causes prevents repeat failures in future attempts.

### 2026-03-29: BLE Connection Failures — Advertising Timing & AD Structure Fixes

**By:** Edward (Firmware Dev)  
**What:** Fixed three critical BLE connection issues in feature/ble-hid firmware.

**Findings & Fixes:**

1. **Advertising before HCI_STATE_WORKING (Issues 1 & 2)**
   - Called `_startAdvertising()` (gap_advertisements_enable) immediately on boot
   - Device appeared in BLE scans but every connection attempt failed
   - Root: BTstack buffers advertising command; device visible but not ready to accept connections
   - **Fix:** Removed early `_startAdvertising()` call. Added `BTSTACK_EVENT_STATE` handler:
   ```cpp
   case BTSTACK_EVENT_STATE:
       if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
           mgr._startAdvertising();
       }
   ```
   - **Why:** Canonical BTstack pattern (all official HID demos); advertising must start after HCI ready

2. **Null terminator in scan response AD data (Issue 3)**
   - AD type 0x09 (Complete Local Name) included null byte in name length
   - Device name announced as 11 bytes with trailing null instead of 10 name bytes
   - Some BLE hosts reject malformed AD structures or display incorrectly
   - **Fix:** Removed null byte, corrected length from 0x0B to 0x0A

3. **SM_EVENT_PAIRING_COMPLETE handler (Issue 5)**
   - Added explicit pairing complete handler:
   ```cpp
   case SM_EVENT_PAIRING_COMPLETE: {
       uint8_t status = sm_event_pairing_complete_get_status(packet);
       if (status != ERROR_CODE_SUCCESS) {
           blink_cyw43_led(5, 50, 50);   // 5x fast blink = pairing failed
           mgr._startAdvertising();
       }
   }
   ```
   - Provides faster recovery on pairing failure + visual LED debug signal

**Files Changed:**
- `src/BLEHIDManager.cpp` — Advertising timing fix, AD structure correction, pairing handler

**Commit:** bb8eb42d on feature/ble-hid

**Build & Flash:** ✅ Clean build, picotool flash successful  
**Board:** Pimoroni Pico Lipo 2 XL W (RP2350B + CYW43439)

**Status:** Fixed; Phase 1 connection issue resolved.

**Why:** Advertising before HCI ready is the primary cause of "device visible but connection fails" symptom. BTstack requires `HCI_STATE_WORKING` event guard per official examples.

### 2026-03-29: BLE Cold-Boot Hard Fault — TLV Context NULL Dereference

**By:** Edward (Firmware Dev)  
**What:** Diagnosed and fixed NULL pointer dereference causing cold-boot hard fault in BLE mode.

**Root Cause (Candidate 4):**

Device advertised correctly on warm resets (after USB power cycle) but hard-faulted on every cold boot. TLV context initialization bug:

```cpp
// BROKEN:
le_device_db_tlv_configure(tlv_impl, NULL);  // Second arg = NULL
sm_init();  // Internally calls le_device_db_tlv_scan() → get_tag(context, ...)
```

`sm_init()` calls `le_device_db_init()` → `le_device_db_tlv_scan()` → `get_tag(context, ...)`. The context is cast to `btstack_tlv_flash_bank_t*` and field-dereferenced. With NULL context, immediate hard fault. Board crashed silently; BLE never advertised.

**Fix:**
```cpp
// FIXED:
le_device_db_tlv_configure(tlv_impl, &tlv_context);  // Context object valid for program lifetime
```

**Additional Fix:**
- Replaced permanent `_initFailed` gate with 5-second retry loop for cyw43_arch_init() transient failures

**LED Debug Signals:**
- 2x fast blinks (100ms) — cyw43_arch_init() succeeded
- 3x fast blinks (100ms) — Full BTstack configured, HCI power on, advertising starting
- No blinks — cyw43_arch_init() failed (retries in 5s)

**Files Changed:**
- `src/BLEHIDManager.cpp` — TLV context fix, retry loop, LED debug helper
- `headers/BLEHIDManager.h` — Removed _initFailed/_pendingInit flags, added _initDelayMs

**Commit:** c440b8b6 on feature/ble-hid

**Status:** Fixed; board now cold-boots successfully into BLE advertising.

**Why:** Transient failures during cold boot were masking the root cause (hard fault). LED debug signals provide visual feedback for field diagnostics.

### 2026-03-29: S2 Boot Web Config Override Must Always Enable USB

**By:** Edward (Firmware Dev)  
**What:** Added `!configMode` guard to the `wirelessOnly` block in `src/gp2040.cpp` so that USB is always initialized when web config mode is active, even when the saved flash config is `INPUT_MODE_BLE`.

```cpp
bool wirelessOnly = false;
#ifdef ENABLE_BLUETOOTH
    if (!configMode && Storage::getInstance().getGamepadOptions().inputMode == INPUT_MODE_BLE) {
        wirelessOnly = true;
    }
#endif
```

`configMode` is derived from `DriverManager::getInstance().isConfigMode()` at line 295, which reflects the runtime boot action (S2 held → `INPUT_MODE_CONFIG`), not the saved flash value.

**Why:** S2 web config mode is a recovery/escape mechanism. It must be unconditionally reachable from USB regardless of saved input mode. Fix is minimal and surgical — one boolean condition, no other BLE behavior affected.  
**Build:** ✅ Pass — `build_ble3/GP2040-CE_0.7.12_PimoroniPicoLipo2XLW.uf2` (3,094,528 bytes)

### 2026-03-30: BLE HID Phase 1 Implementation Complete

**By:** Edward (Firmware Dev)  
**Commit:** c494add2 on feature/ble-hid-v2  
**What:** Full Phase 1 BLE HID gamepad implementation on a clean branch. Files created: `src/ble_hid.gatt`, `headers/BLEHIDManager.h`, `src/BLEHIDManager.cpp`, `headers/btstack_config.h`, `headers/OutputManager.h`, `src/OutputManager.cpp`. Files modified: `CMakeLists.txt`, `src/gp2040.cpp`, `proto/enums.proto`, `headers/display/ui/screens/MainMenuScreen.h`, web UI locale/mode selectors.

**Critical SDK deviations documented:**
- `pico_btstack_hid_device` CMake target does not exist in SDK 2.2.0; `hids_device.c` is inside `pico_btstack_ble`
- `pico_cyw43_arch_poll` requires lwIP; use `pico_cyw43_arch_none` for BT-only
- `ADV_IND` constant is not in BTstack public API; use `0` directly
- `HIDS_SUBEVENT_INPUT_REPORT_DISABLE` does not exist; use `HIDS_SUBEVENT_INPUT_REPORT_ENABLE` + enable getter
- Appearance value `964` in `.gatt` generates narrowing error; must use `C4 03` (hex bytes, little-endian)
- `att_server_init` callbacks must be NULL; `hids_device` registers its own ATT handler

**Build:** ✅ Success — `build_ble3/GP2040-CE_0.7.12_PimoroniPicoLipo2XLW.uf2`  
**Why:** Phase 1 lays the full BTstack BLE HID foundation for the GP2040-CE wireless input path.

### 2026-03-30: BLE HID Report Descriptor Must Match USB HID Descriptor

**By:** Edward (Firmware Dev)  
**Commit:** 176109ef on feature/ble-hid-v2  
**What:** The BLE HID report descriptor body must be byte-for-byte identical to the USB HID descriptor in `HIDDescriptors.h`, with only a Report ID 1 item (`0x85, 0x01`) prepended.

Two bugs in the Phase 1 descriptor caused total input failure on Windows:
- Axis usages Rx/Ry (0x33/0x34) instead of Z/Rz (0x32/0x35) used by the USB driver
- Axis logical range signed -128..127 instead of unsigned 0..255 used by the USB driver and `HIDReport` struct

`OutputManager::dispatch()` also encoded axes as signed `int8_t`; corrected to unsigned `uint8_t` matching `HIDDriver::process()`.

**Rules going forward:**
- BLE HID descriptor = USB `hid_report_descriptor` body + Report ID 1 prefix
- The 9-byte ATT notification payload is always: 4 bytes buttons + 1 byte hat/pad + 4 bytes axes (no Report ID byte in payload)
- Any change to the USB descriptor MUST be reflected in the BLE descriptor and OutputManager simultaneously

**Why:** When Windows parses the GATT HID Report Map characteristic, any mismatch with the actual ATT notification data cascades to full input failure. BLE and USB must present identical layouts.

### 2026-03-30: Remove Blocking sleep_ms from Hot Loop

**By:** Edward (Firmware Dev)  
**Status:** COMPLETE — build verified ✅

**Problem:** `sleep_ms()` calls in `OutputManager::dispatch()` were blocking the entire firmware main loop, causing XInput enumeration timeout and BLE advertising starvation.

**Decision:**
- **Rule: OutputManager::dispatch() must have ZERO blocking calls** — on the hot path of USB+BLE loop
- **Rule: BLEHIDManager::process() must not call sleep_ms() after cyw43_arch_poll()** — starves BTstack run loop
- **Exception: _doInit() blocking is acceptable** — called once during startup, before BTstack runs

**Changes:**
- Removed `dispatched_once` block: 1s LED + sleep_ms(1000) + sleep_ms(500)
- Removed button-detection blink: 3 × sleep_ms(300) per button press
- Replaced blocking LED blinks in `process()` with non-blocking `absolute_time_t` state machine
- Result: ZERO `sleep_ms()` calls in hot path

**Build:** ✅ Success — 3023 KB, zero warnings

**Why:** Blocking calls in the main loop freeze both USB polling and BTstack event handling, causing enumeration timeouts and wireless starvation.

### 2026-03-31: GPIO Input Must Be Read in All Input Modes (Including BLE)

**By:** Edward (Firmware Dev)  
**Status:** Implemented

**Problem:** In BLE mode, `inputDriver->process(gamepad)` was guarded by `!wirelessOnly`, so GPIO pins were never read. `gamepad->state` remained all zeros when dispatching to BLE HID.

**Decision:** Remove the `!wirelessOnly` guard. GPIO input reading is transport-agnostic and must happen before ANY output dispatch (USB HID, BLE HID, or future wireless).

**Implementation:**
```cpp
// OLD: if (!wirelessOnly && inputDriver != nullptr)
// NEW: if (inputDriver != nullptr)
bool processed = inputDriver->process(gamepad);
```

**Why:** GPIO reading is orthogonal to output transport. The physical button states must be sampled from hardware regardless of whether output goes to USB or BLE.

### 2026-03-31: BLE HID Report Pipeline Deep Audit — Debug Instrumentation Added

**By:** Edward (Firmware Dev)  
**Date:** 2026-03-31  
**Status:** Debug build ready for hardware testing

**Decision:** Added comprehensive LED blink diagnostics to the BLE HID pipeline (5 audit points) to identify failure points without USB serial debug output.

**Audit Results:** All structural points verified correct (notifications handling, run loop integration, report ID encoding, GPIO reading). Runtime behavior (CCCD subscription, button data non-zero) required hardware testing.

**Changes:** Added LED blink patterns in `_hciPacketHandler()` for HIDS events and OutputManager for button detection.

**Build:** ✅ Clean, 3,095,040 bytes

**Why:** Pipeline diagnosis required observable LED patterns when zero button presses register but connection succeeds.

### 2026-03-31: BLE Firmware Rebuild Result — e2584c20

**By:** Edward (Firmware Dev)  
**Date:** 2026-03-30

**Finding:** `build_ble3` firmware is current and healthy after commit e2584c20 (volatile fix + sm_init ordering).

**Build:** ✅ No work to do — timestamps confirm build artifacts are newer than last source change

**Output:** `GP2040-CE_0.7.12_PimoroniPicoLipo2XLW.uf2` (3,094,528 bytes)

**What e2584c20 Fixed:**
1. `volatile` on shared-state fields (`_connected`, `_notificationsEnabled`, `_reportPending`, `_conHandle`, `_pendingReportLen`)
2. `sm_init()` ordering — Security Manager must precede ATT service init

**Why:** Without these fixes, compiler cached `_connected == false` in registers, and BTstack initialization failed silently.

### 2026-03-31: Nuclear Debug Diagnostics — LED Pattern System & CCCD Subscription Gate Bypass

**By:** Edward  
**Date:** 2026-03-29  
**Status:** Experimental diagnostic — requires hardware validation

**Decision:** Implement visible LED diagnostic patterns for multi-stage BLE HID pipeline debugging, and bypass the `_notificationsEnabled` gate to isolate CCCD subscription failures.

**Pattern Vocabulary:**
- **5 blinks × 200ms:** HIDS_SUBEVENT_INPUT_REPORT_ENABLE (Windows subscribed)
- **3 blinks × 300ms:** Buttons detected in OutputManager
- **50ms on:** Report sent to BTstack
- **2 blinks × 500ms every 2s:** Notification gate blocked

**Nuclear Bypass:** Removed `_notificationsEnabled` check from `sendReport()` and `process()`. If buttons work with bypass → CCCD event handler never fired. If still blocked → problem upstream.

**Changes:**
1. Moved LED blink execution from BTstack event handler (IRQ context) to `process()` (main loop) via `volatile uint8_t _pendingBlinkType` flag
2. Added 1s dispatch indicator in OutputManager
3. Replaced µs pulses with visible ms-range blinks

**Build:** ✅ 3022.50 KB, zero warnings

**Architectural Patterns Established:**
- Never block BTstack IRQ context — use `volatile` flags to defer execution to main loop
- LED diagnostic vocabulary: 50-100ms for events, 200-300ms for state transitions, 500ms+ for error states
- Nuclear debug pattern: bypass gates one by one to isolate which stage is failing

**Why:** Without hardware serial output, LED patterns are the only debugging tool for wireless firmware. Bypassing gates allows rapid isolation of failure points.

### 2026-03-31: BLE HID Milestone Confirmed — hids_device_register_packet_handler() Was Missing Piece

**By:** User (thegu)  
**Date:** 2026-03-31  
**Commit:** 0291e55a on feature/ble-hid-v2  
**Status:** MILESTONE — BLE HID input working end-to-end ✅

**Confirmed Working:**
- Button presses register in Windows joy.cpl
- Device connects via BLE successfully
- 32 buttons visible in controller configuration
- No USB enumeration failures

**Two Root Causes Found and Fixed:**

1. **BLEHIDManager::init() was never called** → no advertising
   - Device never started BLE advertising sequence
   - Fixed by adding `BLEHIDManager::init()` to startup sequence

2. **hids_device_register_packet_handler() never called** → HIDS events dropped
   - BTstack requires TWO separate event registrations: `hci_add_event_handler()` (general) + `hids_device_register_packet_handler()` (HIDS-specific)
   - HIDS meta events (INPUT_REPORT_ENABLE, CAN_SEND_NOW) only delivered to HIDS handler, not HCI handler
   - Windows writes CCCD → triggers HIDS_SUBEVENT_INPUT_REPORT_ENABLE → but handler was never registered
   - Fixed by adding `hids_device_register_packet_handler(_hciPacketHandler)` in `_doInit()`

**How Identified:** Comparison with BTstack's official `hog_keyboard_demo.c` reference implementation showed both registration calls were required.

**Build Artifact:** `GP2040-CE_0.7.12_PimoroniPicoLipo2XLW.uf2` (3,094,528 bytes)

**Why:** This milestone confirms BLE HID input pipeline is architecturally sound and ready for multi-transport firmware deployment.

## Governance

- All meaningful changes require team consensus
- Document architectural decisions here
- Keep history focused on work, decisions focused on direction
