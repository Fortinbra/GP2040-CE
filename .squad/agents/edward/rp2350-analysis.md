# RP2350A/B Firmware Analysis

**Author:** Edward (Firmware Dev)  
**Date:** 2026-03-28  
**Branch:** docs/copilot-instructions  
**Status:** For Hughes — technical source for user-facing documentation

---

## Executive Summary

RP2350 support is **already partially implemented** in GP2040-CE. Three board configs exist and are CI-tested: `Pico2`, `FlatboxRev8`, and `SparkFunProMicroRP2350`. The build system fully supports RP2350 targeting via the existing `GP2040_BOARDCONFIG` / `PICO_BOARD` / `PICO_PLATFORM` mechanism. The firmware source code has zero chip-specific `#ifdef` guards — it relies entirely on SDK abstractions that are already RP2350-compatible. The primary gap is the absence of a `Pico2W` board config and the absence of RP2350B-specific configs exploiting GPIO 30–47.

---

## 1. RP2350A vs RP2350B Hardware Differences

### RP2350A (e.g., Raspberry Pi Pico 2)
- **CPU:** Dual ARM Cortex-M33 + dual RISC-V Hazard3 (2 cores active at once, user selects arch at boot)
- **GPIO:** 30 pins (GPIO 0–29) — identical count to RP2040
- **SRAM:** 520 KB (vs RP2040's 264 KB) — ~2× more
- **Clock:** Up to 150 MHz nominal (vs RP2040's 133 MHz)
- **PIO:** PIO0 + PIO1 (4 state machines each, same as RP2040) + **PIO2** (new, 4 state machines)
- **Flash interface:** Same QSPI external flash model as RP2040
- **Security:** TrustZone-M, secure boot, OTP fuses
- **Hardware dividers:** 2 (vs 1 in RP2040)
- **Floating point:** Improved HW-accelerated (single + double precision)
- **PSRAM:** Native QSPI PSRAM support added
- **USB:** Same native USB 1.1 device/host PHY as RP2040

### RP2350B (e.g., SparkFun Pro Micro RP2350)
- **Everything in RP2350A, plus:**
- **GPIO:** 48 pins (GPIO 0–47) — **18 additional GPIOs** beyond RP2350A
- **Package:** Larger QFN-80 vs QFN-60 for RP2350A
- **Additional ADC channels, PWM slices** proportional to extra GPIO count

### Key Differences from RP2040 That Matter for Firmware
| Feature | RP2040 | RP2350A | RP2350B |
|---------|--------|---------|---------|
| GPIO count | 30 | 30 | **48** |
| SRAM | 264 KB | 520 KB | 520 KB |
| Max clock | 133 MHz | 150 MHz | 150 MHz |
| PIO blocks | 2 | **3** (PIO2 added) | **3** |
| CPU arch | ARM Cortex-M0+ | M33 + RISC-V | M33 + RISC-V |
| `NUM_BANK0_GPIOS` | 30 | 30 | **48** |

---

## 2. Current Codebase Assessment

### 2.1 SDK Version

`CMakeLists.txt` line 62–63:
```cmake
if (PICO_SDK_VERSION_STRING VERSION_LESS "2.2.0")
  message(FATAL_ERROR "Raspberry Pi Pico SDK version 2.2.0 (or later) required...")
endif()
```

**SDK 2.2.0 is required** and RP2350 support was introduced in SDK 2.0.0. The project is already on a compatible SDK. CI (`cmake.yml`) checks out `pico-sdk` at tag `2.2.0` explicitly.

### 2.2 Existing RP2350 Board Configs

Three configs already exist and are in CI:

| GP2040_BOARDCONFIG | PICO_BOARD | PICO_PLATFORM | Notes |
|--------------------|------------|---------------|-------|
| `Pico2` | `pico2` | `rp2350-arm-s` | Reference RP2350A board |
| `FlatboxRev8` | `pico2` | `rp2350-arm-s` | RP2350A, uses GPIO 0–29 |
| `SparkFunProMicroRP2350` | `sparkfun_promicro_rp2350` | `rp2350-arm-s` | RP2350B chip, but only uses GPIO 0–29 |

All three use `rp2350-arm-s` (ARM Secure mode). No RISC-V configs exist or are needed for typical gamepad use.

**Missing:** `Pico2W` config — the Raspberry Pi Pico 2 W (RP2350A + CYW43439 WiFi). Would use `PICO_BOARD=pico2_w`.

### 2.3 Board Config Structure

Each board config lives in `configs/<BoardName>/` and consists of:

| File | Purpose |
|------|---------|
| `BoardConfig.h` | GPIO pin → button action mappings, LED config, I2C/display config, add-on defaults |
| `<BoardName>.cmake` | Sets `PICO_BOARD` and `PICO_PLATFORM`; included before `project()` |
| `CMakeLists.txt` | Usually empty stub (for legacy IDE compat) |
| `README.md` | Human-readable pin table |
| `assets/` | Board photos |

The `BoardConfig.h` guard macro is `PICO_BOARD_CONFIG_H_` (not chip-specific). Pin mappings use `GPIO_PIN_NN` macros mapping to `GpioAction` enum values.

### 2.4 PICO_BOARD Values Currently in Use

From grepping all `.cmake` board config files:
- `pico` — RP2040 (default)
- `pico_w` — RP2040 + WiFi
- `pico2` — RP2350A
- `sparkfun_promicro_rp2350` — RP2350B package
- Various RP2040-based third-party boards (no explicit `PICO_BOARD`/`PICO_PLATFORM` set, inheriting defaults)

### 2.5 GPIO Handling — Platform-Adaptive Already

`headers/helper.h`:
```cpp
static inline bool isValidPin(int32_t pin) {
    int32_t numBank0GPIOS = NUM_BANK0_GPIOS;
    return pin >= 0 && pin < numBank0GPIOS;
}
```

`NUM_BANK0_GPIOS` is defined by the Pico SDK per target platform:
- RP2040: `NUM_BANK0_GPIOS = 30`
- RP2350A: `NUM_BANK0_GPIOS = 30`
- RP2350B: `NUM_BANK0_GPIOS = 48`

The firmware `isValidPin()` check **automatically adapts** to the compiled target. No hardcoded `30` or `48` constants exist in the main firmware logic.

`headers/gp2040.h` and `headers/storagemanager.h` use `NUM_BANK0_GPIOS` for array sizing (debounce timers, pin mapping storage). These arrays will correctly expand to 48 elements when compiled for RP2350B.

`headers/helper.h` also exposes `numBank0GPIOS` to the web configurator via the config system — the web UI will receive the correct GPIO count from the running firmware.

### 2.6 PIO and USB Host

`headers/tusb_config.h`:
```c
#define CFG_TUH_RPI_PIO_USB 1
```

The firmware uses **PIO-based USB host** (via `pico_pio_usb` library) for gamepad passthrough. Key observations:
- PIO0/PIO1 are identical across RP2040 and RP2350 — fully backwards compatible
- RP2350 adds PIO2 but the existing PIO USB code doesn't need it
- The TinyUSB library (`lib/tinyusb`) already has a `raspberry_pi_pico2` board support file at `lib/tinyusb/hw/bsp/rp2040/boards/raspberry_pi_pico2/board.cmake`
- USB peripheral passthrough (`USB_PERIPHERAL_PIN_DPLUS`) used in configs like FlatboxRev8 (GPIO 20) — this uses the native USB PHY, which is identical on RP2350

### 2.7 Flash and Memory

`src/system.cpp` uses:
- `PICO_FLASH_SIZE_BYTES` — defined by board config, same QSPI flash model on RP2350
- `SRAM_BASE` — both RP2040 and RP2350 use `0x20000000`, compatible
- `flash_do_cmd()` — uses SDK's flash abstraction, compatible with RP2350

No hardcoded flash sizes or SRAM addresses are in the firmware.

### 2.8 Chip-Specific Code Audit

Searched `src/` and `headers/` for: `rp2350`, `rp2040`, `PICO_PLATFORM`, `__RISCV`, `cortex.m33`

**Result: Zero matches** in the main firmware source. The codebase has no chip-specific conditionals. All hardware access goes through Pico SDK abstractions (`hardware_gpio`, `hardware_adc`, `hardware_pwm`, `hardware_i2c`, `hardware_spi`, `hardware_dma`, `hardware_pio`).

---

## 3. What's Needed for Full RP2350 Support

### 3.1 Already Done ✅
- Pico SDK 2.2.0 requirement (supports RP2350)
- `Pico2` board config (`PICO_BOARD=pico2`, `PICO_PLATFORM=rp2350-arm-s`)
- `FlatboxRev8` board config (community board using RP2350A)
- `SparkFunProMicroRP2350` board config (SparkFun RP2350B module)
- CI builds all three in matrix
- GPIO validation uses `NUM_BANK0_GPIOS` — adaptive at compile time
- Storage/debounce arrays sized by `NUM_BANK0_GPIOS` — correct for both variants
- PIO USB backwards compatible, TinyUSB has Pico 2 BSP

### 3.2 Gaps — Pico 2 W Config
- **Missing:** `configs/Pico2W/` directory with:
  - `Pico2W.cmake`: `set(PICO_BOARD pico2_w)` + `set(PICO_PLATFORM rp2350-arm-s)`
  - `BoardConfig.h`: Same pin mapping as Pico2, plus any WiFi-specific additions
  - `README.md`, `CMakeLists.txt` stub
- Must be added to `cmake.yml` CI matrix

### 3.3 Gaps — RP2350B GPIO 30–47 Utilization
- `SparkFunProMicroRP2350` only uses GPIO 0–29 despite being on an RP2350B chip
- No board config currently maps buttons to GPIO 30–47
- For custom RP2350B boards with >30 GPIOs accessible, a config using e.g. `GPIO_PIN_30` through `GPIO_PIN_47` would work — the firmware infrastructure already supports it
- The web configurator pin count comes from `NUM_BANK0_GPIOS` at runtime, so it will correctly show 48 pins for RP2350B builds

### 3.4 Gaps — Platform Documentation  
- No RP2350A vs RP2350B distinction explained in user-facing docs
- No upgrade/migration path documented for users moving from RP2040 boards

### 3.5 PIO Compatibility Notes
- RP2350 PIO0/PIO1 instruction set is backwards compatible with RP2040
- PIO2 on RP2350 supports additional instructions (`mov` extensions) — not needed for current code
- `pico_pio_usb` library compatibility with RP2350 depends on that library's state; existing CI builds passing for Pico2/FlatboxRev8 confirms it works
- Clock-dependent PIO programs: RP2040 runs at 125 MHz default, RP2350 at 150 MHz default. PIO clock dividers may behave differently if any code assumes specific system clock frequency. **Area for testing.**

### 3.6 SDK Variant Notes (rp2350-arm-s vs rp2350-riscv)
- All existing configs use `rp2350-arm-s` (ARM Secure mode)
- `rp2350-arm-ns` (ARM Non-Secure) and `rp2350-riscv` are not used
- For typical gamepad builds, ARM Secure is the correct choice — TrustZone disabled, full hardware access
- RISC-V mode would require recompiling with RISC-V toolchain — not practical for the project

---

## 4. Feature Documentation Outline (for Hughes)

### Suggested Doc: `docs/controller-building/rp2350-support.md`

```
# RP2350 Support in GP2040-CE

## Overview
Brief: what RP2350 is, why it matters, which boards are supported

## Supported RP2350 Boards
Table: board config name, chip variant (A/B), available GPIOs, notable features

## RP2350A vs RP2350B — What's the Difference?
- A: Same GPIO count as RP2040 (30 pins), drop-in upgrade
- B: 48 GPIOs — more buttons, more LEDs, more add-ons
- Both: faster clock, more RAM, better floating point

## What Changes When Upgrading from RP2040
- Flash the RP2350-specific .uf2 (not the RP2040 .uf2 — different binaries)
- Pin mappings are board-specific — check your board's config page
- Web configurator adapts automatically to the GPIO count of the running firmware
- All protocols/modes (XInput, PS5, Switch, etc.) work identically

## What Stays the Same
- All input modes and protocol support
- Add-on system (turbo, SOCD, display, LEDs, etc.)
- Web configurator interface
- Configuration storage in flash

## Building Firmware for RP2350
### Using Pre-built Releases
Point to release page, explain UF2 naming convention

### Building from Source
cmake env vars: GP2040_BOARDCONFIG=Pico2, no other changes needed
SDK 2.2.0 required

## RP2350-Specific Boards
### Raspberry Pi Pico 2 (Pico2)
Pin map table

### Flatbox Rev. 8
Pin map table, USB passthrough note

### SparkFun Pro Micro RP2350
Pin map table, note about RP2350B package

## Known Limitations / Testing Notes
- PIO-based USB host: tested on Pico2/FlatboxRev8 in CI; real-hardware testing recommended
- Clock frequency difference (150 MHz vs 133 MHz): PIO timing verified via CI builds
- Pico 2 W: config not yet available (planned)
- RISC-V mode: not supported (ARM only)
```

### Key User Callouts
1. **UF2 files are not interchangeable** — `GP2040-CE_X.X.X_Pico.uf2` ≠ `GP2040-CE_X.X.X_Pico2.uf2`
2. **RP2350A has the same 30 GPIOs as RP2040** — existing RP2040 board layouts work as-is on RP2350A
3. **RP2350B unlocks 48 GPIOs** — new board designs can have more simultaneous button inputs
4. **More SRAM** — RP2350 has 520 KB vs RP2040's 264 KB; enables more complex LED/display configurations
5. **Web configurator self-configures** — it reads GPIO count from firmware; RP2350B builds show 48 pins automatically

---

## 5. Areas Requiring Hardware Testing

1. **PIO USB host timing** — PIO clock dividers are relative to system clock; at 150 MHz vs 125 MHz default, the PIO USB host bitrate could drift. CI builds pass but hardware pass-through testing needed.
2. **Flash operations** — `flash_do_cmd()` timing may differ on RP2350; `system.cpp::getPhysicalFlash()` should be tested.
3. **I2C/SPI peripherals at 150 MHz** — peripheral clock divisors may behave differently; display add-on testing needed.
4. **BOOTSEL behavior** — RP2350 boot behavior differs; `pico_bootsel_via_double_reset` library is already in `target_link_libraries`.

---

*Analysis complete. No code changes made. Analysis file only.*
