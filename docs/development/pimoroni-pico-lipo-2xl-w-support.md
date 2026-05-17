# Pimoroni Pico Lipo 2 XL W Board Support

## Overview

The **Pimoroni Pico Lipo 2 XL W** is an RP2350B microcontroller board with integrated Bluetooth wireless support and onboard LiPo battery charging. This board is the **designated reference hardware for GP2040-CE Bluetooth HID development and testing**.

**Key features:**
- **RP2350B** SoC with 48 GPIO pins (pins 0–29 standard, pins 30–47 are RP2350B-exclusive)
- **CYW43439** Bluetooth + WiFi module (same chipset as Raspberry Pi Pico W / Pico 2 W)
- **USB-C** connector for power and data
- **LiPo battery charging circuit** with 3:1 voltage divider for battery voltage ADC readout
- **VBUS detection** via dedicated GPIO for power source identification

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
| **Voltage Divider** | 3:1 for battery ADC (GPIO29/ADC3) |
| **VBUS Detection** | GPIO24 (low when USB absent) |

---

## Build Configuration

### CMake Board Target

To build firmware for this board, use:

```bash
PICO_BOARD=pico2_w cmake -B build -S .
cd build
cmake --build .
```

**Key points:**
- `PICO_BOARD=pico2_w` enables both RP2350B support AND CYW43 wireless driver compilation in the Pico SDK
- The **default** `PICO_BOARD=pico` does NOT include CYW43 drivers; do not use for this board
- This board target is named `pico2_w` in the SDK (not `pico_lipo_2xl_w`) because it follows the Raspberry Pi naming convention for RP2350 + CYW43 combinations

### Pico SDK Version Requirement

**Minimum SDK version: 2.2.0** (enforced in `CMakeLists.txt`)

The GP2040-CE build system will fail with a fatal error if an older SDK is detected. Verify your SDK version:

```bash
cat $PICO_SDK_PATH/pico_sdk_version.cmake
# Should show: set(PICO_SDK_VERSION_STRING "2.2.0")
```

---

## GPIO Pin Mapping — Shared-Function Pins

The Pimoroni Pico Lipo 2 XL W has two GPIO pins with **dual function assignments**. Firmware code must manage these functions carefully to avoid conflicts.

### GPIO24 — VBUS Detection + Wireless Data

| Function | Role | Notes |
|---|---|---|
| **WL_DATA** (CYW43) | Half-duplex SPI data line | Required for wireless operation |
| **VBUS Detect** | USB power detection | LOW = no USB; HIGH = USB connected |

**Development constraint:** GPIO24 must be configured as a wireless pin by the CYW43 driver. The firmware cannot directly read it as a GPIO input without breaking wireless. VBUS detection must use a **separate software monitoring approach** (see Battery Management section).

### GPIO29 — Battery ADC + Wireless Clock

| Function | Role | Notes |
|---|---|---|
| **WL_CLK** (CYW43) | Wireless SPI clock | Required for wireless operation |
| **ADC3** | Battery voltage measurement | Reads divider (actual voltage = ADC reading × 3) |

**Development constraint:** GPIO29 is dual-assigned. The Pico SDK CYW43 driver configures it for wireless clocking. Battery voltage reads are performed via the ADC peripheral, not GPIO, so there is no functional conflict—both can coexist. However, code that attempts to control GPIO29 as a standard GPIO pin will fail.

### Complete GPIO Allocation

| GPIO | Function | Status |
|---|---|---|
| GPIO 0–22 | User-available | Can be mapped to buttons, LEDs, etc. |
| **GPIO 23** | **WL_REG_ON** (CYW43 power enable) | Reserved for wireless |
| **GPIO 24** | **WL_DATA + VBUS detect** | Reserved for wireless; VBUS must be read via firmware state logic |
| **GPIO 25** | **WL_CS** (CYW43 chip select) | Reserved for wireless |
| **GPIO 26–28** | User-available | Can be mapped to buttons, LEDs, etc. |
| **GPIO 29** | **WL_CLK + ADC3** (battery) | Reserved for wireless; ADC3 can be read in parallel |
| **GPIO 30–47** | User-available (RP2350B only) | Can be mapped to buttons, LEDs, etc. |

> **Important:** GPIO 23, 24, 25, and 29 are controlled by the CYW43 driver. User code must not attempt to reconfigure these pins.

