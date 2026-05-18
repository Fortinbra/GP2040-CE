# RP2350 Support

## Overview

GP2040-CE supports the **Raspberry Pi RP2350** family through Pico SDK board/platform targets and per-board GP2040 overlays. The RP2350 is the successor to the RP2040 and offers improvements in CPU performance, memory, and GPIO availability.

In this repo, RP2350 behavior is selected in two layers:
- `GP2040_BOARDCONFIG` selects the GP2040 board folder and overlay (`configs/<name>/<name>.cmake`, `BoardConfig.h`).
- `PICO_BOARD` and `PICO_PLATFORM` select Pico SDK board/platform behavior.

No RP2350-specific firmware source fork is required for normal builds; variant/package-specific behavior comes from the selected SDK board definition.

**Key benefit for users:** RP2350A is a drop-in upgrade for RP2040-based boards, while RP2350B unlocks up to 48 GPIO pins, enabling more simultaneous button and feature inputs on new board designs.

---

## RP2350A vs RP2350B

Both variants are fully supported. The main difference is GPIO availability:

| Feature | RP2350A | RP2350B |
|---------|---------|---------|
| **GPIO Pins** | 30 (GPIO 0–29) | 48 (GPIO 0–47) |
| **SRAM** | 520 KB | 520 KB |
| **Flash Interface** | QSPI (same as RP2040) | QSPI (same as RP2040) |
| **Max Clock** | 150 MHz | 150 MHz |
| **CPU Options** | Dual ARM Cortex-M33 + dual RISC-V | Dual ARM Cortex-M33 + dual RISC-V |
| **PIO Blocks** | 3 (PIO0, PIO1, PIO2) | 3 (PIO0, PIO1, PIO2) |

**RP2350A** is ideal for upgrading existing RP2040 board designs—it offers the same 30 GPIO pins, so existing button layouts work without modification.

**RP2350B** is suited for new board designs that need more GPIO inputs. The extra pins (GPIO 30–47) can be mapped to additional buttons, LEDs, or other features through custom board configurations.

---

## Variant Selection Flow (Guaranteed vs Inferred)

For RP2350 targets, the selection flow is:
1. `GP2040_BOARDCONFIG=<BoardName>` picks the GP2040 overlay and pin map defaults.
2. That overlay sets `PICO_BOARD` and `PICO_PLATFORM` (RP2350 overlays set `PICO_PLATFORM=rp2350-arm-s`).
3. The Pico SDK board header selected by `PICO_BOARD` determines board/chip macros and limits such as `NUM_BANK0_GPIOS`.

What this guarantees:
- The platform is RP2350 ARM Secure mode when `PICO_PLATFORM=rp2350-arm-s` is set.
- GPIO/UI limits in firmware follow compile target macros (not board marketing names).

What is inferred (SDK-dependent):
- RP2350 package variant claims (A vs B) are guaranteed only when the selected SDK board definition explicitly implies that variant.
- If multiple GP2040 configs map to the same SDK board target, variant/package assumptions should be treated as inferred unless separately verified.

---

## Supported Boards

GP2040-CE includes configuration support for the following RP2350-based boards:

| Board | Chip Variant Status | Config Name | GPIO Count at Build Time | Notable Features |
|-------|----------------------|-------------|---------------------------|------------------|
| **Raspberry Pi Pico 2** | RP2350A (SDK board target: `pico2`) | `Pico2` | 30 | Reference RP2350A board; pin layout identical to original Pico |
| **Flatbox Rev. 8** | RP2350A (via `pico2` target in overlay) | `FlatboxRev8` | 30 | USB peripheral passthrough for arcade stick arcade mode |
| **SparkFun Pro Micro RP2350** | RP2350B (SDK board target: `sparkfun_promicro_rp2350`) | `SparkFunProMicroRP2350` | SDK-dependent (`NUM_BANK0_GPIOS` from target) | Compact form factor; RP2350B-oriented target |
| **Raspberry Pi Pico 2 W** | RP2350A (SDK board target: `pico2_w`) | `Pico2W` | 30 | CYW43439 wireless; same pin layout as Pico W |
| **Pimoroni Pico Lipo 2 XL W** | SDK-dependent/inferred (current overlay uses `pico2_w`) | `PimoroniPicoLipo2XLW` | Currently 30 with `pico2_w` target | CYW43439 wireless; onboard LiPo charging. See [Pimoroni Pico Lipo 2 XL W Board Support](./pimoroni-pico-lipo-2xl-w-support.md) for board-specific pin and power details. |

