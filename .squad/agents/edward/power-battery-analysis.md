# Power Management & Battery Reporting Analysis

**Author:** Edward (Firmware Developer)  
**Date:** 2026-03-28  
**Status:** Research complete — grounded in codebase, Pico SDK 2.2.0, and BTStack docs  
**Requested by:** Fortinbra  
**Target hardware:** Pimoroni Pico Lipo 2 XL W (RP2350B + CYW43 + LiPo charger)

---

## 1. Pimoroni Pico Lipo 2 XL W — Hardware Summary

### Chip variant
The board uses the **RP2350B**, which is the 48-pin QFN variant of RP2350 (vs. RP2350A at 30 GPIO). This matters for GP2040-CE:

- **`NUM_BANK0_GPIOS = 48`** — GP2040-CE already handles this correctly. `isValidPin()` in `headers/helper.h` uses `NUM_BANK0_GPIOS` from the SDK, so all pin-validation code is automatically correct for RP2350B at compile time. No hardcoded GPIO limits exist in `src/` or `headers/`.
- GPIO 0–47 are available on RP2350B (vs. 0–29 on RP2040/RP2350A).
- CYW43 consumes several GPIO for SPI/SDIO bus (typically 23–25 on standard Pico W layout; Pimoroni may route differently — **verify against schematic**).

### No existing Pimoroni board config
Search of all 56 `configs/` directories: **zero Pimoroni configs**. A new `configs/PimoroniPicoLipo2XLW/` directory with `BoardConfig.h`, `.cmake`, and `README.md` must be created. The `.cmake` file will set:

```cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
```

This is the only change needed to activate both RP2350B support and `PICO_CYW43_SUPPORTED=1` (SDK-provided flag that gates BTStack availability).

### Battery ADC pin
Pimoroni Pico Lipo boards read battery voltage on **GPIO29 / ADC3** via a voltage divider. The standard Pimoroni divider ratio is **200 kΩ / 100 kΩ → factor = 3.0** (V_ADC = V_BAT / 3). The ADC reference is 3.3 V.

> ⚠️ **TBD:** Confirm exact divider resistor values from the Pico Lipo 2 XL W schematic. The 200k/100k ratio is standard on Pimoroni Pico LiPo (RP2040) — verify it is unchanged on the RP2350B variant.

### VBUS detection pin
On standard Pico-family boards, **GPIO24** is connected to VBUS sense through a resistor divider (VBUS is 5 V; GPIO input is 3.3 V). Reads HIGH when USB cable is inserted.

> ⚠️ **TBD:** Confirm GPIO24 is exposed and wired to VBUS sense on the Pimoroni board. The board uses USB-C with onboard MCP73831 (or compatible) charger — VBUS routing may differ from bare Pico layout.

---

## 2. Battery Level Reporting

### BTStack Battery Service API