---

## Battery Management

### Battery Voltage Monitoring

The board includes a 3:1 voltage divider on the LiPo battery connector. The divided voltage is routed to **GPIO29 / ADC3**.

Current GP2040-CE BLE battery reporting expects board config battery macros and uses ADC channel selection from board config.

Required board config macros:

```c
#define BATTERY_ADC_GPIO         29
#define BATTERY_ADC_CHANNEL      3
```

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
// Read ADC3 (connected to GPIO29 / battery divider)
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

The board uses **GPIO24 for VBUS detection**, but GPIO24 is also the CYW43 data line and cannot be read as a standard GPIO input. Instead, **firmware must infer USB power from application state**:

**Recommended approach:**
1. When the board first boots, check if USB is enumerated (via TinyUSB device state)
2. Monitor the USB stack state during operation — if the host enumerates the device, USB is present
3. If USB is disconnected during operation, the TinyUSB stack will report this

**Fallback (if USB state polling is insufficient):**
- Use a separate GPIO pin (from the available pool: 0–22, 26–28, 30–47) and add external circuitry to monitor VBUS
- This is a board-level design decision; the default Pimoroni board does NOT route GPIO24 as a user-accessible VBUS input

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
| GPIO 24 | WL_DATA | Bidir | Half-duplex SPI data (shared with VBUS sensing) |
| GPIO 25 | WL_CS | Out | Chip select |
| GPIO 29 | WL_CLK | Out | SPI clock (shared with ADC3 battery input) |

All CYW43 pins are automatically configured by the Pico SDK when `PICO_BOARD=pico2_w` is set. User code does not manually configure these pins.

### BTStack Integration

**Confirmed:** RP2350B + CYW43439 is fully compatible with BTStack Bluetooth HID implementation. The Pimoroni Pico Lipo 2 XL W is the designated reference hardware for this integration.

**Build flags for Bluetooth:**
```bash
# When Bluetooth support is added to GP2040-CE, the build command will be:
PICO_BOARD=pico2_w cmake -B build -S .
# (No additional flags needed; BTStack integration will be conditional in firmware code)
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

The RP2350B's 18 additional GPIO pins (30–47) are available for expansion on this board. Current GP2040-CE configurations use GPIO 0–29; boards designed in the future can leverage GPIO 30–47 for:
- Additional button inputs (up to 48 total GPIO inputs)
- LED status indicators
- Other peripheral control

See [RP2350 Support](./rp2350-support.md) for custom board configuration details.

### GP43 Battery Sense Planning Note

For custom RP2350B boards, **GP43** is a potential future battery-sense routing target.

Before defining battery macros against GP43, validate hardware and SDK mapping end-to-end:

1. Confirm board schematic routes battery divider output to GP43.
2. Confirm Pico SDK ADC input index for GP43 on the selected target/SDK version.
3. Set `BATTERY_ADC_GPIO` and `BATTERY_ADC_CHANNEL` to that verified mapping only after confirmation.

Open validation item: GP43 ADC channel mapping must be verified on hardware/SDK before documenting a fixed `BATTERY_ADC_CHANNEL` value.

### Related Documentation

- **[Bluetooth HID Support](./bluetooth-support.md)** — Feature documentation for Bluetooth HID implementation
- **[RP2350 Support](./rp2350-support.md)** — Hardware specifications and GPIO validation for all RP2350 boards

---

## Success Criteria — "Officially Supported"

The Pimoroni Pico Lipo 2 XL W is considered **fully supported** in GP2040-CE when:

1. **Firmware compiles cleanly** with `PICO_BOARD=pico2_w`
2. **USB HID works** — all button inputs and outputs function correctly over USB (device/host mode as configured)
3. **CYW43 wireless stack initializes** — BTStack successfully configures the CYW43 module and radio powers on
4. **Battery voltage reports correctly** — ADC reads GPIO29 and converts voltage as specified; reported value matches actual battery voltage
5. **Bluetooth HID is functional** — gamepad can pair with a PC/Mac/phone and send button inputs + battery level over BT
6. **Power management works** — firmware gracefully handles USB disconnect, switches to battery power, and respects low-battery thresholds

---

## Maintainer

**GP2040-CE core team**  
Updated: 2026-03-28
