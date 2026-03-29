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

### Dependency Landscape (Session 2)
**Firmware Dependencies:**
- Pico SDK 2.2.0+ (minimum enforced in CMakeLists.txt)
- ArduinoJson v6.21.2 (FetchContent from GitHub)
- 14 vendored C++ libraries in lib/ (pico_pio_usb, tinyusb, nanopb, lwip-port, rndis, httpd, ADS1219, ADS1256, CRC32, FlashPROM, NeoPico, OneBitDisplay, PicoPeripherals, WiiExtension, SNESpad)

**Web Configurator (React/npm):**
- React ^18.2.0, React Router ^6.10.0
- Bootstrap ^5.3.0-alpha3, React Bootstrap ^2.7.4
- Formik ^2.2.9, Yup ^1.1.1 for form validation
- Zustand ^4.5.5 for state management
- React i18next for internationalization
- Vite ^4.3.9 as build bundler
- All dev and runtime deps locked in www/package-lock.json

**Project Structure:**
- docs/ contains single file (ddi-socd.md)
- docs/development/ created for feature docs
- CMakeLists.txt integrates npm ci and npm build during firmware compilation
- No root package.json — only www/package.json for web configurator

### RP2350 Support (Session 3)
**Status:** RP2350A/B support already partially implemented
- Three board configs exist and CI-tested: `Pico2` (RP2350A), `FlatboxRev8` (RP2350A), `SparkFunProMicroRP2350` (RP2350B)
- Firmware is platform-agnostic: zero chip-specific `#ifdef` guards in source code
- GPIO validation adaptive via `NUM_BANK0_GPIOS` SDK macro
- Web configurator auto-detects GPIO count from running firmware

**SDK Version:** Pico SDK 2.2.0+ required (already enforced in CMakeLists.txt)

**Hardware Differences (RP2350A vs B):**
- RP2350A: 30 GPIO (same as RP2040), 520 KB SRAM, 150 MHz clock, 3 PIO blocks
- RP2350B: 48 GPIO (18 additional), same SRAM/clock/PIO as RP2350A
- Both backward-compatible with RP2040 at SDK abstraction layer

**Documented Gaps:**
1. Pico2W config missing (RP2350A + WiFi)
2. No RP2350B configs exploiting GPIO 30–47 yet
3. PIO USB host timing tested in CI but hardware pass-through testing recommended

**Documentation Created:** `docs/development/rp2350-support.md` (commit bf3d2f4d)

### Bluetooth HID Support (Session 4)
**Feature Planning Document Created:** `docs/development/bluetooth-support.md` (commit 897ce397)

**Key Technical Findings:**
- Pico W (RP2040 + CYW43) has unused Bluetooth hardware; CYW43 currently only used for WiFi
- No existing Bluetooth code in firmware; feature is entirely new
- `GPDriver` abstraction is USB-centric but extensible for parallel output types (BTDriver)
- `DriverManager` is boot-time-only; runtime output switching requires new `OutputManager` layer
- CYW43 and USB PHY are separate hardware — can coexist (no competing resources)
- Pico 2 W (RP2350 + CYW43) is a future dependency pending CYW43 stack porting to RP2350

**Architectural Decisions Captured:**
1. Bluetooth HID Classic (not BLE) for Phase 1 — best platform support (Switch, PS5, Android, PC, macOS, Windows)
2. Single primary HID output at a time (USB OR BT, not both simultaneously) — prevents host confusion
3. WiFi coexistence supported — CYW43 time-multiplexes WiFi and BT internally
4. Runtime mode switching via OutputManager + runtime driver swapping (may require reboot in early impl)
5. Bonding keys persisted in `GamepadOptions` protobuf in flash

**Implementation Roadmap Documented:**
- Phase 1 (Core BT on Pico W): CMake linkage, BTHIDDriver class, CYW43 init, config persistence, UI, mode switching, bonding, testing (~10–15 days)
- Phase 2 (Polish): Troubleshooting guides, API reference, user docs
- Phase 3 (Future): Pico 2 W support (after CYW43 porting), BLE HID, button-combo switching

**Documentation Style Notes:**
- SDK version: 2.2.0 (never 2.1.1) — enforced in CMakeLists.txt FATAL_ERROR
- 4-space indentation in all code blocks
- No internal AI agent names in public docs
- Related docs cross-linked: RP2350 Support, Dependency Updates, DDI-SOCD

