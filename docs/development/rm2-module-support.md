# RM2 Module Support for Custom Boards

**Last updated:** 2026-03-28  
**Maintained by:** GP2040-CE core team  
**SDK version:** 2.2.0

---

> **Status: Planned — not yet implemented**

---

## Overview

The **Pimoroni RM2** is a compact, solderable breakout module containing the CYW43439 wireless chip — the same chip found on the Raspberry Pi Pico W and Pico 2 W. Where those boards integrate the CYW43 directly onto the PCB alongside the RP2040/RP2350 microcontroller, the RM2 exposes the CYW43 as a standalone module that a custom board designer can solder onto their own hardware. This makes it possible to add Bluetooth and WiFi to custom RP2040 or RP2350-based GP2040-CE boards without adopting the Pico W form factor.

For GP2040-CE, RM2 support is significant because the Bluetooth HID feature (see [Bluetooth HID Support](bluetooth-support.md)) depends on the presence of a CYW43439. Official Pico W and Pico 2 W boards have the chip built in, but the broader ecosystem of custom GP2040-CE arcade stick and gamepad boards typically uses a bare RP2040 or RP2350 chip with no wireless silicon. The RM2 module closes this gap: a custom board that routes the RM2 to the same GPIO pins used by the Pico W's onboard CYW43 can use the existing Bluetooth and WiFi driver support in the Pico SDK without any firmware-level changes.

---

## RM2 Module Hardware

The RM2 module is built around the **Broadcom CYW43439** wireless chip, which provides:

- **Bluetooth:** Classic BR/EDR and BLE (Bluetooth 5.0)
- **WiFi:** 802.11b/g/n (2.4 GHz), single-band
- **Interface to host MCU:** Proprietary half-duplex PIO-based SPI (not standard 4-wire SPI)
- **Power enable:** Dedicated REG_ON line for controlled power sequencing