All boards are CI-tested and release-ready.

Both the Raspberry Pi Pico 2 W and the Pimoroni Pico Lipo 2 XL W configurations are included. Full Bluetooth HID support for CYW43439-equipped boards is planned in a future release. Pimoroni-specific behavior is centralized in [Pimoroni Pico Lipo 2 XL W Board Support](./pimoroni-pico-lipo-2xl-w-support.md).

### Maintainer Variant Validation Checklist

Use this checklist before asserting RP2350A/RP2350B in docs or release notes:

1. Confirm overlay mapping in `configs/<Board>/<Board>.cmake` (`PICO_BOARD`, `PICO_PLATFORM`).
2. Confirm the selected SDK board header under `$PICO_SDK_PATH/src/boards/include/boards/` and inspect its macros.
3. Verify compile-time GPIO limit by checking `NUM_BANK0_GPIOS` for that target (build output, generated compile definitions, or direct header inspection).
4. Validate runtime behavior in firmware/web config by confirming pin loops/options align with `NUM_BANK0_GPIOS`.
5. If a board config reuses another SDK board target, label variant claims as SDK-dependent/inferred unless separate hardware/package proof is linked.

---

## Minimum Requirements

- **Pico SDK 2.2.0 or later** — required by the GP2040-CE build system for all targets (enforced in `CMakeLists.txt`; the build will fail with a fatal error if an older SDK is detected)
- **CMake 3.10+** — standard requirement
- **ARM Embedded Toolchain** — e.g., `arm-none-eabi-gcc`

> **Note on SDK version:** SDK 2.2.0 is the minimum for the entire GP2040-CE project, not just RP2350 boards. RP2040 boards also require SDK 2.2.0. The project-wide version is pinned in `CMakeLists.txt` (`set(sdkVersion 2.2.0)`) and in CI.

### Verifying your SDK version

Check the Pico SDK version file directly:
```bash
cat $PICO_SDK_PATH/pico_sdk_version.cmake
# Look for: set(PICO_SDK_VERSION_STRING "2.2.0")
```

Or confirm during CMake configuration — the configure step prints the SDK version and will halt with a `FATAL_ERROR` if the version is below 2.2.0:
```bash
cmake -B build -S .
# Output includes: "Pico SDK is 2.2.0"
```

---

## Building for RP2350

### Using Pre-built Releases