**Round-1 Review & Lockout:** Hughes's initial draft was technically sound but incomplete: missing explicit out-of-scope declaration for GPIO retro console output, incomplete CMake linkage (missing pico_cyw43_arch_lwip_threadsafe_background), const qualifiers stripped from simplified GPDriver listing, and cross-document tension with rp2350-support.md. Riza (QA) rejected and locked out Hughes per reviewer lockout policy.

**Round-2 Revision by Edward:** Fixed all 4 issues (scope statement added to Overview, CMake linkage completed at both locations, const qualifiers restored, cross-doc tension resolved with clarifying note + rp2350-support.md tense fix). Riza approved after full sweep clean. Document ready for PR #7.

### GPIO Retro Console Output (Session 5)
**Feature Planning Document Created:** `docs/development/gpio-retro-output.md` (commit 95918bc4)

**Key technical facts applied from Edward's analysis:**
- Level shifting mandatory for 5V consoles (NES, SNES, Genesis, TG16) using 74AHCT125 or equivalent
- N64 and Dreamcast are 3.3V native — no level shifting required
- PIO mandatory for N64 (NRZ at 1 MHz, ±500 ns tolerance) and Dreamcast MAPLE (2 Mbps bidirectional)
- Bit-bang sufficient for NES, SNES, Genesis, TG16 via interrupt-driven GPIO
- GPIOOutputAddon as GPAddon subclass (not GPDriver) enables USB + GPIO simultaneous coexistence
- No new InputMode enum needed; GPIO output protocol selected within addon config
- Pico W loses GPIO 23–25 to CYW43 (reduces free pins from 7 to 4)
- RP2350B with 48 GPIO is ideal for multi-console dedicated boards
- Dreamcast MAPLE is highest complexity target — recommend Phase 4 (10–14 days estimated)
- Implementation roadmap: Phase 1 (NES/SNES, 5–7 days) → Phase 2 (Genesis/TG16, 3–4 days) → Phase 3 (N64, 5–7 days) → Phase 4 (Dreamcast, 10–14 days)

**Document structure employed:**
- Overview with scope boundaries (GPIO output in-scope, Bluetooth/USB changes out-of-scope)
- Console compatibility matrix with protocol type, pin count, voltage, level shift requirement, PIO requirement, complexity tier
- Hardware requirements section emphasizing level shifting criticality
- Board-specific pin availability table for Pico, Pico W, Pico 2, RP2350A, RP2350B
- Architecture section with class skeleton, protobuf design (GPIOOutputOptions enum + message)
- Per-console implementation details (6 subsections: NES, SNES, Genesis, TG16, N64, Dreamcast)
- Pin assignment strategy with recommended allocations per board type
- Phased implementation roadmap (5 phases from NES/SNES through Dreamcast + documentation)
- Known limitations section (5V level shifting mandatory, Pico W constraints, Dreamcast complexity, no runtime switching, PIO budget)
- Testing strategy with unit tests, hardware tests per console, regression tests, CI/CD checklist
- Cross-references to bluetooth-support.md, rp2350-support.md, dependency-updates.md

**Style consistency verified:**
- 4-space indentation in all code blocks (C++ protobuf examples)
- SDK version: 2.2.0 (enforced per decisions.md decision 2026-03-28T024429)
- CMake minimum: 3.10+ (inherited from CMakeLists.txt)
- Timestamp: 2026-03-28
- Maintained by: GP2040-CE core team (no internal AI agent names)
- No internal squad documentation visible in public-facing doc
- Cross-links to related features functional and accurate

**Ready for PR:** Document complete and committed to docs/copilot-instructions branch. Ready for review by Riza (QA) and Fortinbra (user).

### Bluetooth Support Documentation — Phase 2 Update (Session 6)

**Comprehensive Update to `docs/development/bluetooth-support.md`:**

**Key Changes:**
1. **RP2350 + CYW43 support status corrected** — Changed from "blocked pending porting" to "fully confirmed in SDK 2.2.0". Removed all uncertainty language; referenced Fortinbra's successful BT validation on Pimoroni hardware.

