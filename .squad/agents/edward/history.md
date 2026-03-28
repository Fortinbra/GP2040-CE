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

### 2026-03-28: SDK Version Conflict Resolution (Riza Review)

**SDK 2.2.0 is the project-wide baseline** — confirmed by three sources: `CMakeLists.txt` `set(sdkVersion 2.2.0)`, FATAL_ERROR check at configure time (`VERSION_LESS "2.2.0"`), and CI checking out pico-sdk at tag `2.2.0`. This applies to RP2040 and RP2350 builds equally. There is no "RP2040 uses 2.1.1, RP2350 uses 2.2.0" split — the whole project requires 2.2.0.

**`copilot-instructions.md` was updated** from 2.1.1 → 2.2.0 (and picotool from 2.1.1 → 2.2.0) because it held stale values that contradicted the build system.

**Pico2W blocker is CYW43 wireless stack porting.** The existing PicoW config (`PICO_BOARD=pico_w`, `PICO_PLATFORM=rp2040`) uses the CYW43439 driver in its RP2040 form. The Pico 2 W needs `PICO_BOARD=pico2_w`/`PICO_PLATFORM=rp2350-arm-s` plus adaptation of the CYW43 wireless integration layer for RP2350. The base firmware runs fine on RP2350A — the gap is specifically the wireless feature stack, not the core gamepad firmware.

**SDK version verification method:** `cat $PICO_SDK_PATH/pico_sdk_version.cmake` or CMake configure output (prints version and halts on FATAL_ERROR if < 2.2.0). `cmake --version` only checks CMake itself, not the SDK.

### 2026-03-28T024429: Review Gauntlet — Round 1 Revision (cross-agent update from Scribe)

Edward's round-1 revision (`688582e4`) passed technical content checks in all subsequent rounds. The SDK version ground truth established here (2.2.0 project-wide) held through all 6 review rounds and became a recorded team decision. The Pico2W / CYW43 wireless porting gap explanation and SDK verification method correction were accepted as-is and survived to final approval.
