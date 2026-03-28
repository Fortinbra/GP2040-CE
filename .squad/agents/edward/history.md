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

### 2026-03-28: RP2350 Analysis

**SDK version:** Project requires **Pico SDK 2.2.0** (CMakeLists.txt fatal error if < 2.2.0). CI checks out SDK at tag `2.2.0`. RP2350 support was introduced in SDK 2.0.0 — project is already compatible.

**Existing RP2350 support:** Three board configs already exist and are CI-tested: `Pico2` (pico2/rp2350-arm-s), `FlatboxRev8` (pico2/rp2350-arm-s), `SparkFunProMicroRP2350` (sparkfun_promicro_rp2350/rp2350-arm-s). `Pico2W` is the only obvious missing config.

**Board config structure:** Each board lives in `configs/<Name>/` with `BoardConfig.h` (GPIO→action mappings), `<Name>.cmake` (sets PICO_BOARD + PICO_PLATFORM), optional `CMakeLists.txt` stub, `README.md`, and `assets/`. The `.cmake` file is what controls chip targeting.

**GPIO handling is already platform-adaptive:** `helper.h::isValidPin()` uses `NUM_BANK0_GPIOS` from the SDK — returns 30 for RP2040/RP2350A, 48 for RP2350B. Array sizes in `gp2040.h` and `storagemanager.h` also use this constant. No hardcoded GPIO count anywhere in main firmware.

**No chip-specific `#ifdef` guards:** Zero `rp2040`, `rp2350`, or `PICO_PLATFORM` references in `src/` or `headers/`. All hardware access goes through SDK abstractions.

**PIO USB host:** `CFG_TUH_RPI_PIO_USB 1` — uses PIO-based USB host. PIO0/PIO1 are backwards compatible on RP2350. CI passes for RP2350 builds. Hardware testing of pass-through at 150 MHz clock recommended.

**All existing RP2350 configs use `rp2350-arm-s`** (ARM Secure mode). RISC-V mode not used and not appropriate for this project.