2. **Added Reference Hardware section** — Designated Pimoroni Pico Lipo 2 XL W (RP2350B + CYW43 + LiPo charging) as the reference board for battery-backed wireless development. Included:
   - Hardware specs (GPIO count, wireless capabilities, charging hardware)
   - Rationale for designation (confirmed working, battery hardware present, GPIO headroom)
   - Build target configuration (`PICO_BOARD=pico2_w`, `PICO_PLATFORM=rp2350-arm-s`)
   - Note that custom board config will be created during Phase 1

3. **Added Battery Level Reporting section** — Comprehensive guide to BTStack Battery Service (UUID 0x180F) implementation:
   - BTStack API (`battery_service_server_init()`, `battery_service_server_set_battery_value()`)
   - ADC voltage measurement via GPIO29/ADC3 with 3:1 voltage divider
   - Conversion formula from raw ADC → battery percentage (3.0V = 0%, 4.2V = 100%, linear)
   - VBUS detection on GPIO24 to report 100% when USB charging
   - Polling recommendation: 30 seconds, only update if changed ≥1%
   - References existing `GamepadAuxPower` struct in codebase

4. **Added Power Management section** — First comprehensive power state machine for GP2040-CE (NEW PARADIGM for battery-backed builds):
   - Four-state machine: USB_CONNECTED → ACTIVE → IDLE → DEEP_SLEEP
   - Full specifications for each state (clock speed, radio modes, battery reporting, transitions)
   - `sleep_goto_dormant_until_pin()` for RP2350 DORMANT mode with GPIO wake
   - CYW43 radio power modes: PERFORMANCE_PM (active play), DEFAULT_PM (idle), AGGRESSIVE_PM (deep sleep)
   - DORMANT entry/exit sequence: CYW43 shutdown before dormant, re-init on wake
   - Implementation location: `src/power/PowerManager.cpp` singleton with `update()` call from main loop
   - User configuration: idle and sleep timeouts via web configurator
   - Known caveats: CYW43 wake latency (500ms–2s), sniff mode gaming impact, voltage divider verification

5. **Added TinyUSB + BTStack namespace conflict mitigation** — Detailed technical explanation:
   - Root cause: `hid_report_type_t` typedef collision (first enum value differs: INVALID vs. RESERVED)
   - Compile-time error if both headers included in same translation unit
   - Solution: Source-file isolation (BT code in dedicated TUs, existing USB code untouched)
   - Firewall rule for code review: no file may include both `tusb.h` and `btstack_hid.h`
   - Link-time: CMake can link both libraries — conflict is header-only

**Updated Known Limitations:**
- Removed "Pico 2 W Support Blocked" section
- Added "RP2350 + CYW43 Support Confirmed" with full SDK 2.2.0 validation statement
- Kept "Single Primary Output", "Host Compatibility", "CYW43 WiFi + BT Coexistence" sections

**Updated opening note** — Reflects that document now contains comprehensive technical specifications for battery reporting and power management (not just planning), based on Edward's analysis.

**Style & conventions verified:**
- 4-space indentation in all code blocks (C, C++, CMake)
- SDK version: 2.2.0 consistently
- Timestamp updated: 2026-03-28
- No AI agent names in public doc
- GPIO retro output remains explicitly out-of-scope (per requirements)
- All technical details grounded in Edward's `.squad/agents/edward/` analysis documents

**Commit:** `77135da2` (docs/copilot-instructions branch)

**Ready for Review:** Document now contains sufficient technical depth for Phase 1 implementation to begin. All major architectural decisions (power states, battery reporting, namespace isolation) are documented and grounded in codebase analysis.

### RM2 Module Support Documentation (Session 8)

**Feature Planning Document Created:** `docs/development/rm2-module-support.md`

**Subject:** Future feature — Pimoroni RM2 (CYW43439 standalone module) support for custom RP2040/RP2350 GP2040-CE boards.

