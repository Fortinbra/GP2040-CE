# Pimoroni Pico Lipo 2 XL W Board Support

## Overview

The **Pimoroni Pico Lipo 2 XL W** is an RP2350B microcontroller board with integrated Bluetooth wireless support and onboard LiPo battery charging. This board is the **designated reference hardware for GP2040-CE Bluetooth HID development and testing**.

**Key features:**
- **RP2350B** SoC with 48 GPIO pins (pins 0–29 standard, pins 30–47 are RP2350B-exclusive)
- **CYW43439** Bluetooth + WiFi module (same chipset as Raspberry Pi Pico W / Pico 2 W)
- **USB-C** connector for power and data
- **LiPo battery charging circuit** with 3:1 voltage divider for battery voltage ADC readout on **GP43 (ADC 3)** (per Pimoroni documentation)
- **VBUS detection** via **CYW43 WL GPIO 2** for power source identification

**Product page:** [Pimoroni Pico Lipo 2 XL W](https://shop.pimoroni.com/products/pimoroni-pico-lipo-2-xl-w)

This board introduces two new requirements to GP2040-CE:
1. **Bluetooth HID support** — the CYW43 wireless chipset requires BTStack integration
2. **Power management** — battery voltage monitoring and power state awareness (USB vs. battery operation)

---

## Board Specifications

| Specification | Value |
|---|---|
| **Microcontroller** | RP2350B |
| **CPU** | Dual ARM Cortex-M33 + dual RISC-V, up to 150 MHz |
| **SRAM** | 520 KB |
| **GPIO Pins** | 48 (GPIO 0–47) |
| **USB** | USB-C (device/host mode configurable) |
| **Wireless** | CYW43439 (BT 5.3 + 802.11 a/b/g/n) |
| **Power Input** | USB-C or LiPo battery |
| **Battery Charging** | Onboard TP4056-based charge controller |
| **Voltage Divider** | 3:1 for battery ADC (GP43 / ADC 3 per Pimoroni docs) |
| **VBUS Detection** | CYW43 WL GPIO 2 |

---

## Build Configuration

### CMake Board Target

To build firmware for this board, use:

```bash
PICO_BOARD=pico2_w SKIP_WEBBUILD=TRUE GP2040_BOARDCONFIG=PimoroniPicoLipo2XLW cmake -G Ninja -B build -S .
cmake --build build
```

**Key points:**
- `PICO_BOARD=pico2_w` enables both RP2350B support AND CYW43 wireless driver compilation in the Pico SDK
- The **default** `PICO_BOARD=pico` does NOT include CYW43 drivers; do not use for this board
- This board target is named `pico2_w` in the SDK (not `pico_lipo_2xl_w`) because it follows the Raspberry Pi naming convention for RP2350 + CYW43 combinations
- Optional use of Pimoroni-provided SDK board definitions is tracked as a separate feature proposal and is not implemented yet: see [Pimoroni SDK Board Definition Option for Pico LiPo 2 XL W](./pimoroni-sdk-board-definition-option.md)

### Pico SDK Version Requirement

**Minimum SDK version: 2.2.0** (enforced in `CMakeLists.txt`)

The GP2040-CE build system will fail with a fatal error if an older SDK is detected. Verify your SDK version:

```bash
cat $PICO_SDK_PATH/pico_sdk_version.cmake
# Should show: set(PICO_SDK_VERSION_STRING "2.2.0")
```

---

## GPIO Pin Mapping — Board-Specific Signals

The Pimoroni Pico Lipo 2 XL W reuses the standard CYW43 control pins from `pico2_w`, but its battery and VBUS sensing are **not** routed the same way as a Pico W / Pico 2 W.

### GPIO24 — Wireless Data Only

| Function | Role | Notes |
|---|---|---|
| **WL_DATA** (CYW43) | Half-duplex SPI data line | Required for wireless operation |

**Development constraint:** GPIO24 must be configured as a wireless pin by the CYW43 driver. It is **not** the board's VBUS-sense signal on this hardware.

### GPIO29 — Wireless Clock Only

| Function | Role | Notes |
|---|---|---|
| **WL_CLK** (CYW43) | Wireless SPI clock | Required for wireless operation |

**Development constraint:** GPIO29 is reserved for CYW43 clocking on this board. It should not be documented or configured as the battery-sense input for the Pico LiPo 2 XL W.

### GP43 — Battery Sense

| Function | Role | Notes |
|---|---|---|
| **Battery ADC** | Battery voltage measurement | Dedicated RP2350B-only pin connected to the LiPo divider in Pimoroni's board documentation |

**Development constraint:** Treat GP43 as reserved for battery monitoring. The firmware should not reuse it for buttons or add-ons.

### CYW43 WL GPIO 2 — VBUS Detection

| Function | Role | Notes |
|---|---|---|
| **CYW43_WL_GPIO_VBUS_PIN** | USB power detection | Read through `cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN)`; not exposed as an RP2350 GPIO |

### Complete GPIO Allocation

| GPIO | Function | Status |
|---|---|---|
| GPIO 0–22 | User-available | Can be mapped to buttons, LEDs, etc. |
| **GPIO 23** | **WL_REG_ON** (CYW43 power enable) | Reserved for wireless |
| **GPIO 24** | **WL_DATA** | Reserved for wireless |
| **GPIO 25** | **WL_CS** (CYW43 chip select) | Reserved for wireless |
| **GPIO 26–28** | User-available | Can be mapped to buttons, LEDs, etc. |
| **GPIO 29** | **WL_CLK** | Reserved for wireless |
| **GPIO 30–42, 44–47** | User-available (RP2350B only) | Can be mapped to buttons, LEDs, etc. |
| **GPIO 43** | **Battery sense** | Reserved for the onboard LiPo divider |

> **Important:** GPIO 23, 24, 25, and 29 are controlled by the CYW43 driver. GP43 is reserved for battery sensing. User code must not repurpose these signals.

---

## Battery Management

### Battery Voltage Monitoring

The board includes a 3:1 voltage divider on the LiPo battery connector. Pimoroni's board documentation routes that divided voltage to **GP43**.

Current GP2040-CE BLE battery reporting expects board config battery macros and uses ADC channel selection from board config.

Implementation note: this document describes the intended Pimoroni-docs mapping (**GP43 / ADC 3**), but the current repository board config still maps battery ADC to **GPIO29 / ADC 3**. Treat this as a required reconciliation item before implementation is considered complete.

Target board config values after reconciliation:

- `BATTERY_ADC_GPIO` = `43`
- `BATTERY_ADC_CHANNEL` = `3` (GP43 maps to ADC 3)


Current BLE conversion implementation in `src/BLEHIDManager.cpp` uses fixed raw thresholds:

```c
adc_select_input(BATTERY_ADC_CHANNEL);
uint16_t raw = adc_read();
if (raw <= 1241) return 0;
if (raw >= 1737) return 100;
return (uint8_t)((raw - 1241) * 100 / 496);
```

If `BATTERY_ADC_GPIO` is not defined for a board, BLE battery reporting falls back to `100%`.

**Reference conversion model (used to derive the threshold constants):**

```c
// Read battery sense (connected to GP43 / battery divider)
uint16_t adc_raw = adc_read();  // Returns 12-bit value (0–4095)

// Convert to mV: ADC @ 3.3V reference, 12-bit resolution
float adc_voltage_mv = (adc_raw / 4095.0) * 3300.0;

// Undo the 3:1 voltage divider
float battery_voltage_mv = adc_voltage_mv * 3.0;

// Convert to percentage (LiPo discharge curve)
// 3.0V = 0%, 4.2V = 100% (standard LiPo curve)
float battery_percent = ((battery_voltage_mv - 3000.0) / (4200.0 - 3000.0)) * 100.0;
battery_percent = fmax(0.0, fmin(100.0, battery_percent));  // Clamp to 0–100%
```

**Expected voltage range:**
- Fully discharged: ~3.0 V (0%)
- Mid-discharge: ~3.7 V (50%)
- Fully charged: ~4.2 V (100%)

### USB Power Detection (VBUS)

The board's VBUS sense path is exposed through **CYW43 WL GPIO 2**, not RP2350 GPIO24. Firmware should query it through the CYW43 API after wireless initialization:

```c
bool usb_present = cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN);
```

**Recommended approach:**
1. Initialize the CYW43 stack first.
2. Read `cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN)` for raw USB-present state.
3. Optionally combine that signal with TinyUSB connection state if the application also needs host-enumeration awareness.

**Power state awareness:**
- **USB powered:** Normal operation, no power constraints
- **Battery powered:** Power management required — reduce CPU clock, disable unused peripherals, enter dormant mode during idle periods

### Battery Percentage Reporting via Bluetooth

When Bluetooth HID is active, firmware reports battery level to the connected host using the **BLE GATT Battery Service** (UUID `0x180F`).

Current status caveats:

- BLE status endpoints/UI currently show connectivity/bond state but do not expose battery percentage.
- USB charging/VBUS override behavior is not currently wired into the BLE battery conversion path.

---

## Wireless Support

### CYW43439 Connectivity

The Pimoroni Pico Lipo 2 XL W uses the same **CYW43439** wireless module as the Raspberry Pi Pico W and Pico 2 W.

**GPIO pinout for CYW43:**

| GPIO | Signal | Direction | Purpose |
|---|---|---|---|
| GPIO 23 | WL_REG_ON | Out | Wireless module power enable |
| GPIO 24 | WL_DATA | Bidir | Half-duplex SPI data |
| GPIO 25 | WL_CS | Out | Chip select |
| GPIO 29 | WL_CLK | Out | SPI clock |

All CYW43 pins are automatically configured by the Pico SDK when `PICO_BOARD=pico2_w` is set. User code does not manually configure these pins.

### BTStack Integration

**Confirmed:** RP2350B + CYW43439 is fully compatible with BTStack Bluetooth HID implementation. The Pimoroni Pico Lipo 2 XL W is the designated reference hardware for this integration.

**Build flags for Bluetooth:**
```bash
# When Bluetooth support is added to GP2040-CE, the build command will be:
PICO_BOARD=pico2_w SKIP_WEBBUILD=TRUE GP2040_BOARDCONFIG=PimoroniPicoLipo2XLW cmake -G Ninja -B build -S .
# (BTStack integration remains conditional in firmware code)
```

### TinyUSB ↔ BTStack Namespace Conflict

**Known issue:** TinyUSB and BTStack both define `hid_report_type_t`. This creates a compile error if both libraries are included in the same translation unit.

**Mitigation:** Use translation unit isolation:
- Do NOT include both `tusb.h` and BTStack HID headers in the same `.cpp` file
- Create separate compilation units for USB HID handling and Bluetooth HID handling
- Use an internal header or enum wrapper to avoid direct namespace collision

Example:
```cpp
// usb_hid.cpp
#include "tusb.h"
// USB HID code only

// bt_hid.cpp
#include "btstack_hid.h"
// Bluetooth HID code only

// hid_common.h
enum class HIDReportType { ... };  // Wrapper, avoids tusb.h + btstack conflict
```

---

## Development Notes

### This Board is the Bluetooth Reference Platform

The Pimoroni Pico Lipo 2 XL W is the **primary development and testing board for all Bluetooth HID features**. New Bluetooth functionality should be tested on this hardware first before claiming RP2350 + CYW43 compatibility on other boards.

### Extra GPIO Capacity

The RP2350B's extra GPIO pins remain a major advantage on this board. With **GP43 reserved for battery sensing**, the remaining RP2350B-only pins (**GPIO 30–42 and 44–47**) can be used for:
- Additional button inputs beyond the standard Pico pin set
- LED status indicators
- Other peripheral control

See [RP2350 Support](./rp2350-support.md) for custom board configuration details.

### GP43 Battery Sense Note

For the Pimoroni Pico LiPo 2 XL W specifically, battery sense is **GP43**, and **GP43 maps to ADC 3**.

### Related Documentation

- **[Bluetooth HID Support](./bluetooth-support.md)** — Feature documentation for Bluetooth HID implementation
- **[RP2350 Support](./rp2350-support.md)** — Hardware specifications and GPIO validation for all RP2350 boards
- **[Pimoroni SDK Board Definition Option for Pico LiPo 2 XL W](./pimoroni-sdk-board-definition-option.md)** — Implementation-gating feature document for optional Pimoroni board-definition build path

---

## Success Criteria — "Officially Supported"

The Pimoroni Pico Lipo 2 XL W is considered **fully supported** in GP2040-CE when:

1. **Firmware compiles cleanly** with `PICO_BOARD=pico2_w`
2. **USB HID works** — all button inputs and outputs function correctly over USB (device/host mode as configured)
3. **CYW43 wireless stack initializes** — BTStack successfully configures the CYW43 module and radio powers on
4. **Battery voltage reports correctly** — ADC reads GP43 (ADC 3) and converts voltage as specified; reported value matches actual battery voltage
5. **Bluetooth HID is functional** — gamepad can pair with a PC/Mac/phone and send button inputs + battery level over BT
6. **Power management works** — firmware gracefully handles USB disconnect, switches to battery power, and respects low-battery thresholds

---

## Maintainer

**GP2040-CE core team**  
Updated: 2026-03-28
