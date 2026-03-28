# RM2 Module Support Analysis

**Author:** Edward (Firmware Developer)  
**Date:** 2026-03-28  
**Status:** Research complete — grounded in SDK 2.2.0 board headers and cyw43_driver CMakeLists  
**Context:** Technical foundation for Hughes's RM2 module feature documentation  

---

## 1. Exact GPIO Pin Assignments — Pico W CYW43 Interface

Sourced directly from SDK 2.2.0 board headers:
- `pico_w.h` → `C:\Users\<user>\.pico-sdk\sdk\2.2.0\src\boards\include\boards\pico_w.h`
- `pico2_w.h` → same path, `pico2_w.h`

**Both `pico_w.h` and `pico2_w.h` define IDENTICAL CYW43 pin assignments:**

| SDK Define | GPIO | CYW43 Function |
|---|---|---|
| `CYW43_DEFAULT_PIN_WL_REG_ON` | **23** | Power enable — drives HIGH to power up CYW43 chip |
| `CYW43_DEFAULT_PIN_WL_DATA_OUT` | **24** | SPI data from RP to CYW43 |
| `CYW43_DEFAULT_PIN_WL_DATA_IN` | **24** | SPI data from CYW43 to RP (same pin — half-duplex) |
| `CYW43_DEFAULT_PIN_WL_HOST_WAKE` | **24** | IRQ/interrupt from CYW43 to RP (same pin!) |
| `CYW43_DEFAULT_PIN_WL_CS` | **25** | SPI chip select |
| `CYW43_DEFAULT_PIN_WL_CLOCK` | **29** | SPI clock |
| `PICO_VSYS_PIN` | **29** | VSYS ADC input (shared with clock — see below) |

### Critical nuance: GPIO 24 is tri-functional

`DATA_OUT`, `DATA_IN`, and `HOST_WAKE` all resolve to **GPIO 24**. This is intentional — the CYW43 uses a proprietary half-duplex PIO-based SPI protocol (not standard 4-wire SPI). The PIO program in `cyw43_bus_pio_spi.pio` manages the direction switching and interrupt multiplexing on this single line. **This is not a typo in the SDK headers** — it is the actual hardware design.

### Critical nuance: GPIO 29 shared with VSYS ADC

`CYW43_DEFAULT_PIN_WL_CLOCK = 29` and `PICO_VSYS_PIN = 29` are the same GPIO. The SDK resolves this conflict with `CYW43_USES_VSYS_PIN=1` — callers must wrap VSYS ADC reads in `cyw43_thread_enter()` / `cyw43_thread_exit()` to prevent CYW43 using the pin simultaneously.

### Internal CYW43 GPIOs (not RP2040 GPIOs)

These are GPIOs on the CYW43 chip itself, not RP2040 bank0 GPIOs:

| Define | CYW43 GPIO | Function |
|---|---|---|
| `CYW43_WL_GPIO_LED_PIN` | WL_GPIO0 | Onboard LED (not on any RP2040 GPIO) |
| `CYW43_WL_GPIO_SMPS_PIN` | WL_GPIO1 | SMPS mode control (PWM vs PFM power supply) |
| `CYW43_WL_GPIO_VBUS_PIN` | WL_GPIO2 | VBUS sense — read via `cyw43_arch_gpio_get()` |

**Consequence:** On Pico W / Pico 2 W, there is NO `PICO_DEFAULT_LED_PIN` and NO `PICO_VBUS_PIN` RP2040 GPIO. Battery-powered applications must read VBUS through the CYW43 `WL_GPIO2`, not via a direct RP2040 ADC. (Contrast: bare Pico uses GPIO 25 for LED and has a VBUS pin.)

### Summary: 4 RP2040 GPIOs consumed by CYW43

**GPIO 23, 24, 25, 29** are permanently reserved on any board that wires CYW43 to the Pico W pinout. These cannot be used for buttons, LEDs, I2C, UART, or any other purpose.

---

## 2. SDK CMake and Compile Defines for a Custom Board Using CYW43/RM2

### The master switch: `PICO_CYW43_SUPPORTED`

Set in the board header via `pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)`. This single flag:
- Enables `pico_cyw43_driver` CMake target
- Enables `pico_cyw43_arch_*` CMake targets (lwip, poll, freertos variants)
- Enables `pico_btstack_*` CMake targets (classic, ble, etc.)
- Triggers the "Pico W Bluetooth build support available" message in the SDK build

Without this flag, none of the wireless or Bluetooth SDK cmake targets are registered, regardless of how the hardware is wired.

### Pin defines: board header approach (canonical)