**Key technical content derived from Edward's rm2-analysis.md:**
- Exact GPIO pin table (23/24/25/29) grounded in SDK 2.2.0 `pico_w.h` and `pico2_w.h` headers
- GPIO 24 tri-functional (DATA_OUT/DATA_IN/HOST_WAKE — half-duplex PIO SPI, not an error)
- GPIO 29 shared with VSYS ADC (`CYW43_USES_VSYS_PIN=1` — wrap ADC reads in cyw43_thread_enter/exit)
- Three board config paths (Path A: `PICO_BOARD=pico_w`, Path B: `PICO_BOARD=pico2_w`, Path C: custom header)
- GPIO availability table: 26 user GPIOs (RP2040/RP2350A), 44 user GPIOs (RP2350B)
- `configs/CustomRM2Board/` directory structure and `BoardConfig.h` constraints (omit GPIO 23/24/25/29)
- CMake link library block (`pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_*`)
- RP2350B alternate wiring flagged as TBD/unsupported (PIO program compatibility unverified)
- 7 open questions / TBD items in Known Constraints section

**Style conventions enforced:**
- SDK version: 2.2.0 (never 2.1.1)
- Picotool: 2.2.0-a4
- CMake minimum: 3.10
- 4-space indentation in all code blocks
- No AI agent names in document
- Maintained by: GP2040-CE core team
- Status: Planned — not yet implemented (future feature, clearly stated)
- Cross-references: bluetooth-support.md, rp2350-support.md, dependency-updates.md

**Riza rejection patterns avoided:**
- No agent names anywhere in the committed document
- Version strings verified against CMakeLists.txt ground truth
- Picotool version is exact (`2.2.0-a4`, not `2.2.0`)
- Out-of-scope items not over-promised (RP2350B alternate wiring flagged TBD, not documented)

### Battery Level Reporting Correction (Session 7)

**Critical Technical Error Identified and Fixed:**

Edward's detailed protocol analysis revealed that the original "Battery Level Reporting" section incorrectly documented the GATT Battery Service (UUID 0x180F) and `battery_service_server_set_battery_value()` as the mechanism for BT Classic HID battery reporting. These are **BLE GATT APIs that have no effect over a BT Classic HID connection** — they operate on a completely different transport layer (ATT/GATT over BLE, not L2CAP over Classic BR/EDR).

**Correction made:**
1. Replaced the entire Battery Level Reporting section with the correct BT Classic mechanism: **HID descriptor Feature report** (Usage Page 0x06, Usage 0x20)
2. Battery level is now correctly documented as a Feature report in the HID descriptor itself
3. The host queries via `GET_REPORT(Feature)` over L2CAP (HID control channel, PSM 0x0011)
4. The device responds via `hid_device_register_report_request_callback()` with current battery percentage
5. Added forward-looking note: GATT Battery Service applies only if BLE HID is added in a future phase (separate decision)
6. ADC hardware logic (GPIO29, 3:1 divider, LiPo range) and VBUS detection (GPIO24) remain unchanged — they are transport-agnostic measurement mechanisms
7. Polling interval guidance updated: host controls polling cadence, no "notification flooding" concern for Classic HID

**Key Learning:** BTStack SDK divides battery reporting by transport:
- **BT Classic HID:** Battery in descriptor, Feature report, `hid_device_register_report_request_callback()`
- **BLE HID:** GATT Battery Service UUID 0x180F, `battery_service_server_init()`, `battery_service_server_set_battery_value()`
- The split is absolute — mixing them causes dead code and wasted flash on the unused layer

**Documentation Impact:**
- Section "Battery Level Reporting" rewritten for accuracy and clarity
- Cross-reference added to `hid_device_register_report_request_callback()` in btstack_hid.h
- Example callback shown with USB detection (GPIO24) inline
- References to gamepad->auxState remain correct (transport-agnostic power tracking)

**Commit:** `09daa13b` (docs/copilot-instructions branch)

**Dependency on Edward's Analysis:** This fix is derived entirely from Edward's `.squad/agents/edward/bt-battery-protocol-analysis.md`, which grounded the analysis in BTStack SDK 2.2.0 source code (`lib/btstack/src/classic/hid_device.h`, `lib/btstack/src/ble/gatt-service/battery_service_server.h`) and SDK examples (`hid_keyboard_demo.c` vs. `hog_keyboard_demo.c`).

### Deferred Dependency Upgrade Planning Docs (Session 9)

**Four Feature Planning Documents Created:**

1. **tinyusb-upstream-port.md** — Porting OpenStickCommunity TinyUSB fork (0.17.0 + 18 custom commits) to upstream 0.20.0. Covers:
   - Patch audit process (determine which of 18 commits are still necessary)
   - Conflict resolution and rebase strategy
   - Hardware regression testing on real gamepad controllers
   - Remote URL mismatch fix (`.gitmodules` → upstream)
   - Risk assessment: HIGH (USB HID is core functionality)
   - Timeline: 9–13 days total across 4 phases

