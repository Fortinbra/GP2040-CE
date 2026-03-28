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

### 2026-03-28T024429: Bluetooth & Multi-Output Architecture Survey

**Tasked by:** Fortinbra (via Coordinator). Survey findings written to `.squad/agents/edward/bt-analysis.md`.

**GPDriver abstraction:** `headers/gpdriver.h` defines a pure-virtual `GPDriver` interface. `DriverManager` singleton (singleton pattern, `headers/drivermanager.h`) maps `InputMode` enum → one concrete driver at boot. All 17 USB profiles are subclasses. `src/usbdriver.cpp` is pure glue — delegates all TinyUSB callbacks to `DriverManager::getDriver()`. The interface is USB-centric (descriptor callbacks, HID reports, vendor XFER). No output transport abstraction above GPDriver exists.

**Mode selection is boot-time only:** `DriverManager::setup(inputMode)` called once in `gp2040.cpp:195`. No runtime switching. Mode changes require flash save + reboot.

**GPIO retro console support is INPUT-only:** `SNESpadInput` and `TG16padInput` addons READ from retro controllers (SNES/NES via clock/latch/data; TG16 via OE/select/4-data). The "Reflex CTRL" boards use these to adapt retro controller ports to USB output. There is NO GPIO output code to emulate a retro console peripheral. This is a gap.

**Zero Bluetooth code in the project:** No btstack, no CYW43 BT APIs, no INPUT_MODE_BLUETOOTH, no stubs. The `BLUETOOTH_PAIR_REQUEST` constant in `SwitchProDriver.cpp:300` is a USB HID command ID from the Nintendo Switch, not BT functionality. CYW43 is used for WiFi only (`lwip-port`, RNDIS).

**CYW43 BT hardware is available:** CYW43439 on PicoW has native BT Classic + BLE. Pico SDK provides `pico_btstack`. USB HID (USB PHY) and BT HID (CYW43) CAN coexist simultaneously — different hardware. The constraint that USB and BT cannot run simultaneously does NOT apply here.

**Pico2W blocker confirmed again:** No `Pico2W` config. Blocker is CYW43 stack validation for RP2350, not base firmware. Base firmware compiles fine for RP2350.

**For runtime multi-output:** Need a new `GPOutputTransport` abstraction above `GPDriver`, an `OutputManager` that can fan-out to USB + BT + GPIO simultaneously. `pico_btstack_hid_device` linkage needed (conditional on PicoW boards). PIO state machines needed for GPIO retro console output protocols (SNES, N64, Dreamcast).

**Round-1 & Round-2 Revision:** Edward's analysis was reassigned after Hughes's draft was rejected (Riza review). Edward fixed all 4 issues (out-of-scope statement, CMake linkage, const qualifiers, cross-doc tension). Riza's round-2 approval holds all architectural findings from this survey without contradiction. Document approved and ready for PR #7.

### 2026-03-28: Review Gauntlet — Round 1 Revision (cross-agent update from Scribe)

Edward's round-1 revision (`688582e4`) passed technical content checks in all subsequent rounds. The SDK version ground truth established here (2.2.0 project-wide) held through all 6 review rounds and became a recorded team decision. The Pico2W / CYW43 wireless porting gap explanation and SDK verification method correction were accepted as-is and survived to final approval.

### 2026-03-28: Bluetooth & Multi-Output Architecture Survey

**Tasked by:** Fortinbra (via Coordinator). Survey findings written to `.squad/agents/edward/bt-analysis.md`.

**GPDriver abstraction:** `headers/gpdriver.h` defines a pure-virtual `GPDriver` interface. `DriverManager` singleton (singleton pattern, `headers/drivermanager.h`) maps `InputMode` enum → one concrete driver at boot. All 17 USB profiles are subclasses. `src/usbdriver.cpp` is pure glue — delegates all TinyUSB callbacks to `DriverManager::getDriver()`. The interface is USB-centric (descriptor callbacks, HID reports, vendor XFER). No output transport abstraction above GPDriver exists.

**Mode selection is boot-time only:** `DriverManager::setup(inputMode)` called once in `gp2040.cpp:195`. No runtime switching. Mode changes require flash save + reboot.

**GPIO retro console support is INPUT-only:** `SNESpadInput` and `TG16padInput` addons READ from retro controllers (SNES/NES via clock/latch/data; TG16 via OE/select/4-data). The "Reflex CTRL" boards use these to adapt retro controller ports to USB output. There is NO GPIO output code to emulate a retro console peripheral. This is a gap.

**Zero Bluetooth code in the project:** No btstack, no CYW43 BT APIs, no INPUT_MODE_BLUETOOTH, no stubs. The `BLUETOOTH_PAIR_REQUEST` constant in `SwitchProDriver.cpp:300` is a USB HID command ID from the Nintendo Switch, not BT functionality. CYW43 is used for WiFi only (`lwip-port`, RNDIS).

**CYW43 BT hardware is available:** CYW43439 on PicoW has native BT Classic + BLE. Pico SDK provides `pico_btstack`. USB HID (USB PHY) and BT HID (CYW43) CAN coexist simultaneously — different hardware. The constraint that USB and BT cannot run simultaneously does NOT apply here.

**Pico2W blocker confirmed again:** No `Pico2W` config. Blocker is CYW43 stack validation for RP2350, not base firmware. Base firmware compiles fine for RP2350.

**For runtime multi-output:** Need a new `GPOutputTransport` abstraction above `GPDriver`, an `OutputManager` that can fan-out to USB + BT + GPIO simultaneously. `pico_btstack_hid_device` linkage needed (conditional on PicoW boards). PIO state machines needed for GPIO retro console output protocols (SNES, N64, Dreamcast).

**Key file references:**
- `proto/enums.proto:143–160` — InputMode enum
- `src/drivermanager.cpp` — driver factory
- `src/gp2040.cpp:281–353` — main loop
- `src/addons/snes_input.cpp` — SNESpadInput (GPIO input, not output)
- `src/addons/tg16_input.cpp` — TG16padInput (GPIO input, not output)
- `configs/PicoW/PicoW.cmake` — only wireless board config
