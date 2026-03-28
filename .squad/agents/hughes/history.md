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

