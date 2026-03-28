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

### 2026-03-28: RM2 Module CYW43 GPIO Analysis

**Tasked by:** Fortinbra (via Coordinator). Full findings in `.squad/agents/edward/rm2-analysis.md`.

**Exact GPIO pins confirmed from SDK 2.2.0 `pico_w.h` and `pico2_w.h`:** Both boards use IDENTICAL CYW43 pin assignments — GPIO 23 (`WL_REG_ON`/power enable), GPIO 24 (`DATA_OUT`/`DATA_IN`/`HOST_WAKE` — all three share GPIO 24 due to half-duplex gSPI), GPIO 25 (`CS`), GPIO 29 (`CLOCK`; shared with `PICO_VSYS_PIN` for ADC). Total: 4 RP2040 bank0 GPIOs consumed. LED and VBUS sense go through CYW43 internal WL_GPIO0/WL_GPIO2 — NOT on any RP2040 GPIO.

**`PICO_CYW43_SUPPORTED=1` is the master switch.** Set via `pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)` in the board `.h` header. This unlocks all `pico_cyw43_driver`, `pico_cyw43_arch`, and `pico_btstack_*` CMake targets. Pin defaults set via `#ifndef CYW43_DEFAULT_PIN_WL_*` guards in board header — overridable via custom board header.

**"Same pins as Pico W" = easiest path.** A custom board wiring RM2 to GPIO 23/24/25/29 can use `PICO_BOARD=pico_w` (RP2040) or `PICO_BOARD=pico2_w` (RP2350A) directly. No firmware changes, no custom board header, no driver modifications. The existing `configs/PicoW/BoardConfig.h` works as-is (it already omits GPIO 23–25, 29).

**Alternate GPIO wiring (RP2350B only) requires PIO driver verification.** The `cyw43_bus_pio_spi.pio` PIO program governs GPIO constraints for the CYW43 interface. Non-standard pin placement may require SDK-level PIO program modification. This is unsupported for non-Pico-W boards and is an open question.

**Open TBD items for Hughes:** RM2 VBUS sense mechanism (CYW43 WL_GPIO or separate pin?), RM2 LED pin (WL_GPIO0 vs RP2040 GPIO?), PIO program GPIO constraint for RP2350B alternate wiring, Pimoroni RM2 schematic confirmation.

### 2026-03-28: BT Battery Protocol Analysis — Classic HID vs BLE HID

**Tasked by:** Fortinbra (via Coordinator). Full findings in `.squad/agents/edward/bt-battery-protocol-analysis.md`.

**`battery_service_server_set_battery_value()` is BLE GATT only.** Confirmed from BTStack SDK source: the function lives in `lib/btstack/src/ble/gatt-service/battery_service_server.h`. It requires `att_server_init()`, `sm_init()`, and an active BLE connection. It has no effect over a BT Classic L2CAP HID connection. The SDK's `hog_keyboard_demo.c` (BLE HID) uses it; `hid_keyboard_demo.c` (Classic HID) does not include `battery_service_server.h` at all.

**BT Classic HID battery = HID descriptor Feature report.** Correct mechanism: add a `Battery Strength` item (Usage Page 0x06 / Generic Device Controls, Usage 0x20) as a `Feature` report to the HID descriptor. Host reads it via `GET_REPORT(Feature)` over L2CAP control channel. Handled in BTStack via `hid_device_register_report_request_callback()`. No GATT, no UUID 0x180F, no BLE required.

**`bluetooth-support.md` battery section is incorrect.** The Battery Level Reporting section documents the GATT path (`battery_service_server_init`, UUID 0x180F, GATT notifications) but the doc chose BT Classic as the transport. This is a direct contradiction. The section must be replaced with HID descriptor battery reporting. The ADC reading logic (GPIO29, voltage divider, LiPo %) is transport-agnostic and unchanged.

**BTStack API surface for Classic HID battery:** `hid_device_register_report_request_callback(cb)` — in the callback, respond to `HID_REPORT_TYPE_FEATURE` + battery Report ID by returning `readBatteryPercent()`. Full descriptor and `hid_sdp_record_t` already include the descriptor so the host discovers the battery Feature report via SDP.

**BLE HID future note:** If BLE HID is added in a future phase, GATT Battery Service (UUID 0x180F) + `battery_service_server_set_battery_value()` + `pico_btstack_ble` CMake target would then be correct. The doc should note this explicitly as a future path.

### 2026-03-28: Power Management & Battery Reporting Analysis

**Tasked by:** Fortinbra (via Coordinator). Full findings in `.squad/agents/edward/power-battery-analysis.md`.