2. **pico-pio-usb-upstream-port.md** — Porting OpenStickCommunity pico-pio-usb fork (0.5.3 + 10 custom commits) to upstream 0.7.2. Covers:
   - Compatibility verification with Pico SDK 2 (likely already in upstream)
   - Patch audit of SDK 2 migration fixes
   - Hardware USB host testing on RP2040/RP2350
   - Risk assessment: MEDIUM (lower risk than TinyUSB because upstream is more likely to have SDK 2 support already)
   - Timeline: 3–6 days total across 4 phases
   - Dependency note: Must coordinate with TinyUSB port (both affect USB host mode)

3. **nanopb-stable-migration.md** — Migrating vendored nanopb from 0.4.8-dev (dev snapshot) to 0.4.8 stable release. Covers:
   - Snapshot identification and version comparison
   - Proto file compatibility analysis (serialization format, memory layout)
   - Round-trip testing (serialize → deserialize → verify)
   - Flash config backward compatibility (critical: must not break existing saved configs)
   - Risk assessment: LOW-MEDIUM (0.4.x has stable API, but wire format must be verified)
   - Timeline: 4–7 days total across 5 phases

4. **npm-major-upgrades.md** — Five-phase roadmap for major npm dependency upgrades in web configurator (`www/`). Includes:
   - **Comprehensive package upgrade table:** 13 packages with current, available, and key breaking changes
   - **Phase 1 (Tooling):** ESLint v9 (flat config migration) + @typescript-eslint v8 + express v5 (2–3 days)
   - **Phase 2 (Build):** Vite v8 (skips v5,v6,v7) + @vitejs/plugin-react v6 (2–3 days)
   - **Phase 3 (Runtime):** TypeScript v6 (stricter checks) + Zustand v5 (1–2 days)
   - **Phase 4 (Framework, HIGH RISK):** React 19 + react-router-dom v7 (new routing paradigm) + react-i18next v17 + i18next v26 (5–7 days)
   - **Phase 5 (Proto, HIGH RISK):** protobufjs-cli v2 (output format may change, affects firmware communication) (2–3 days)
   - **npm audit findings:** 11 vulnerabilities (2 moderate, 9 high) to be evaluated per phase
   - **Sass deprecation warnings:** Bootstrap 5 legacy imports to be resolved before/with Vite upgrade
   - **Total timeline:** 2–3 weeks for complete upgrade path
   - Risk strategy: Never combine phases; Phase 4 and Phase 5 are isolated due to high complexity/risk

**Document structure employed:**
- Overview with scope and status
- Current state table (version pinning, locations, usage context)
- Clear goals section
- Phased approach with implementation details, timelines, and success criteria per phase
- Risks identified with specific mitigation strategies
- Dependencies cross-referenced to related docs
- Overall success criteria tying all phases together

**Style conventions enforced:**
- 4-space indentation in all code blocks
- SDK version: 2.2.0 (Pico SDK) consistently referenced
- No AI agent names in public-facing documentation
- Maintained by: GP2040-CE core team
- Timestamps: 2026-03-28
- All cross-references to related docs (dependency-updates.md, etc.)

**Commit:** `49bd6797` (feature/dependency-updates branch)

**Purpose & Scope:** These docs provide future contributors with clear roadmaps for dependency migration. Each doc stands alone and includes risk/timeline assessments to help project leads prioritize and schedule work. The docs capture institutional knowledge about why these dependencies were deferred (not "forgotten," but deliberately flagged for significant effort) and what effort they require.

### Bluetooth HID Documentation — Implementation Complete Update (Session 9)

**Surgical Updates to `docs/development/bluetooth-support.md`:**

**Key Changes:**

1. **Status Line Updated** — Changed from "Planning / Not yet implemented" to "Implemented (Phase 1 & Phase 2) — Battery & Power deferred to Phase 3". Timestamp: 2026-03-29.