The `CYW43_DEFAULT_PIN_WL_*` defines use `#ifndef` guards in the board header, meaning any definition before inclusion overrides them. The canonical way to set them for a custom board is in the board's `.h` file:

```c
// In custom_rm2_board.h
pico_board_cmake_set(PICO_CYW43_SUPPORTED, 1)

#ifndef CYW43_DEFAULT_PIN_WL_REG_ON
#define CYW43_DEFAULT_PIN_WL_REG_ON 23u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_DATA_OUT
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 24u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_DATA_IN
#define CYW43_DEFAULT_PIN_WL_DATA_IN 24u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_HOST_WAKE
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 24u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_CLOCK
#define CYW43_DEFAULT_PIN_WL_CLOCK 29u
#endif
#ifndef CYW43_DEFAULT_PIN_WL_CS
#define CYW43_DEFAULT_PIN_WL_CS 25u
#endif
#ifndef CYW43_USES_VSYS_PIN
#define CYW43_USES_VSYS_PIN 1
#endif
#ifndef PICO_VSYS_PIN
#define PICO_VSYS_PIN 29
#endif
```

> **Note:** The SDK's `pico_cyw43_driver/CMakeLists.txt` has commented-out blocks for setting these via `target_compile_definitions()`. The comments explicitly state: "I don't think there is a major use case for these to be settable from CMake command line vs board header or `target_compile_definitions`". The board header approach is the intended and supported mechanism.

### Alternative: `CYW43_PIN_WL_DYNAMIC=1`

Setting `CYW43_PIN_WL_DYNAMIC=1` enables runtime pin configuration — but this is `0` by default in all current boards and not needed for the RM2 scenario. Static compile-time pin assignment is the correct approach.

### CMake targets required in the firmware `CMakeLists.txt`

```cmake
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_cyw43_arch_lwip_threadsafe_background  # WiFi (if used)
        pico_btstack_classic                         # BT Classic HID (if used)
        pico_btstack_base
        pico_btstack_cyw43
        pico_btstack_run_loop_async_context
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE GP2040_HAS_BLUETOOTH=1)
endif()
```

This block already exists in the project's BT feature plan; it works identically for RM2 custom boards and official Pico W / Pico 2 W boards, as long as `PICO_CYW43_SUPPORTED=1` is set.

---

## 3. What a `configs/CustomRM2Board/` Directory Needs

### Directory structure

```
configs/
└── CustomRM2Board/
    ├── BoardConfig.h         # GPIO→button/axis mappings for user-accessible pins
    ├── CustomRM2Board.cmake  # PICO_BOARD + PICO_PLATFORM
    ├── CMakeLists.txt        # Stub (matches PicoW/CMakeLists.txt pattern)
    └── README.md             # Board description, pinout diagram
```

### `CustomRM2Board.cmake`

**Path A: Same pins as Pico W on RP2040**
```cmake
set(PICO_BOARD pico_w)
set(PICO_PLATFORM rp2040)
```
This reuses the official `pico_w.h` board header verbatim. No custom board header needed. The SDK will configure everything correctly.

**Path B: Same pins as Pico W on RP2350A**
```cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
```
Same principle — reuses `pico2_w.h`. Correct for Pimoroni Pico 2 W-compatible custom boards.

**Path C: Custom board header with identical pin wiring**
```cmake
set(PICO_BOARD custom_rm2_board)
set(PICO_PLATFORM rp2040)  # or rp2350-arm-s
```
Requires a custom `.h` board header in the SDK's `boards/` directory (or pointed to via `PICO_BOARD_HEADER_DIRS`). This is the path for boards that need different defaults for UART/SPI/I2C/LED while keeping the same CYW43 wiring.

### `BoardConfig.h`

Identical structure to `configs/PicoW/BoardConfig.h`. The key constraint: **do not define GPIO_PIN_23, GPIO_PIN_24, GPIO_PIN_25, or GPIO_PIN_29** — these are reserved for CYW43. The Pico W `BoardConfig.h` correctly omits these; a custom RM2 board must do the same.

Available user GPIO range (same as Pico W): GPIO 0–22, 26, 27, 28 (27 GPIOs total for user use).

### `CMakeLists.txt` (stub)

```cmake
# add_executable(CustomRM2Board
# ${CMAKE_SOURCE_DIR}/src/main.cpp)
# link_libraries(${PROJECT_NAME})
```
Copy verbatim from `configs/PicoW/CMakeLists.txt` — this is a comment-stub that the build system uses for discovery.

---

## 4. RP2040 vs RP2350A vs RP2350B Implications

### GPIO count

| Chip | Bank0 GPIOs | `NUM_BANK0_GPIOS` |
|---|---|---|
| RP2040 | GPIO 0–29 (30 total) | 30 |
| RP2350A | GPIO 0–29 (30 total) | 30 |
| RP2350B | GPIO 0–47 (48 total) | 48 |