**No Pimoroni board config exists.** Zero Pimoroni entries among 56 `configs/` directories. A new `configs/PimoroniPicoLipo2XLW/` must be created with `PICO_BOARD=pico2_w` / `PICO_PLATFORM=rp2350-arm-s`.

**`GamepadAuxPower` struct already exists** in `headers/gamepad/GamepadAuxState.h` with `charging`, `pluggedIn`, and `level` fields — hardcoded to `pluggedIn=true`, `level=100` in `src/gp2040.cpp:75–78`. Battery reporting replaces this hardcode.

**ADC pattern confirmed** (from `src/addons/analog.cpp`): `adc_gpio_init(pin)` → `adc_select_input(pin - 26)` → `adc_read()` → 12-bit (0–4095). ADC_PIN_OFFSET = 26. Battery pin is GPIO29/ADC3. Voltage formula: `V_BAT = (raw/4095.0) * 3.3 * divider_factor`. LiPo range: 3.0 V (0%) to 4.2 V (100%). Pimoroni standard divider = 3.0 (200k/100k) — needs schematic confirmation.

**BTStack Battery Service:** `battery_service_server_init(uint8_t 0–100)` + `battery_service_server_set_battery_value(uint8_t)`. Lives in Pico SDK's BTStack submodule — NOT in GP2040-CE's `lib/`. BLE GATT service UUID 0x180F, characteristic 0x2A19. Requires `#import <battery_service.gatt>` in GATT profile file.

**Zero power management exists.** No `pico/sleep.h`, no `cyw43_wifi_pm()`, no sleep/dormant entry anywhere in the codebase. BT+battery builds are the first power-constrained paradigm.

**Proposed state machine:** USB_CONNECTED (full power, report 100%) → ACTIVE (battery ADC, CYW43_PERFORMANCE_PM) → IDLE (clock reduction, CYW43_DEFAULT_PM or CYW43_AGGRESSIVE_PM, BT sniff mode 40 ms) → DEEP_SLEEP (RP2350 DORMANT via `sleep_goto_dormant_until_pin()`, CYW43 deinit first, wake on button GPIO edge, re-init CYW43 + BT on wake).

**CYW43 DORMANT caveat:** CYW43 must be gracefully deinit'd before RP2350 enters DORMANT. Re-init + BT re-advertisement on wake adds unknown latency (estimated 500 ms–2 s) — needs hardware testing.

**Namespace conflict mitigation confirmed:** Translation-unit isolation (BT driver code in `src/drivers/bt/` includes only BTStack headers; existing TinyUSB files unchanged). `gp_hid_report_type_t` project-local enum optional for abstraction boundaries. `#undef` approach rejected. Gate all BT/power code on `PICO_CYW43_SUPPORTED`.

### 2026-03-28: TinyUSB / BTStack Namespace Conflict Analysis

**Tasked by:** Fortinbra (via Coordinator). Full findings in `.squad/agents/edward/bt-namespace-analysis.md`.

**Primary confirmed collision:** `hid_report_type_t` is defined as a `typedef enum` by BOTH TinyUSB (`lib/tinyusb/src/class/hid/hid.h:84`) and BTStack (`<sdk>/lib/btstack/src/btstack_hid.h:109`). Same name, different first enum value (`HID_REPORT_TYPE_INVALID` vs `HID_REPORT_TYPE_RESERVED`). Including both in the same translation unit causes a compile error. This type is pervasive in GP2040-CE's existing HID driver layer.

**No CMake mutual exclusion:** Neither `pico_btstack` nor `tinyusb` CMakeLists in SDK 2.2.0 enforces a FATAL_ERROR when both are linked simultaneously. They can coexist in the same firmware binary. The conflict is header-level (compile-time), not link-time.