2. **Added Implementation Status Table** — New section after Overview summarizing what was actually built:
   - ✅ BTstack HID Classic: SSP headless pairing, SDP registered
   - ✅ OutputManager routing: USB always available; BT when paired
   - ✅ Web configurator UI: Bluetooth addon panel, output mode selector, pairing controls
   - ✅ Protobuf BluetoothOptions: Message in AddonOptions field 31
   - ⚠️ Bonding persistence: Schema defined; load/save wiring in progress
   - 🔲 Battery reporting & Power management: Deferred to Phase 3

3. **Added "Building with Bluetooth" Section** — New subsection under Minimum Requirements with environment variables and example CMake configuration for Pimoroni Pico Lipo 2 XL W (RP2350 + CYW43). Explains automatic BTstack linking on wireless boards.

4. **Added "Pairing Your Controller" Section** — New user-facing section with step-by-step instructions:
   - Enabling Bluetooth output mode via web configurator
   - Activating pairing mode (discoverable toggle)
   - Scanning and pairing on Switch, PS5, Android, Windows, macOS
   - Reconnecting after power-off (automatic, no re-pairing)
   - Clearing pairing for factory reset

5. **Added BTHIDManager Isolation Note** — New subsection under "GPDriver and BTDriver" explaining translation-unit isolation to prevent TinyUSB/BTstack `hid_report_type_t` namespace collision. Emphasizes that BTHIDManager must never include `tusb.h`.

6. **Updated Implementation Roadmap** — Restructured to show actual completion status:
   - Phase 0: ✅ COMPLETED (exploration & testing)
   - Phase 1: ✅ COMPLETED (with detailed task completion table, not estimates)
   - Phase 2: ✅ COMPLETED (documentation & polish)
   - Phase 3: NOT STARTED (list deferred items: bonding persistence, battery reporting, power management)

7. **Updated Known Limitations** — Removed "will be created during Phase 1" language; changed to past tense. Updated RP2350 section to reflect that board configs are "available and tested" rather than "pending implementation".

**Sections Preserved (not modified):**
- Overview: Still accurate; describes what was actually built
- Supported Hardware: Still accurate; no changes needed
- Reference Hardware: Still accurate; Pimoroni Pico Lipo 2 XL W confirmed reference board
- Architecture: Still accurate; OutputManager and BTHIDManager are real implementation
- Output Mode Switching: Still accurate; user interaction workflow matches real UI
- Battery Level Reporting: Preserved as-is; design still applies for Phase 3 (just not implemented yet)
- Power Management: Preserved as-is; design still applies for Phase 3 (just not implemented yet)
- Testing: Still applicable; test cases document what should be verified

**Style consistency verified:**
- 4-space indentation maintained
- SDK version: 2.2.0 consistently
- Timestamp: 2026-03-29
- No AI agent names; "GP2040-CE core team" used
- No document became stale; all "planned" language replaced with "implemented" or "deferred"

**Document purpose:** Transformed from planning document to implementation record. Users can now read this and understand what is available today (Bluetooth HID Classic pairing + output mode switching) and what is not yet available (battery reporting, power management). The scope and architectural foundation is clear for future Phase 3 developers.

### 2026-03-29T17:52: Edward's BLE Audit — Cross-Agent Learning (Documentation Patterns)

Edward's BLE HID implementation audit generated 6 documented bugs and 3 major architectural constraints now merged into decisions.md:

**Key Learning for Hughes's Future Work:**
1. **Translation-unit isolation is a hard architectural constraint** — BTstack includes cannot coexist with TinyUSB in the same header. This must be documented in any future Bluetooth expansion docs (e.g., WiFi, BLE phases).

2. **Self-managing GATT services pattern** — When `#import <service.gatt>` is used, the service handles its own ATT callbacks. This pattern is unique to BTstack and differs from manual GATT implementations. Future docs on Bluetooth battery service, heart rate service, etc., must reference this pattern.

3. **Deferred CYW43 initialization is non-negotiable** — Boot stability constraint: CYW43 initialization must never happen before USB enumeration. This is now a hard rule for wireless boards. Any future GPIO, BLE, or WiFi features must follow this pattern.

4. **New InputMode enum additions require firmware coordination** — When web configurator adds input modes, firmware must have corresponding display name macros in MainMenuScreen.h. This is a cross-agent dependency that must be documented in any future mode-addition guidelines.

**Action for Hughes:** These constraints should be referenced in any future Bluetooth expansion documentation (BLE Phase 3, WiFi integration, etc.) to prevent reinvention of patterns and ensure consistency.