### Impact of CYW43 pin reservation

On **RP2040 / RP2350A**: GPIOs 23, 24, 25, 29 consumed by CYW43 → **26 user GPIOs** (0–22, 26, 27, 28).

On **RP2350B** with RM2 at Pico W pins: Same 4 pins consumed → **44 user GPIOs** (0–22, 26–28, 30–47). The additional pins 30–47 are available without any CYW43 conflict.

### RP2350B alternate wiring possibility

RP2350B has enough GPIO headroom to wire the RM2 to pins that do NOT overlap with the standard Pico W set. For example, wiring to GPIO 30–33 (or any 4 RP2350B-only GPIOs):

```c
// Hypothetical RP2350B custom board wiring RM2 to non-standard GPIOs
#define CYW43_DEFAULT_PIN_WL_REG_ON   30u
#define CYW43_DEFAULT_PIN_WL_DATA_OUT 31u
#define CYW43_DEFAULT_PIN_WL_DATA_IN  31u
#define CYW43_DEFAULT_PIN_WL_HOST_WAKE 31u
#define CYW43_DEFAULT_PIN_WL_CLOCK    32u  // ← GPIO 29 freed for ADC/other use
#define CYW43_DEFAULT_PIN_WL_CS       33u  // ← GPIO 25 freed for LED/button
```

This frees GPIO 25 (usable as an LED pin, matching bare Pico convention) and GPIO 29 (usable for ADC without CYW43 thread coordination). However, this requires a full custom board header and a PIO SPI compatibility check (see Section 5).

### PIO constraint note for alternate wiring

The CYW43 PIO SPI program (`cyw43_bus_pio_spi.pio`) uses consecutive GPIO numbering for efficiency. The driver requires that `DATA_OUT`, `DATA_IN`/`HOST_WAKE`, and `CLOCK` be a specific relative arrangement compatible with the PIO `in_pins` / `out_pins` configuration. **TBD: Verify whether arbitrary alternate GPIO assignments on RP2350B are PIO-compatible or require a modified PIO program.** This is the key open question for alternate pinouts.

---

## 5. The "Same Pins as Pico W" Constraint

### Why it is the easy path

1. **No firmware changes.** The cyw43_driver PIO SPI program, the btstack CYW43 transport, and all CYW43 power management code are verified against the Pico W GPIO layout. Using the same physical layout means the existing driver works out of the box.

2. **`PICO_BOARD=pico_w` or `PICO_BOARD=pico2_w` works directly.** A custom board that reuses these official board headers gets all CYW43 pin definitions, VSYS handling, and SMPS mode settings for free. No custom board header required in the simplest case.

3. **GP2040-CE `configs/PicoW/` works as-is.** The existing `BoardConfig.h` (which skips GPIO 23–25, 29) is already correct for any board wired identically to Pico W.

4. **Hardware testing validates automatically.** Any testing done on Pico W hardware also validates the RM2 wiring on a custom board with the same pinout.

### What alternate pinouts would require

1. **Custom SDK board header** defining `PICO_CYW43_SUPPORTED=1` and all `CYW43_DEFAULT_PIN_WL_*` overrides.

2. **PIO program compatibility verification.** The `cyw43_bus_pio_spi.pio` PIO program is hard-coded for specific `in_pins`/`out_pins` bases. Relocating CYW43 to non-consecutive or unexpected GPIO positions may require modifying the PIO program or the `cyw43_driver.c` pin setup code. This is a non-trivial SDK-level change.

3. **New `configs/` entry.** A distinct board config directory cannot share the PicoW `BoardConfig.h` — it would need its own with the alternate available GPIOs documented.

4. **Re-validation of all BT and WiFi functionality.** No community testing exists for non-standard CYW43 pin placements on RP2350B.

### Summary rule for documentation

> **If your custom board wires the RM2 to GPIO 23, 24, 25, and 29 (matching Pico W), use `PICO_BOARD=pico_w` (RP2040) or `PICO_BOARD=pico2_w` (RP2350A). No firmware changes are needed. If you wire to different GPIOs, custom SDK board configuration and PIO driver verification are required — this is unsupported by the current SDK for non-Pico-W boards and may require SDK-level modifications.**

---

## 6. Relationship to BT Docs and Cross-References

### `bluetooth-support.md` dependency

The BT feature documentation (`docs/development/bluetooth-support.md`) currently assumes Pico W or Pico 2 W as the hardware target. RM2 support generalizes this: any custom RP2040 or RP2350 board wired like a Pico W can use the same BT feature.

