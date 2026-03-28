# RP2350 Support

## Overview

GP2040-CE fully supports the **Raspberry Pi RP2350** microcontroller and its variants. The RP2350 is the successor to the RP2040 and offers improvements in CPU performance, memory, and GPIO availability. GP2040-CE automatically adapts to run on both RP2350A and RP2350B chips with no firmware source code changes required—the build system and SDK handle all hardware abstraction.

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

## Supported Boards

GP2040-CE includes configuration support for the following RP2350-based boards:

| Board | Chip Variant | Config Name | GPIO Count | Notable Features |
|-------|--------------|-------------|------------|------------------|
| **Raspberry Pi Pico 2** | RP2350A | `Pico2` | 30 | Reference RP2350A board; pin layout identical to original Pico |
| **Flatbox Rev. 8** | RP2350A | `FlatboxRev8` | 30 | USB peripheral passthrough for arcade stick arcade mode |
| **SparkFun Pro Micro RP2350** | RP2350B | `SparkFunProMicroRP2350` | 48 (uses 0–29) | Compact form factor; RP2350B variant ready for expansion |

All boards are CI-tested and release-ready.

**Planned:** `Pico2W` configuration for the Raspberry Pi Pico 2 W (RP2350A + CYW43439 WiFi module).

---

## Minimum Requirements

- **Pico SDK 2.2.0 or later** — required for RP2350 support
- **CMake 3.13+** — standard requirement
- **ARM Embedded Toolchain** — e.g., `arm-none-eabi-gcc`

Verify your SDK version:
```bash
cmake --version
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

The build system automatically selects the correct Pico SDK target platform (`rp2350-arm-s`) and GPIO configuration for the chosen board.

**Note:** If Pico SDK is not installed, you can set `PICO_SDK_PATH`:
```bash
cmake -DPICO_SDK_PATH=/path/to/pico-sdk -DGP2040_BOARDCONFIG=Pico2 -B build -S .
```

---

## Pin Mapping and GPIO

Each RP2350 board configuration defines its own GPIO-to-button mappings. Refer to the board's configuration page or `README.md` for pinout diagrams and button assignments.

### GPIO Validation

GP2040-CE automatically validates pin assignments against the target chip's available GPIO count:
- **RP2350A:** Pins 0–29 are valid
- **RP2350B:** Pins 0–47 are valid

The firmware will reject any configuration that assigns buttons to unavailable pins.

### Web Configurator

The web configurator automatically detects the number of available GPIO pins on the running firmware and displays only valid pins for configuration. When you flash firmware compiled for RP2350B, the configurator will show 48 GPIO options.

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

The firmware's pin validation automatically adapts to the compiled target. Pins 30–47 will only be valid if you compile for `PICO_BOARD=sparkfun_promicro_rp2350` or another RP2350B board.

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

### Pico 2 W Config

The **Raspberry Pi Pico 2 W** (RP2350A + CYW43439 WiFi) is not yet supported in the main branch. A configuration file will be added in a future release.

### RISC-V Mode

RP2350 supports booting in RISC-V mode, but GP2040-CE targets ARM (Secure mode, `rp2350-arm-s`). RISC-V mode is not supported and not planned for gamepad use.

---

## Additional Resources

- [Raspberry Pi RP2350 Datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf)
- [Pico 2 Getting Started Guide](https://datasheets.raspberrypi.com/pico/pico-getting-started.pdf)
- [Pico SDK 2.2.0 Release Notes](https://github.com/raspberrypi/pico-sdk/releases/tag/2.2.0)
- [GP2040-CE GitHub Repository](https://github.com/FeralAI/GP2040-CE)

---

**Last Updated:** 2026-03-28