BTStack ships as a submodule inside the Pico SDK (not in GP2040-CE's `lib/`). The relevant files, accessible via the SDK at `${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/`, are:

**`battery_service_server.h`** — public API surface:
```c
/**
 * Init Battery Service Server with ATT DB.
 * @param battery_value  Initial value in range 0–100
 */
void battery_service_server_init(uint8_t battery_value);

/**
 * Update battery value. Triggers BLE notifications if client is subscribed.
 * @param battery_value  New value in range 0–100
 */
void battery_service_server_set_battery_value(uint8_t battery_value);
```

The service requires a GATT profile file that imports the battery service template:
```
// hid_device.gatt  (or equivalent for GP2040-CE BT driver)
#import <battery_service.gatt>
```

This adds the standard BT Battery Service (UUID **0x180F**) with a Battery Level characteristic (UUID **0x2A19**) that supports READ and NOTIFY.

### Integration in CMake

```cmake
if (PICO_CYW43_SUPPORTED)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pico_btstack_base
        pico_btstack_ble          # Battery Service is BLE GATT
        pico_btstack_classic      # If also running BT Classic HID
        pico_btstack_run_loop_async_context
    )
endif()
```

### ADC → LiPo percentage formula

GP2040-CE's existing ADC pattern (established in `src/addons/analog.cpp`):
```cpp
// Initialization (once, during setup):
adc_init();
adc_gpio_init(29);              // GPIO29 = ADC3 = battery pin

// Per-read (e.g., every 10–60 seconds is sufficient for battery reporting):
adc_select_input(3);            // ADC channel = GPIO pin - ADC_PIN_OFFSET (26)
uint16_t raw = adc_read();      // 12-bit, range 0–4095
```

**Voltage and percentage conversion:**
```cpp
constexpr float ADC_VREF        = 3.3f;
constexpr float ADC_MAX         = 4095.0f;
constexpr float BATT_DIVIDER    = 3.0f;   // 200k/100k divider — CONFIRM FROM SCHEMATIC
constexpr float BATT_MIN_V      = 3.0f;   // 0%   — LiPo fully discharged
constexpr float BATT_MAX_V      = 4.2f;   // 100% — LiPo fully charged

float v_adc  = (raw / ADC_MAX) * ADC_VREF;
float v_bat  = v_adc * BATT_DIVIDER;
float pct    = (v_bat - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V) * 100.0f;
uint8_t batt = (uint8_t)std::clamp(pct, 0.0f, 100.0f);

battery_service_server_set_battery_value(batt);
```

> **Note:** LiPo discharge is non-linear. The simple linear map is acceptable for a controller — a lookup table can be added later for accuracy at the low end.

### VBUS / charging state handling

```cpp
bool usb_connected = gpio_get(24);   // HIGH = USB cable present

if (usb_connected) {
    // Charging or fully charged — report 100% to host
    battery_service_server_set_battery_value(100);
    // Also update GamepadAuxPower for any USB driver that reports power state:
    gamepad->auxState.power.pluggedIn = true;
    gamepad->auxState.power.charging  = true;
    gamepad->auxState.power.level     = 100;
} else {
    // Battery-only mode — read ADC and report real %
    uint8_t batt = readBatteryPercent();
    battery_service_server_set_battery_value(batt);
    gamepad->auxState.power.pluggedIn = false;
    gamepad->auxState.power.charging  = false;
    gamepad->auxState.power.level     = batt;
}
```

`GamepadAuxPower` struct is already defined in `headers/gamepad/GamepadAuxState.h` with `charging`, `pluggedIn`, and `level` fields. The `level` field is currently hardcoded to `GAMEPAD_AUX_MAX_POWER (100)` in `src/gp2040.cpp:78`. The battery service implementation replaces this hardcode.

### Polling rate

Battery level should be read at most every **10–30 seconds**. ADC reads are fast (~2 µs) but calling `battery_service_server_set_battery_value()` unnecessarily floods GATT notifications. Recommended: sample every 30 s; only call the setter if the value changed by ≥ 1%.

---

## 3. Power Management

### Current state: zero power management

As of this analysis, GP2040-CE has NO power management code:
- No `pico/sleep.h` usage
- No `cyw43_wifi_pm()` calls
- No sleep/dormant entry
- Power is always assumed on (`pluggedIn = true`, `level = 100` hardcoded)

This is correct for USB builds (VBUS is always present). BT+battery builds are a fundamentally new paradigm — the first time power budget matters.

### Available SDK APIs (Pico SDK 2.2.0)

**RP2350 sleep (`pico/sleep.h`):**
| Function | Effect |
|---|---|
| `sleep_run_from_xosc()` | Switch clocks to crystal oscillator (12 MHz) — reduces dynamic power |
| `sleep_goto_sleep_until(datetime_t*, callback)` | Timed sleep with RTC wake; retains RAM |
| `sleep_goto_dormant_until_pin(pin, edge, level)` | Deepest sleep; wake on GPIO edge; retains RAM but loses all PLLs — must re-init clocks |
| `recover_from_sleep(scb_orig, clock0_orig, clock1_orig)` | Re-initialize clocks after dormant |

**Key distinction:**
- **SLEEP:** Retains all clocks and RAM state. Lower power than ACTIVE. Wake is fast (<1 ms typical). CPU stops executing; peripherals (WDT, RTC) can wake.
- **DORMANT:** All oscillators stopped (except XOSC in some configs). Deepest RP2350 power state. RAM retained. Wake from GPIO edge only. Re-init of PLLs required on wake (~10 ms). **Do not enter DORMANT if CYW43 needs to maintain a BT connection.**

**CYW43 radio power management (`cyw43_arch.h`):**
```c
// Set WiFi power management mode:
cyw43_wifi_pm(&cyw43_state, pm_mode);
```

| Constant | Value | Behavior |
|---|---|---|
| `CYW43_PERFORMANCE_PM` | `0x00A00000` | No power saving; radio always active; lowest latency |
| `CYW43_DEFAULT_PM` | `0x00A50000` | Balanced; ~20 ms sleep intervals; default SDK setting |
| `CYW43_AGGRESSIVE_PM` | `0x00A51000` | Maximum power saving; higher BT/WiFi latency (~100 ms) |
| `CYW43_NO_POWERSAVE_PM` | `0x00A00000` | Alias for PERFORMANCE_PM |

> **Note:** These constants control WiFi radio power save, not Bluetooth radio directly. Bluetooth power management in BTStack is handled separately via HCI sniff mode: `hci_send_cmd(&hci_sniff_mode, handle, max_interval, min_interval, attempt, timeout)`. For a gamepad, sniff mode intervals of 20–40 ms give acceptable button latency while saving power.

### Proposed Power State Machine

```
                    USB inserted (GPIO24 HIGH)
                           ↓
┌──────────────────────────────────────────────────┐
│  USB_CONNECTED                                   │
│  - Full clock (150 MHz)                          │
│  - CYW43_PERFORMANCE_PM (if BT active)           │
│  - Battery reporting: always 100%                │
│  - LEDs: full brightness                         │
│  - Charging: true                                │
└──────────────────────────────────────────────────┘
         ↑ USB inserted        ↓ USB removed
┌──────────────────────────────────────────────────┐
│  ACTIVE (battery powered)                        │
│  - Full clock (150 MHz)                          │
│  - CYW43_PERFORMANCE_PM                          │
│  - Battery reporting: ADC read every 30 s        │
│  - LEDs: normal operation                        │
│  Transition → IDLE: no button input for N s      │
│              (suggest N = 30–60 s, configurable) │
└──────────────────────────────────────────────────┘
         ↑ any button press
┌──────────────────────────────────────────────────┐
│  IDLE                                            │
│  - Reduce clock (48 MHz via sleep_run_from_xosc) │
│  - CYW43_DEFAULT_PM or CYW43_AGGRESSIVE_PM       │
│  - BT HCI sniff mode (40 ms intervals)           │
│  - LEDs: dimmed or off                           │
│  - Battery reporting: continue at reduced rate   │
│  Transition → DEEP_SLEEP: idle for M s (M >> N)  │
│              (suggest M = 300 s, configurable)   │
│  Transition → ACTIVE: any button press           │
└──────────────────────────────────────────────────┘
         ↑ any button press (GPIO IRQ → exit dormant)
┌──────────────────────────────────────────────────┐
│  DEEP_SLEEP                                      │
│  - Enter RP2350 DORMANT via sleep_goto_dormant_  │
│    until_pin(any_button_gpio, edge, low)         │
│  - CYW43 BT disconnected before entering         │
│  - LEDs off                                      │
│  - Battery NOT reported (radio off)              │
│  Wake: any button press                          │
│  Post-wake: recover_from_sleep(), re-init CYW43, │
│             re-advertise BT HID, reconnect       │
└──────────────────────────────────────────────────┘
```

**State timings (suggested defaults, user-configurable via web configurator):**
| Timeout | Default | Description |
|---|---|---|
| `idle_timeout_ms` | 30,000 ms | ACTIVE → IDLE on no input |
| `sleep_timeout_ms` | 300,000 ms | IDLE → DEEP_SLEEP on no input |

### Implementation location

A new `src/power/PowerManager.cpp` + `headers/power/PowerManager.h` is the cleanest approach — a singleton class with `update(bool any_input, bool usb_present)` called from the main loop, similar to the existing addon pattern. Gated with `#if defined(PICO_CYW43_SUPPORTED)`.

---

## 4. CYW43 Radio PM Modes — When to Switch

| State | WiFi PM | BT approach | Rationale |
|---|---|---|---|
| USB_CONNECTED | PERFORMANCE_PM | Full throughput | USB present = no power constraint; web configurator may be active (uses WiFi) |
| ACTIVE | PERFORMANCE_PM | Active mode | Playing a game — minimum latency required for button inputs |
| IDLE | DEFAULT_PM | HCI sniff (40 ms) | Light power saving; still maintain BT connection; ~100 µA radio savings |
| DEEP_SLEEP | N/A (radio off) | Disconnect before dormant | CYW43 must be gracefully shut down before RP2350 enters DORMANT; re-init on wake |

**CYW43 shutdown sequence before DORMANT:**
```c
// 1. Disconnect BT
gap_disconnect(connection_handle);
// 2. Power down WiFi (BT radio shares the chip — no standalone BT-off API)
cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);
// 3. Deinit CYW43 arch (or put in lowest-power state)
cyw43_arch_deinit();
// 4. Now enter RP2350 DORMANT
sleep_goto_dormant_until_pin(wake_pin, true, false);  // wake on falling edge (button press)
// 5. On wake: re-init
cyw43_arch_init();
btstack_init();                                        // re-register services
gap_advertisements_enable(true);                       // re-advertise for host reconnection
```

> ⚠️ **TBD:** CYW43 init/deinit cycle latency on RP2350 is not well-characterized in public docs. May add 500 ms–2 s to wake-from-DORMANT time. User experience impact (controller unresponsive after button wake) needs testing.

---

## 5. Namespace Conflict Mitigation (Confirmed from Prior Analysis)

**Confirmed approach:** Translation-unit isolation.

The `hid_report_type_t` collision between TinyUSB (`lib/tinyusb/src/class/hid/hid.h:84`) and BTStack (`btstack_hid.h:109`) is the only hard compile-time collision. The mitigation is:

1. **Battery service code lives in `src/drivers/bt/`** — these `.cpp` files include ONLY BTStack headers (including `battery_service_server.h`). They never include `tusb.h` or any TinyUSB class headers.

2. **Existing TinyUSB files are untouched** — no `#include` changes in any of the existing driver files.

3. **Firewall rule (for doc + code review):** No `.cpp` file may `#include` both `tusb.h` (or TinyUSB class headers) and `btstack_hid.h` (or `classic/hid_device.h`). Enforce in code review checklist.

4. **Optional `gp_hid_report_type_t`** for any shared abstraction boundary headers — avoids exposing either stack's typedef in neutral interface headers. Not required for battery service itself (battery service is BLE-only, no HID type collision).

5. **CMake gate:** All BT code (including battery service) is compiled conditionally:
   ```cmake
   if (PICO_CYW43_SUPPORTED)
       target_sources(${PROJECT_NAME} PRIVATE
           src/drivers/bt/BTHIDDriver.cpp
           src/power/PowerManager.cpp
       )
   endif()
   ```

The `#undef` approach was evaluated and **rejected** — fragile across compiler and library versions.

---

## 6. Caveats and Open Questions for Hughes (TBD List)

| # | Item | Why it matters |
|---|---|---|
| 1 | **Pimoroni Pico Lipo 2 XL W voltage divider exact ratio** — assumed 200k/100k (÷3); confirm from schematic | Wrong divider → wrong battery % calculation |
| 2 | **GPIO24 VBUS sense availability on Pimoroni board** — standard Pico routing assumed; Pimoroni USB-C implementation may differ | VBUS detection is the trigger for USB_CONNECTED state |
| 3 | **CYW43 GPIO routing on Pico Lipo 2 XL W** — which GPIO the CYW43 SPI/SDIO occupies; needed to list truly free GPIO for buttons and LEDs | Board config GPIO assignment depends on this |
| 4 | **CYW43 init/deinit cycle latency on RP2350** — wake-from-DORMANT time including CYW43 re-init + BT re-advertisement not measured | User experience: how long is the controller "dead" after button wake from deep sleep |
| 5 | **BT HCI sniff mode latency vs. gaming acceptance** — sniff interval of 40 ms means worst-case 40 ms input latency in IDLE; may be unacceptable for competitive play | Should sniff mode be disabled for gaming? |
| 6 | **Battery service is BLE only or BT Classic too?** — BTStack's `battery_service_server` is a GATT/BLE service; BT Classic HID does not natively expose battery level in the same way | If host connects via BT Classic (not BLE), battery % may not be reported |
| 7 | **`PICO_BOARD=pico2_w` vs. custom board file for Pimoroni** — Pico SDK has `pico2_w.h` for RPi Pico 2 W; Pimoroni board may need a custom board header if GPIO routing differs | Board config correctness |
| 8 | **Idle/sleep timeouts as user-configurable fields** — if timeouts go into `AddonOptions` protobuf, they need `PowerOptions` message + web configurator fields | Scope of protobuf change needed |

---

## 7. Files Referenced

| File | Role |
|---|---|
| `headers/gamepad/GamepadAuxState.h:122–127` | `GamepadAuxPower` struct (`charging`, `pluggedIn`, `level`) |
| `src/gp2040.cpp:75–78` | Power state hardcoded initialization (`pluggedIn=true`, `level=100`) |
| `src/addons/analog.cpp:11–14` | ADC constants (`ADC_MAX=4095`, `ADC_PIN_OFFSET=26`) |
| `headers/helper.h:38–40` | `isValidPin()` using `NUM_BANK0_GPIOS` |
| `configs/PicoW/BoardConfig.h` | Closest existing wireless board config (GPIO layout reference) |
| `configs/Pico2/Pico2.cmake` | RP2350-arm-s target (model for new board `.cmake`) |
| `${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.h` | BTStack Battery Service API |
| `${PICO_SDK_PATH}/lib/btstack/src/ble/gatt-service/battery_service_server.c` | BTStack Battery Service implementation |
| `lib/tinyusb/src/class/hid/hid.h:84` | TinyUSB `hid_report_type_t` (collision point — must isolate) |