**Required cross-reference addition in `bluetooth-support.md`:**
- A note in the "Supported Hardware" section: "Third-party RM2 module (CYW43439 breakout) wired to Pico W-compatible GPIO 23/24/25/29 is supported — see RM2 module documentation for board configuration details."

### `rp2350-support.md` relationship

The RM2 module is the enabling hardware for BT + RP2350B use cases. `rp2350-support.md` should gain a note about RM2 as the CYW43 provider for custom RP2350B boards that want wireless.

### No firmware source changes required for RM2 (Pico W pinout)

Zero changes to `src/`, `headers/`, `proto/`, or `CMakeLists.txt` are needed for a custom board using RM2 at Pico W-compatible pins. The entire integration is at the board configuration layer (`configs/` + cmake + SDK board header).

---

## 7. Open Questions / TBD Items for Hughes to Flag

1. **PIO program GPIO constraint for alternate pinouts (RP2350B):** Does `cyw43_bus_pio_spi.pio` support arbitrary GPIO bases, or must `DATA_OUT`/`DATA_IN`/`HOST_WAKE`/`CLOCK`/`CS` be at specific relative offsets? The SDK comments say "cyw43 SPI pins can't be changed at runtime" but don't document compile-time constraints beyond the `#ifndef` guards. Needs either SDK source review of the PIO program or testing on RP2350B with relocated pins.

2. **CYW43 VSYS/GPIO 29 sharing on custom RM2 boards:** If a custom board does NOT use GPIO 29 for VSYS (e.g., it has a dedicated ADC pin for battery), should `CYW43_USES_VSYS_PIN` be set to 0? What is the battery ADC story for a custom RM2 board vs Pimoroni Pico Lipo 2 XL W?

3. **RM2 module VBUS sense:** On Pico W, VBUS is sensed through CYW43 `WL_GPIO2`, not via an RP2040 GPIO. Does the RM2 module expose a separate VBUS sense pin, or does the same `cyw43_arch_gpio_get(CYW43_WL_GPIO_VBUS_PIN)` pattern apply? This matters for battery vs USB power detection in GP2040-CE.

4. **RM2 module LED:** On Pico W, the LED is on `CYW43_WL_GPIO_LED_PIN = WL_GPIO0`. Custom boards may have an LED on an RP2040 GPIO instead. `BoardConfig.h` LED_PIN handling may differ between RM2 boards and Pico W.

5. **RP2350A vs RP2350B for RM2 use case:** Is there a compelling reason to wire RM2 to RP2350B-only GPIOs (30–47)? The main motivation would be to free up GPIO 25 (Pico standard LED) and GPIO 29 (ADC without CYW43 coordination). Whether this is worth the increased complexity is a design question for board creators.

6. **Pimoroni RM2 datasheet confirmation:** The analysis above is derived from Pico W/2W SDK headers (which use CYW43439). The RM2 product page should be confirmed to expose the same 4-wire interface (REG_ON, DATA, CLK, CS) and uses the same CYW43439 firmware blob. The Pimoroni RM2 product URL is `https://shop.pimoroni.com/products/rm2-breakout` — Hughes should pull the schematic/pinout from there for the doc.

---

## 8. Files Referenced

| File | Role |
|---|---|
| `C:\Users\<user>\.pico-sdk\sdk\2.2.0\src\boards\include\boards\pico_w.h` | Pico W board header — authoritative CYW43 pin defines |
| `C:\Users\<user>\.pico-sdk\sdk\2.2.0\src\boards\include\boards\pico2_w.h` | Pico 2 W board header — identical CYW43 pin defines for RP2350 |
| `C:\Users\<user>\.pico-sdk\sdk\2.2.0\src\rp2_common\pico_cyw43_driver\CMakeLists.txt` | CYW43 driver cmake — `PICO_CYW43_SUPPORTED` gate, pin override comments |
| `C:\Users\<user>\.pico-sdk\sdk\2.2.0\src\rp2_common\pico_cyw43_driver\cyw43_bus_pio_spi.pio` | PIO SPI program — governs GPIO constraint for alternate pinouts |
| `configs/PicoW/BoardConfig.h` | Existing Pico W board config — user GPIO assignments, correctly omits GPIO 23–25, 29 |
| `configs/PicoW/PicoW.cmake` | `set(PICO_BOARD pico_w)` + `set(PICO_PLATFORM rp2040)` — RM2 same-pins path mirrors this |
| `configs/Pico2/Pico2.cmake` | `set(PICO_BOARD pico2)` + `set(PICO_PLATFORM rp2350-arm-s)` — pattern for RP2350A variant |
| `headers/helper.h` | `isValidPin()` using `NUM_BANK0_GPIOS` — correctly handles 30 vs 48 GPIO count |