Download the appropriate UF2 firmware file from the [GP2040-CE releases page](https://github.com/FeralAI/GP2040-CE/releases).

**Important:** UF2 files are **not interchangeable** between RP2040 and RP2350 boards. Flashing the wrong UF2 will not work:
- `GP2040-CE_*_Pico.uf2` → for RP2040-based Pico
- `GP2040-CE_*_Pico2.uf2` → for RP2350A-based Pico 2
- `GP2040-CE_*_FlatboxRev8.uf2` → for RP2350A-based Flatbox
- `GP2040-CE_*_SparkFunProMicroRP2350.uf2` → for RP2350B-based SparkFun module
- `GP2040-CE_*_Pico2W.uf2` → for RP2350A-based Pico 2 W
- `GP2040-CE_*_PimoroniPicoLipo2XLW.uf2` → for Pimoroni Pico Lipo 2 XL W config (variant/package labeling is SDK-target dependent in this repo)

### Building from Source

To build firmware for an RP2350 board, specify the board configuration via `GP2040_BOARDCONFIG`:

```bash
# For Pico 2 (RP2350A)
cmake -DGP2040_BOARDCONFIG=Pico2 -B build -S .
cmake --build build

# For Flatbox Rev. 8 (RP2350A)
cmake -DGP2040_BOARDCONFIG=FlatboxRev8 -B build -S .
cmake --build build

# For SparkFun Pro Micro RP2350 (RP2350B)
cmake -DGP2040_BOARDCONFIG=SparkFunProMicroRP2350 -B build -S .
cmake --build build
```

The board overlay selects `PICO_PLATFORM` (RP2350 overlays use `rp2350-arm-s`), while GPIO limits and variant/package details are SDK-target dependent and come from the selected `PICO_BOARD` definition.

**Note:** If Pico SDK is not installed, you can set `PICO_SDK_PATH`:
```bash
cmake -DPICO_SDK_PATH=/path/to/pico-sdk -DGP2040_BOARDCONFIG=Pico2 -B build -S .
```

---

## Pin Mapping and GPIO

Each RP2350 board configuration defines its own GPIO-to-button mappings. Refer to the board's configuration page or `README.md` for pinout diagrams and button assignments.

### GPIO Validation

GP2040-CE automatically validates pin assignments against the target chip's available GPIO count:
- **SDK-dependent:** Valid pins are `0..(NUM_BANK0_GPIOS - 1)` for the compiled target
- Typical result: RP2350A-oriented targets expose 0–29; RP2350B-oriented targets may expose 0–47 when the SDK board definition does

The firmware will reject any configuration that assigns buttons to unavailable pins.

### Web Configurator

The web configurator reads pin availability from firmware behavior tied to compile-target limits (`NUM_BANK0_GPIOS`) and displays only valid pins for configuration. GPIO option count is therefore SDK-target dependent.

---

## Creating a Custom RP2350 Board Configuration

If you're designing a custom RP2350-based gamepad, you can create a new board configuration:

### Directory Structure

Create a new directory in `configs/`:

```
configs/MyCustomBoard/
├── BoardConfig.h          # Pin-to-button mappings
├── MyCustomBoard.cmake    # Build configuration
├── CMakeLists.txt         # (can be empty stub)
└── README.md              # Human-readable pinout
```

### 1. Create `MyCustomBoard.cmake`

```cmake
# Define which Pico SDK board to target
set(PICO_BOARD pico2)           # For RP2350A
# OR
set(PICO_BOARD sparkfun_promicro_rp2350)  # For RP2350B

# Platform defaults to rp2350-arm-s (ARM, Secure mode)
set(PICO_PLATFORM rp2350-arm-s)
```

### 2. Create `BoardConfig.h`

Reference an existing config like `configs/Pico2/BoardConfig.h` or `configs/FlatboxRev8/BoardConfig.h`. The pin mappings use macros like:

```cpp
#ifndef PICO_BOARD_CONFIG_H_
#define PICO_BOARD_CONFIG_H_

// Button mappings (for RP2350A, pins 0–29)
#define GPIO_PIN_1 GpioAction::BUTTON_PRESS_UP
#define GPIO_PIN_2 GpioAction::BUTTON_PRESS_DOWN
#define GPIO_PIN_3 GpioAction::BUTTON_PRESS_LEFT
#define GPIO_PIN_4 GpioAction::BUTTON_PRESS_RIGHT
// ... etc.

// For RP2350B, you can additionally map GPIO 30–47:
#define GPIO_PIN_30 GpioAction::BUTTON_PRESS_B1
#define GPIO_PIN_31 GpioAction::BUTTON_PRESS_B2
// ... etc.

#endif
```

The firmware's pin validation automatically adapts to the compiled target. Pins 30–47 are only valid when the selected SDK board target exposes that range via `NUM_BANK0_GPIOS`.

### 3. Document the Pinout

Create `README.md` with a clear pinout table:

```markdown
# My Custom RP2350 Board

## Pin Assignments

| GPIO | Function | Button |
|------|----------|--------|
| GP0 | ... | UP |
| GP1 | ... | DOWN |
| GP2 | ... | LEFT |
| GP3 | ... | RIGHT |
| ... | ... | ... |
```

### 4. Build and Test

```bash
cmake -DGP2040_BOARDCONFIG=MyCustomBoard -B build -S .
cmake --build build
```

Your custom board will be available in the release builds and CI pipeline.

---

## Migration from RP2040

Upgrading from an RP2040-based board to RP2350?

### What Changes

1. **Download the correct firmware**: Choose the UF2 file for your RP2350 board (e.g., `Pico2.uf2` for Pico 2).
2. **Flash as usual**: Copy the UF2 to your board in bootloader mode (BOOTSEL button).
3. **Reconfigure pins (if needed)**: If you upgraded to an RP2350B-based board with custom GPIO mappings, you may need to update button assignments via the web configurator.

### What Stays the Same

- **Protocols:** All input modes (XInput, PS5, Switch, etc.) work identically.
- **Configuration:** Button mappings, add-ons (turbo, SOCD, display, LEDs), and controller features work unchanged.
- **Web configurator:** Same interface; the pin count auto-adjusts to your board.
- **Add-ons:** All existing add-ons (turbo, SOCD, LED drivers, displays) work on RP2350.

### No Firmware Source Changes

Unlike some platform upgrades, RP2350 support requires **zero changes to the firmware source code**. The GP2040-CE codebase uses SDK abstractions for all hardware access, so the same firmware source compiles to working binaries for both RP2040 and RP2350.

---

## Known Limitations & Testing Notes

### PIO-Based USB Host

GP2040-CE uses the Pico SDK's PIO-based USB host library for gamepad passthrough. The PIO instruction set on RP2350 is fully backward-compatible with RP2040, and this feature is CI-tested for `Pico2` and `FlatboxRev8`. However, real-hardware testing is recommended if you depend on USB passthrough for tournament or critical use.

### Clock Frequency Difference

RP2350 runs at 150 MHz by default (vs 133 MHz for RP2040). This can affect PIO timing and peripheral clock divisors. I²C, SPI, and display add-ons should be tested on your specific board if they rely on precise timing.

### Pico 2 W and Pimoroni Pico Lipo 2 XL W

Board configurations for both the **Raspberry Pi Pico 2 W** (SDK target: `pico2_w`) and the **Pimoroni Pico Lipo 2 XL W** (current SDK target: `pico2_w`) are included. The base firmware compiles and runs on both boards.

For Pimoroni-specific board wiring, battery sense mapping (**GP43 / ADC 3**), and VBUS handling details, use [Pimoroni Pico Lipo 2 XL W Board Support](./pimoroni-pico-lipo-2xl-w-support.md) as the canonical reference.

Planned optional support for Pimoroni-provided SDK board definitions for Pico LiPo 2 XL W is documented in [Pimoroni SDK Board Definition Option for Pico LiPo 2 XL W](./pimoroni-sdk-board-definition-option.md). That proposal is implementation-gating only and does not change current build defaults.

Variant/package labeling for Pimoroni Pico Lipo 2 XL W should be treated as SDK-dependent/inferred in this repo unless a dedicated SDK board target or separate hardware/package verification is cited.

**Remaining gap:** Full Bluetooth HID support for CYW43439-equipped boards is not yet implemented. The CYW43 wireless driver integration (required for Bluetooth gamepad mode) is planned for a future release. USB HID works normally on both boards today.

### RISC-V Mode

RP2350 supports booting in RISC-V mode, but GP2040-CE targets ARM (Secure mode, `rp2350-arm-s`). RISC-V mode is not supported and not planned for gamepad use.

---

## Additional Resources

- [Raspberry Pi RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
- [Pico 2 Getting Started Guide](https://datasheets.raspberrypi.com/pico/pico-getting-started.pdf)
- [Pico SDK 2.2.0 Release Notes](https://github.com/raspberrypi/pico-sdk/releases/tag/2.2.0)
- [GP2040-CE GitHub Repository](https://github.com/FeralAI/GP2040-CE)

---

**Last Updated:** 2026-05-07