**Other collisions investigated:** `hid_protocol_mode_t` (different typedef names — no collision), `HID_USAGE_PAGE_*` (different design patterns — no collision), `TU_ATTR_PACKED` (BTStack doesn't define `PACKED` the same way — no collision), `HID_KEY_*` (TinyUSB-only — no collision).

**RP2350 + CYW43 CONFIRMED for BTStack:** `pico2_w.h` in SDK 2.2.0 sets `PICO_CYW43_SUPPORTED=1` and `PICO_PLATFORM=rp2350`. `pico_btstack` cmake registers all sub-libraries when CYW43 is supported. Previous uncertainty was incorrect. Pimoroni Pico Lipo 2 XL W (RP2350A+CYW43) is a confirmed working hardware target per Fortinbra.

**Recommended approach:** Translation-unit isolation — BT driver code lives in `src/drivers/bt/` and includes only BTStack headers. Existing TinyUSB files unchanged. Gated by `PICO_CYW43_SUPPORTED` (SDK-provided, no custom flag needed). Optional: `gp_hid_report_type_t` project-local enum at the abstraction boundary to avoid any cross-stack type exposure.

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

### 2026-03-28: GPIO Retro Console OUTPUT Architecture Survey

**Tasked by:** Fortinbra (via Coordinator). Survey findings written to `.squad/agents/edward/gpio-analysis.md`.

**SNES/NES protocol timing (from `lib/SNESpad/SNESpad.cpp`):** Console drives CLOCK and LATCH. Latch HIGH = 12 µs, LOW setup = 6 µs. Clock cycle: 6 µs LOW + 6 µs HIGH = 12 µs per bit. 16 bits for SNES (12 buttons + 4 device ID), 8 bits for NES. Data is active-low. For OUTPUT mode, MCU flips: DATA becomes output, CLOCK/LATCH become inputs (IRQ-driven). Bit-bang feasible at this timing; PIO is jitter-free alternative.

**TG16/PC Engine protocol (from `src/addons/tg16_input.cpp`):** 6 pins. Console drives OE (active-low) and SELECT. For output, MCU drives DATA0–DATA3 (4-bit nibble) based on SELECT state. Timing is loose (1 ms sleep in existing code). Bit-bang sufficient.

**N64 protocol:** Single-wire NRZ serial at 1 MHz (1 µs/bit). 3.3V native — no level shifting needed. PIO mandatory — 500 ns per-bit tolerance cannot be met with bit-bang or interrupts. Community PIO programs exist for reference.

**Dreamcast MAPLE bus:** 2-wire (SDCKA/SDCKB), half-duplex, ~2 Mbps, 3.3V native. Bidirectional with CRC and frame protocol. No code exists in the project. Two PIO SMs needed (TX + RX). Most complex retro console target — frame parsing, CRC, response timing within ~200 µs window.

**Voltage levels — critical hardware constraint:**
- NES, SNES, Genesis/MD, TG16/PC Engine: **5V TTL** — level shifting required both directions (RP2040 inputs are NOT 5V tolerant; 74AHCT125 for output, 74LVC245 for bidirectional)
- N64: **3.3V** — RP2040 native, no level shifting
- Dreamcast: **3.3V** — RP2040 native, no level shifting

**PIO state machine budget (RP2040):** 8 total. Current usage: PIO-USB host = 3 SMs (default config, only when USB_PERIPHERAL_ENABLED), WS2812 = 1 SM (pio0/sm0 in neopicoleds.cpp:282). A full retro-output implementation (N64 + Dreamcast + LEDs) = 4 SMs. Not a bottleneck on RP2040; RP2350B has 12 SMs.

**GPIOOutputAddon architecture:** Must be a `GPAddon` subclass (NOT a new `GPDriver`). This enables USB + GPIO simultaneous operation — GPIO output runs in `process()` after USB report is sent. No new `InputMode` enum value needed; enabled via a new `GPIOOutputOptions` block in `AddonOptions` protobuf. Pin assignments configurable per protocol via web configurator.

**PicoW GPIO constraints:** GPIO 23–25 reserved for CYW43 SPI/SDIO bus. Free GPIO candidates: 22, 26, 27, 28 (4 pins). Sufficient for NES/SNES/N64/Dreamcast/TG16 but tight for Genesis 6-button (needs 9 pins).

**Genesis/Mega Drive:** Parallel + SELECT mux. 7–9 signal lines. Console drives SELECT at ~60 Hz. Bit-bang fully sufficient — no PIO needed. Level shifting required (5V).

**Key file references:**
- `lib/SNESpad/SNESpad.cpp` — SNES/NES timing constants (12 µs latch, 6 µs clock)
- `lib/pico_pio_usb/src/pio_usb_configuration.h` — PIO-USB SM allocation (PIO0 sm0 TX, PIO1 sm0/sm1 RX)
- `lib/NeoPico/src/NeoPico.h` + `src/addons/neopicoleds.cpp:282` — WS2812 on pio0/sm0
- `headers/helper.h:38–40` — `isValidPin()` using `NUM_BANK0_GPIOS`
- `configs/Pico/BoardConfig.h` — standard Pico pin allocation (GPIO 2–21 for buttons, 0–1 I2C, 28 LEDs)
- `configs/PicoW/BoardConfig.h` — Pico W same layout; GPIO 23–25 reserved (CYW43)
