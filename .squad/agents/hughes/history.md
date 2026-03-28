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