The RM2 is available as a solderable module with exposed pads. Product page and pinout:  
[https://shop.pimoroni.com/products/rm2-breakout](https://shop.pimoroni.com/products/rm2-breakout)

**What the RM2 does NOT provide:**
- It does not include a USB interface, RP2040/RP2350, or any other host microcontroller. It is strictly a wireless module intended to be driven by a host MCU via its SPI pads.
- It does not include an LED connected to an RP2040 GPIO. On Pico W, the onboard LED is connected to a CYW43 internal GPIO (`WL_GPIO0`), not to an RP2040 GPIO pin. Custom RM2 boards may wire a separate LED to an RP2040 GPIO.

> **Assembler note:** The RM2 module must have its SPI interface pads physically exposed and routed to the host board. The module does not function as a plug-in header connector out of the box.

---

## Required Pin Mapping

To use the RM2 without any firmware changes, the custom board **must wire the RM2 to the same GPIO pins** that Pico W uses for its onboard CYW43. These pin assignments are defined in the Pico SDK 2.2.0 board headers (`pico_w.h` and `pico2_w.h`) and are the basis for all existing CYW43 driver code.

| GPIO | SDK Define | CYW43 Function | Notes |
|------|-----------|----------------|-------|
| 23 | `CYW43_DEFAULT_PIN_WL_REG_ON` | Power enable — drive HIGH to power up the CYW43 chip | Required before any BT/WiFi init |
| 24 | `CYW43_DEFAULT_PIN_WL_DATA_OUT` / `WL_DATA_IN` / `WL_HOST_WAKE` | Half-duplex data line; also serves as IRQ from CYW43 to RP | Single pin, tri-functional — see note below |
| 25 | `CYW43_DEFAULT_PIN_WL_CS` | SPI chip select | Active-low |
| 29 | `CYW43_DEFAULT_PIN_WL_CLOCK` | SPI clock | Shared with VSYS ADC — see note below |

### GPIO 24 — Half-Duplex, Tri-Functional

GPIO 24 is simultaneously defined as `WL_DATA_OUT`, `WL_DATA_IN`, and `WL_HOST_WAKE` in the SDK headers. This is intentional hardware design, not an error. The CYW43 uses a proprietary half-duplex PIO-based SPI protocol managed by `cyw43_bus_pio_spi.pio`. The PIO program handles direction switching and interrupt multiplexing on this single line. Do not attempt to use GPIO 24 for any other purpose on an RM2-equipped board.

### GPIO 29 — Shared with VSYS ADC

GPIO 29 is simultaneously defined as `CYW43_DEFAULT_PIN_WL_CLOCK` and `PICO_VSYS_PIN` (battery/supply voltage ADC input). The SDK handles this conflict via the `CYW43_USES_VSYS_PIN=1` define. Any code that reads the VSYS ADC must wrap the measurement in `cyw43_thread_enter()` / `cyw43_thread_exit()` to prevent the CYW43 driver from using the pin concurrently. See [Known Constraints and Open Questions](#known-constraints-and-open-questions) for a related TBD item.

### The Easy Path

If your custom board wires the RM2 to GPIO 23, 24, 25, and 29 (matching Pico W), you can set `PICO_BOARD=pico_w` (for RP2040) or `PICO_BOARD=pico2_w` (for RP2350A). No custom board header is required. The existing `configs/PicoW/BoardConfig.h` is already correct for any board with this identical wiring.

---

## GPIO Availability by Chip

On any board that wires the RM2 to Pico W-compatible pins, **GPIO 23, 24, 25, and 29 are permanently reserved** for the CYW43 interface. These pins cannot be used for buttons, axes, LEDs, I2C, UART, or any other purpose.

| Chip | Total GPIO | Reserved (CYW43) | Available for User I/O |
|------|-----------|-----------------|----------------------|
| RP2040 | 30 (GPIO 0–29) | GPIO 23, 24, 25, 29 | **26 GPIOs** (GPIO 0–22, 26–28) |
| RP2350A | 30 (GPIO 0–29) | GPIO 23, 24, 25, 29 | **26 GPIOs** (GPIO 0–22, 26–28) |
| RP2350B | 48 (GPIO 0–47) | GPIO 23, 24, 25, 29 | **44 GPIOs** (GPIO 0–22, 26–28, 30–47) |

For RP2040 and RP2350A boards, GPIO availability is identical to Pico W. For RP2350B boards, GPIO 30–47 are freely available with no CYW43 conflict (see [rp2350-support.md](rp2350-support.md) for a full GPIO reference).

### RP2350B Alternate Wiring (TBD — Advanced)

RP2350B has enough GPIO headroom to wire the RM2 to GPIO 30–33 (or another set of RP2350B-only pins), which would free GPIO 25 (usable as a standard LED pin) and GPIO 29 (usable for ADC without CYW43 coordination). However, this path:

- Requires a fully custom SDK board header with all `CYW43_DEFAULT_PIN_WL_*` overrides
- May require modifications to the `cyw43_bus_pio_spi.pio` PIO program, which is designed around specific GPIO relative offsets
- Has no community testing or validation on non-Pico-W pin layouts

**RP2350B alternate wiring is not documented here and is not planned in the current roadmap.** It remains a TBD item for future exploration.

---

## SDK and CMake Configuration

### Board Definition File

The recommended way to configure a custom RM2 board is via the SDK board header. For the simplest case (same GPIO wiring as Pico W), no custom header is needed:

**Path A: RP2040 custom board, RM2 at Pico W pins**

```cmake
# In configs/CustomRM2Board/CustomRM2Board.cmake
set(PICO_BOARD pico_w)
set(PICO_PLATFORM rp2040)
```

This reuses the official `pico_w.h` SDK board header verbatim. All CYW43 pin definitions, VSYS handling, and SMPS mode settings are inherited automatically.

**Path B: RP2350A custom board, RM2 at Pico W pins**

```cmake
# In configs/CustomRM2Board/CustomRM2Board.cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
```

Reuses the official `pico2_w.h` SDK board header. Correct for RP2350A-based boards with identical CYW43 wiring.

**Path C: Custom board header (different defaults for UART/SPI/I2C/LED, same CYW43 wiring)**

If your board needs different peripheral defaults but the same CYW43 wiring, create a custom `.h` board header:

```c
// custom_rm2_board.h
// Place in SDK boards/ directory or point to with PICO_BOARD_HEADER_DIRS

#ifndef CUSTOM_RM2_BOARD_H
#define CUSTOM_RM2_BOARD_H

// Enable CYW43 support — required for all wireless/BT SDK targets
#define PICO_CYW43_SUPPORTED 1

// CYW43 pin assignments — must match physical RM2 wiring
#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON     23u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT   24u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN    24u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE  24u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS         25u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK      29u
#endif
#ifndef CYW43_USES_VSYS_PIN
#define CYW43_USES_VSYS_PIN             1
#endif
#ifndef PICO_VSYS_PIN
#define PICO_VSYS_PIN                   29
#endif

// ... other board-specific defines (UART, SPI, I2C, LED, etc.)

#endif // CUSTOM_RM2_BOARD_H
```

### Required CMake Defines

The `PICO_CYW43_SUPPORTED=1` flag is the master switch that enables all wireless and Bluetooth SDK CMake targets. Without it, none of the CYW43 or BTStack targets are registered, regardless of how the hardware is wired. When using `PICO_BOARD=pico_w` or `PICO_BOARD=pico2_w`, this is set automatically by the SDK board header. For custom board headers, it must be set explicitly (as shown in Path C above).

`CYW43_PIN_WL_DYNAMIC` (runtime pin configuration) is `0` by default and should remain `0`. Static compile-time pin assignment is the correct and supported approach for GP2040-CE.

### CMake Link Libraries

The firmware `CMakeLists.txt` already includes a conditional block for CYW43 support. The same block works for RM2 custom boards and official Pico W / Pico 2 W boards:

```cmake
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_cyw43_arch_lwip_threadsafe_background  # WiFi
        pico_btstack_classic                         # BT Classic HID
        pico_btstack_base
        pico_btstack_cyw43
        pico_btstack_run_loop_async_context
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE GP2040_HAS_BLUETOOTH=1)
endif()
```

Note: `pico_sdk_import.cmake` in GP2040-CE contains no SDK version pins. SDK version is controlled exclusively by `CMakeLists.txt` (minimum 2.2.0, enforced with `FATAL_ERROR`).

---

## GP2040-CE Board Configuration

### Directory Structure

A custom RM2 board follows the same `configs/` directory structure as any other GP2040-CE board. Using `configs/PicoW/` as the reference:

```
configs/
└── CustomRM2Board/
    ├── BoardConfig.h             # GPIO → button/axis mappings (user-accessible pins only)
    ├── CustomRM2Board.cmake      # PICO_BOARD + PICO_PLATFORM
    ├── CMakeLists.txt            # Stub (see below)
    └── README.md                 # Board description and pinout diagram
```

### `BoardConfig.h`

Structure is identical to `configs/PicoW/BoardConfig.h`. The critical constraint: **do not define `GPIO_PIN_23`, `GPIO_PIN_24`, `GPIO_PIN_25`, or `GPIO_PIN_29`** — these are reserved for the CYW43/RM2 interface. The Pico W `BoardConfig.h` correctly omits these; a custom RM2 board must do the same.

Available user GPIO range (same as Pico W on RP2040/RP2350A): **GPIO 0–22, 26, 27, 28** (26 GPIOs total).

When a `HAS_WIRELESS` flag or equivalent is introduced in the firmware to gate Bluetooth feature availability, it will be set here. The exact define name is TBD pending implementation.

### `CMakeLists.txt` Stub

Copy verbatim from `configs/PicoW/CMakeLists.txt`:

```cmake
# add_executable(CustomRM2Board
# ${CMAKE_SOURCE_DIR}/src/main.cpp)
# link_libraries(${PROJECT_NAME})
```

This comment-stub is used by the build system for board discovery. No active build logic is needed in this file.

### Reference: `configs/PicoW/`

The existing `configs/PicoW/` directory is the canonical example for any board using the CYW43 at Pico W-compatible GPIO pins. It is the correct starting point for any RM2 custom board configuration.

---

## Relationship to Bluetooth and WiFi Features

The RM2 module is an **enabler**, not a feature in itself. It provides the CYW43439 wireless chip on custom boards that would otherwise have no wireless capability. Once the RM2 is wired and configured correctly, the board gains the same CYW43 capability as a Pico W — including all features described in [Bluetooth HID Support](bluetooth-support.md).

**Key relationships:**

- **Bluetooth HID (see [bluetooth-support.md](bluetooth-support.md)):** RM2 generalizes the BT HID feature from Pico W/Pico 2 W to the broader ecosystem of custom GP2040-CE boards. Any custom board wired with a Pico W-compatible RM2 pinout is a valid BT HID target.
- **USB HID remains primary:** GP2040-CE's USB HID output is independent of the CYW43. RM2 enables optional Bluetooth mode switching; it does not replace or affect USB HID operation.
- **WiFi coexistence:** The CYW43439 time-multiplexes WiFi and Bluetooth internally. Both can be active simultaneously on an RM2-equipped board, subject to the same considerations as Pico W.
- **No firmware source changes:** Zero changes to `src/`, `headers/`, `proto/`, or root `CMakeLists.txt` are required for an RM2 board using Pico W-compatible pins. The integration is entirely at the board configuration layer (`configs/` + CMake + SDK board header).

---

## Power Considerations

Power management for RM2-equipped boards follows the same patterns as Pico W. The key points:

- **GPIO 23 (`WL_REG_ON`)** must be driven HIGH before any CYW43 initialization. The CYW43 powers down when this pin is LOW, which is useful for battery-powered builds that need to conserve power when wireless is not active.
- **SMPS mode control** is managed through CYW43 internal GPIO `WL_GPIO1`. As with Pico W, this controls whether the onboard power supply operates in PWM (better EMI, higher power) or PFM (lower power, slightly worse EMI) mode.
- **VBUS detection** on RM2-equipped boards uses the same mechanism as Pico W: `cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN)` via CYW43 internal `WL_GPIO2`. There is no direct RP2040 GPIO for VBUS on boards using the standard CYW43 wiring.

For detailed power state machine design (active, idle, dormant modes) and CYW43 radio power management, see [Bluetooth HID Support — Power Management section](bluetooth-support.md).

---

## Known Constraints and Open Questions

The following items require confirmation or resolution before or during implementation:

| # | Item | Status |
|---|------|--------|
| 1 | **GPIO 29 VSYS sharing on custom RM2 boards:** If a custom board does NOT route GPIO 29 to a VSYS voltage divider (e.g., it has a dedicated ADC pin for battery), should `CYW43_USES_VSYS_PIN` be set to `0`? Exact voltage divider ratio for VSYS ADC when GPIO 29 is shared also needs board-designer confirmation. | TBD |
| 2 | **RM2 module VBUS sense:** On Pico W, VBUS is sensed through CYW43 internal `WL_GPIO2`, not via an RP2040 GPIO. Confirm whether the RM2 module exposes a separate VBUS sense pin or whether `cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN)` is the correct mechanism on all RM2 boards. | TBD |
| 3 | **RM2 module physical assembly:** The RM2 SPI interface pads must be exposed and routed to the host board. Confirm exact pad layout and whether castellated edges or through-holes are used for board-to-board connection. | TBD — confirm from Pimoroni datasheet/schematic |
| 4 | **RM2 antenna orientation and RF interference:** CYW43 RF performance depends on antenna placement and clearance from GPIO signal traces. Best practices for PCB layout around the RM2 module have not been documented. | TBD — board designer guidance needed |
| 5 | **`PICO_BOARD=pico_w` reuse for RM2 boards:** Confirmed viable in Path A/B above, with one caveat: the official `pico_w.h` header includes SMPS and other settings tuned for the Pico W board itself. A custom board with a different power supply topology may need a true custom header even when CYW43 wiring is identical. | TBD — verify per-board |
| 6 | **RP2350B alternate wiring (GPIO 30–33):** PIO SPI program compatibility with non-standard GPIO offsets is unverified. `cyw43_bus_pio_spi.pio` may require modification for arbitrary GPIO bases. Requires SDK-level source review and hardware testing before this path can be documented. | TBD — not planned |
| 7 | **HAS_WIRELESS firmware flag:** The exact define name (e.g., `HAS_WIRELESS`, `GP2040_HAS_BLUETOOTH`, or similar) to gate Bluetooth feature availability in `BoardConfig.h` is not yet settled. To be determined during Phase 1 BT implementation. | TBD — pending BT Phase 1 |

---

## Related Documents

- [Bluetooth HID Support](bluetooth-support.md) — Feature planning for BT HID Classic output, power management, and battery reporting. The RM2 module is the hardware enabler for this feature on custom boards.
- [RP2350 Support](rp2350-support.md) — RP2350A/B GPIO availability, board configurations, and migration guidance. See this document for RP2350A/B GPIO 30–47 details relevant to custom RM2 board design.
- [Dependency Updates](dependency-updates.md) — SDK, toolchain, and library version management, including Pico SDK 2.2.0 and Picotool 2.2.0-a4 version references.
