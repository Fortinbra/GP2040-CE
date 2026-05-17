# Bluetooth HID Support — Feature Planning

**Last updated:** 2026-03-28  
**Maintained by:** GP2040-CE core team  
**Status:** Planning / Not yet implemented
**SDK version:** 2.2.0+

---

## Overview

This document describes the planned **Bluetooth HID (Human Interface Device) support** for GP2040-CE. Bluetooth HID will enable wireless gamepad connectivity on compatible boards, allowing users to pair and play with modern gaming systems (phones, tablets, PCs, consoles) without cables or USB receivers.

**What this adds:**
- Wireless gamepad output via Bluetooth HID Classic on supported boards
- Runtime switching between USB HID and Bluetooth HID output modes
- Persistent configuration for output mode and Bluetooth bonding keys
- A new multi-output architecture to support current USB modes alongside future wireless output modes

**Why it matters:**
GP2040-CE currently supports 17 USB HID output modes (XInput, PS4, Switch, keyboard, etc.) but has zero wireless capability. Bluetooth HID fills a significant gap for portable gaming and console play, unlocking wireless arcade stick use cases.

**Which boards benefit:**
- **Today:** Raspberry Pi Pico W (RP2040 + CYW43 wireless chip) and Raspberry Pi Pico 2 W (RP2350 + CYW43)
- **Future:** Additional wireless boards as they become available
- All other boards retain USB-only output

**Out of scope for this document:** GPIO output to retro consoles (SNES, N64, Dreamcast, Genesis, etc.). GP2040-CE reads FROM retro controllers via GPIO input add-ons (SNESpadInput, TG16padInput) but does not emit GPIO signals to emulate controllers for retro consoles. Retro console output is a separate architectural concern not addressed here and will be covered in a future document.

---

## Supported Hardware

### Wireless-Capable Boards

| Board | Chip | Wireless Hardware | Status |
|-------|------|-------------------|--------|
| Pico W | RP2040 + CYW43439 | WiFi + Bluetooth HID | ✅ Target |
| Pico 2 W | RP2350 + CYW43439 | WiFi + Bluetooth HID | ✅ Confirmed |
| Pimoroni Pico Lipo 2 XL W | RP2350B + CYW43439 | WiFi + Bluetooth HID + LiPo | ✅ Reference board |
| All other boards | RP2040 / RP2350 | None | USB only |

### Minimum Requirements

**For Bluetooth HID support:**
- Pico SDK version: **2.2.0 or later** (enforced in CMakeLists.txt)
- Pico W board with CYW43439 chip
- Available GPIO and flash storage for bonding keys
- CMake target linkage: `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device`

**Note on Pico 2 W and RP2350:** RP2350 + CYW43 support is fully confirmed in Pico SDK 2.2.0. Both Pico 2 W (RP2350A) and boards like Pimoroni Pico Lipo 2 XL W (RP2350B) support Bluetooth HID without additional porting work. The Pimoroni board is designated as the reference platform for battery-backed wireless development in GP2040-CE.

---

## Reference Hardware

### Pimoroni Pico Lipo 2 XL W

The **Pimoroni Pico Lipo 2 XL W** is the designated reference board for Bluetooth and battery-backed development in GP2040-CE.

**Specifications:**
- **Microcontroller:** RP2350B (48 GPIO, 150 MHz, 520 KB SRAM)
- **Wireless:** CYW43439 (WiFi + Bluetooth HID)
- **Battery:** Built-in LiPo charger (MCP73831), battery voltage sensing via GPIO29/ADC3
- **Power input:** USB-C with integrated charging
- **GPIO headroom:** 30+ GPIO available after CYW43 routing (RP2350B advantage over Pico 2 W)

**Why it's the reference:**
- **Confirmed working:** Fortinbra has validated Bluetooth HID on this board
- **Battery hardware included:** Simplifies battery voltage measurement and charging state detection
- **GPIO capacity:** The 48-pin RP2350B variant provides excellent headroom for multi-output configurations (BT + GPIO retro output)

**Build target:**
```cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
```

A new board configuration (`configs/PimoroniPicoLipo2XLW/`) will be created during Phase 1 implementation. This configuration will include:
- GPIO pin mapping and CYW43 SPI/SDIO routing
- ADC configuration for battery voltage measurement (GPIO29, voltage divider 3:1)
- VBUS detection configuration (GPIO24 for USB input detection)

---

## Architecture

### Current State: USB-Centric Output

Today, GP2040-CE uses a **single output transport**: USB HID via TinyUSB. The architecture is:

```
┌─────────────────────────────────────────────────┐
│ Main Loop (src/gp2040.cpp)                      │
│  + GPIO read                                     │
│  + Button processing (MPGS profiles, hotkeys)   │
│  + Input addons (SNES controller, etc.)         │
└────────────────┬────────────────────────────────┘
                 │ gamepad state
                 ▼
┌─────────────────────────────────────────────────┐
│ DriverManager::getDriver()->process(gamepad)    │
│  (One GPDriver subclass per USB HID mode)       │
└────────────────┬────────────────────────────────┘
                 │ HID report
                 ▼
┌─────────────────────────────────────────────────┐
│ TinyUSB (lib/tinyusb/) → USB HID output         │
└─────────────────────────────────────────────────┘
```

The `GPDriver` abstraction (`headers/gpdriver.h`) is the core output interface. All 17 USB HID modes are subclasses of `GPDriver`. The interface is **thoroughly USB-centric**:

```cpp
// Simplified GPDriver interface
class GPDriver {
    virtual void initialize() = 0;
    virtual bool process(Gamepad * gamepad) = 0;
    virtual void processAux() = 0;
    // ... USB-specific callbacks:
    virtual const uint8_t * get_descriptor_device_cb() = 0;
    virtual const uint8_t * get_hid_descriptor_report_cb(...) = 0;
    virtual uint16_t get_report(...) = 0;    // TinyUSB GET_REPORT
    virtual void set_report(...) = 0;         // TinyUSB SET_REPORT
    virtual bool vendor_control_xfer_cb(...) = 0;
    // ... etc
};
```

### Proposed Multi-Output Architecture

To support Bluetooth HID alongside USB, we will introduce an **Output Transport Abstraction Layer** above `GPDriver`. This decouples gamepad state processing from transport-specific concerns.

```
┌─────────────────────────────────────────────────────┐
│ Main Loop (gp2040.cpp)                              │
│  + GPIO read, button processing, input addons       │
└────────────────┬────────────────────────────────────┘
                 │ gamepad state
                 ▼
┌─────────────────────────────────────────────────────┐
│ OutputManager (NEW)                                 │
│  + Selects active output transport(s)               │
│  + Manages output mode: USB, BT, or USB+BT hybrid   │
└─────┬─────────────────────┬─────────────────────────┘
      │                     │
      ▼                     ▼
┌────────────────────┐  ┌──────────────────┐
│ DriverManager      │  │ BTHIDManager     │
│  + getDriver()     │  │  (NEW)           │
│  + 17 USB modes    │  │  + BT HID output │
└────────────────────┘  └──────────────────┘
      │                     │
      ▼                     ▼
┌─────────────────────────────────────────┐
│ TinyUSB HID + CYW43 Bluetooth stack      │
└─────────────────────────────────────────┘
```

### GPDriver and BTDriver

**GPDriver** remains the abstraction for USB output. No changes to existing drivers.

**BTDriver** (NEW) is a parallel output interface for Bluetooth HID:

```cpp
class BTDriver {
    virtual void initialize() = 0;              // CYW43 BT init
    virtual bool process(Gamepad * gamepad) = 0; // Send HID report via BT
    virtual void processAux() = 0;               // BT link management
    virtual void setPairMode(bool enabled) = 0;  // Pairing on/off
    virtual bool isPaired() = 0;                 // Query bonding state
    // ... BT-specific methods
};
```

Unlike `GPDriver`, `BTDriver` has **no TinyUSB callbacks** — Bluetooth HID is managed entirely by the BTstack library.

### Output Mode Switching

**Current behavior:** Output mode (XInput, PS4, Switch, etc.) is selected **once at boot** and cannot be changed without rebooting.

**Proposed behavior:** Add **runtime output mode switching** via:

1. **Boot-time selection:** If no override, use the saved mode from flash config
2. **Button combo or menu:** A new hotkey (e.g., `SELECT + L1 + R1`) or web configurator entry to switch between USB and BT modes
3. **Flash persistence:** The selected mode is saved to `GamepadOptions` protobuf so it persists across reboot

**Constraint:** Only **one primary HID output** is active at a time. USB HID and Bluetooth HID cannot both transmit the same gamepad state simultaneously (the transports are independent, and syncing identical reports would be confusing to the host OS). A board CANNOT simultaneously be:
- An XInput device over USB AND a Switch Pro Controller over Bluetooth

However, **WiFi (for web config)** can coexist with either USB or BT HID — the CYW43 chip time-multiplexes WiFi and Bluetooth internally, and neither competes with the USB PHY.

### Configuration Persistence

Output mode and Bluetooth bonding state are persisted in the protobuf config stored in flash:

```protobuf
// proto/enums.proto (NEW entries)
enum OutputMode {
    OUTPUT_MODE_USB = 0;
    OUTPUT_MODE_BLUETOOTH = 1;
}

// proto/config.proto (NEW)
message BluetoothConfig {
    repeated BondingKey bonded_devices = 1;
    bool pairing_enabled = 2;
}

// Extend GamepadOptions with:
OutputMode output_mode = 100;  // default: USB
BluetoothConfig bt_config = 101;
```

The web configurator will present an "Output Mode" dropdown and a "Paired Devices" list to manage Bluetooth bonding.

---

## Bluetooth HID Profile

### HID Classic vs. BLE: Which to Implement?

**Bluetooth HID Classic (L2CAP + HID):**
- **Pros:** Native gamepad support on all major gaming platforms (Nintendo Switch, PS4, PS5, Xbox, phones, PCs)
- **Cons:** Higher power consumption; requires 24 KB+ RAM for BTstack profile
- **Latency:** ~5–10 ms typical (suitable for arcade and fighting games)
- **Devices:** Nintendo Switch, PS5, Android, Windows, macOS all support BT Classic HID natively

**Bluetooth Low Energy (BLE) HID:**
- **Pros:** Lower power consumption (~50% less); fits in constrained devices
- **Cons:** Limited host support for gamepads (mostly mobile/PC, not consoles); more complex pairing
- **Latency:** ~10–20 ms typical
- **Devices:** Mobile phones and some PCs; Nintendo Switch does NOT support BLE gamepads

### Recommendation

**Phase 1 will implement Bluetooth HID Classic.** It has the broadest platform support (Switch, PS5, Android, PC) and zero fragmentation in the gaming ecosystem. BLE is a future optimization for mobile-specific builds, if desired.

### HID Report Descriptor

The Bluetooth HID report descriptor is **reusable from USB HID**. We can adopt the same gamepad layout from `HIDDriver` (a generic gamepad with 16 buttons, 2 analog sticks, and trigger axes) or match the active USB mode's report descriptor at runtime.

For simplicity, **Phase 1 will use a single standard gamepad descriptor** compatible with all BT hosts (16 buttons, dual analog sticks, dual triggers).

---

## Output Mode Switching

### User Interaction

Users will switch output modes via:

1. **Web configurator:** New "Output Mode" menu on the settings page (dropdown: USB / Bluetooth)
2. **Button combo (future):** A configurable hotkey (e.g., `SELECT + START + L1 + R1`) to toggle output mode on-the-fly
3. **Boot-time hold:** Extend the boot button-hold system to support "hold B1 → BT mode" in addition to existing overrides

### Switching Implementation

When the user selects a new output mode:

```
1. Save the new mode to flash (GamepadOptions.output_mode)
2. If the mode has changed:
   a. Disable the currently active output (disable TinyUSB HID or BT HID)
   b. Shut down the current driver (call driver->shutdown())
   c. Initialize the new driver (call newDriver->initialize())
   d. Resume normal operation
3. If a reboot is required (e.g., CYW43 state conflict), notify the user
```

**Note:** In early implementation, a reboot may be necessary to cleanly switch between USB and BT. Runtime switching without reboot is a future optimization.

### Constraint: Single Primary Output

**This is non-negotiable:** Only one primary HID output (USB or BT) is active at a time. The gamepad cannot claim to be both an XInput device over USB and a Switch Pro Controller over BT simultaneously.

**Why?**
- The operating system assumes exclusive ownership of a device. Reporting the same gamepad state via two independent transports would confuse the host.
- Latency and responsiveness are transport-specific. A user playing on Switch over BT should not see phantom inputs from a stale USB report.

**WiFi / web config is not affected:** The RNDIS/Ethernet gadget (used for web config at `192.168.7.1`) is separate from the HID output. Both USB HID and Bluetooth HID can coexist with WiFi.

---

## Battery Level Reporting

Battery voltage measurement and reporting to the Bluetooth host is essential for wireless gaming on battery-powered boards (e.g., Pimoroni Pico Lipo 2 XL W). For BT Classic HID (Phase 1), battery level is reported via the **HID descriptor**, not a GATT service.

### HID Descriptor Battery Strength Feature Report

BT Classic HID reports battery level as a **Feature report** in the HID report descriptor itself, per the HID Usage Tables specification:

- **Usage Page:** `0x06` (Generic Device Controls)
- **Usage:** `0x20` (Battery Strength)
- **Report type:** Feature (responds to `GET_REPORT` requests)
- **Logical range:** 0–100 (percentage)
- **Report size:** 8 bits

**HID descriptor snippet (add to the gamepad descriptor):**
```c
// Battery Strength — Feature report (BT Classic HID)
0x85, 0x02,        // Report ID (2)
0x05, 0x06,        // Usage Page (Generic Device Controls)
0x09, 0x20,        // Usage (Battery Strength)
0x15, 0x00,        // Logical Minimum (0)
0x26, 0x64, 0x00,  // Logical Maximum (100)
0x75, 0x08,        // Report Size (8 bits)
0x95, 0x01,        // Report Count (1)
0xB1, 0x02,        // Feature (Data, Variable, Absolute)
```

The host sends a `GET_REPORT(Feature, report_id=0x02)` request over the HID control channel (L2CAP PSM 0x0011). The device responds with the current battery percentage (0–100). The host may poll this periodically or on connection.

**BTStack API for handling battery queries:**
```c
// Register callback to handle GET_REPORT(Feature) requests
hid_device_register_report_request_callback(report_request_cb);

// In the callback, respond with battery percentage:
static int report_request_cb(uint16_t hid_cid,
                             hid_report_type_t report_type,
                             uint16_t report_id,
                             int * out_size,
                             uint8_t * out_report) {
    if (report_type == HID_REPORT_TYPE_FEATURE && report_id == 0x02) {
        bool usb = gpio_get(24);
        *out_report = usb ? 100 : readBatteryPercent();
        *out_size = 1;
        return 0;  // success
    }
    return -1;    // not handled
}
```

The full HID descriptor (including the battery Feature report) is passed to both `hid_device_init()` and the `hid_sdp_record_t` struct in `hid_create_sdp_record()`, so the host discovers the battery capability via SDP.

### BLE Battery Reporting (Current Implementation)

Battery percentage is currently delivered over **BLE GATT Battery Service** (UUID `0x180F`), not BT Classic HID Feature reports.

Current behavior in `src/BLEHIDManager.cpp`:

- The Battery Service is initialized at BLE startup with `battery_service_server_init(_readBatteryPercent())`.
- During runtime, battery reads are attempted only when BLE is both connected and notifying.
- Update cadence is throttled to 30 seconds (`_lastBatteryUpdateMs`) and only pushes when value changes.
- Pushing uses `battery_service_server_set_battery_value(level)`.

Current conversion logic in `_readBatteryPercent()`:

```c
// Uses BATTERY_ADC_CHANNEL when BATTERY_ADC_GPIO is defined.
adc_select_input(BATTERY_ADC_CHANNEL);
uint16_t raw = adc_read();
if (raw <= 1241) return 0;
if (raw >= 1737) return 100;
return (uint8_t)((raw - 1241) * 100 / 496);
```

These constants correspond to a 12-bit ADC (`0..4095`), `3.3V` reference, `3:1` divider, and a `3.0V..4.2V` LiPo window.

### Board Config Requirements for Battery Sense

For board-level battery sensing to be active in BLE battery reports, board config must define at least:

- `BATTERY_ADC_GPIO`
- `BATTERY_ADC_CHANNEL`

If `BATTERY_ADC_GPIO` is not defined, `_readBatteryPercent()` returns `100` unconditionally.

Note: `BATTERY_VOLTAGE_DIVIDER`, `BATTERY_MIN_VOLTAGE`, and `BATTERY_MAX_VOLTAGE` are useful board documentation macros, but current BLE conversion code uses fixed raw ADC thresholds and does not consume those macros yet.

### Current Limits and UI Visibility

- There is currently no BLE battery field exposed in `/api/getBleHidStatus` (`src/webconfig.cpp`) or in the Web Config BLE status panel (`www/src/Pages/SettingsPage.jsx`).
- There is currently no VBUS/charging override in the BLE battery reporting path; reported value comes from ADC conversion (or the 100% fallback when battery ADC macros are absent).

### Validation Checklist (Recommended)

Use this checklist when validating a board battery setup:

1. Confirm board macros define the intended ADC input (`BATTERY_ADC_GPIO` and `BATTERY_ADC_CHANNEL`).
2. Measure actual battery voltage on hardware with a meter.
3. Read host-reported BLE battery percentage (OS battery indicator for the paired BLE controller).
4. Verify BLE value tracks expected percentage for the measured voltage using the current linear conversion.
5. Check edge behavior near low and high thresholds (approximately `3.0V -> 0%`, `4.2V -> 100%`).
6. Repeat at multiple points across discharge to quantify linear-model error.

---

## Power Management

This is the first time GP2040-CE must actively manage power. Previous builds were USB-powered (VBUS always present) and treated power as unlimited. Battery-backed wireless builds require a new paradigm.

### Power States

The firmware implements a four-state power state machine:

**USB_CONNECTED** (always full power)
- Full clock speed (150 MHz)
- CYW43 radio: `CYW43_PERFORMANCE_PM` (no power saving)
- Battery reporting: always 100%
- Entry: VBUS (GPIO24) goes HIGH
- Exit: USB removed (VBUS goes LOW)

**ACTIVE** (playing, full power)
- Full clock speed (150 MHz)
- CYW43 radio: `CYW43_PERFORMANCE_PM` (no power saving)
- Battery reporting: ADC read every 30 seconds
- Entry: Boot or any button press from IDLE/DEEP_SLEEP
- Exit: No input for N seconds (default N = 30–60 s, configurable)

**IDLE** (light activity, reduced power)
- Reduced clock speed (48 MHz via `sleep_run_from_xosc()`)
- CYW43 radio: `CYW43_DEFAULT_PM` or `CYW43_AGGRESSIVE_PM`
- Bluetooth sniff mode: HCI sniff every 40 ms (maintains connection, reduces wake-up latency)
- LEDs: dimmed or off
- Battery reporting: continue at reduced polling rate
- Entry: ACTIVE timeout elapsed
- Exit: Button press → ACTIVE, or idle for M seconds (default M = 300 s) → DEEP_SLEEP

**DEEP_SLEEP** (maximum power saving)
- RP2350 dormant mode: `sleep_goto_dormant_until_pin()` with all buttons as wake sources
- CYW43 radio: disabled (Bluetooth disconnected before dormant entry)
- Clock: stopped except XOSC
- LEDs: off
- Battery reporting: none (radio off)
- Entry: IDLE timeout elapsed
- Exit: Any button press (GPIO edge triggers wake)
- Post-wake: Re-initialize clocks, CYW43, and re-advertise for Bluetooth reconnection

### Implementation Location

A new `src/power/PowerManager.cpp` + `headers/power/PowerManager.h` provides a singleton interface:

```cpp
class PowerManager {
    enum PowerState { USB_CONNECTED, ACTIVE, IDLE, DEEP_SLEEP };
    
    void update(bool any_input, bool usb_present);  // Called once per main loop
    void setState(PowerState new_state);
    uint32_t getIdleTimeoutMs() const;
    void setIdleTimeoutMs(uint32_t ms);
    // ... etc
};
```

Called from `src/gp2040.cpp::loop()` after input processing:

```cpp
bool any_input = gamepad->hasAnyButtonPress();
bool usb_present = gpio_get(24);
powerManager.update(any_input, usb_present);
```

This approach is gated with `#if defined(PICO_CYW43_SUPPORTED)` — USB-only boards (no CYW43) skip power management entirely.

### CYW43 Radio Power Modes

The CYW43 chip supports three WiFi power-saving modes (which also affect Bluetooth radio latency):

| Mode | Constant | Latency Impact | Use Case |
|------|----------|-----------------|----------|
| Performance | `CYW43_PERFORMANCE_PM` (0x00A00000) | None — radio always on | USB power, gameplay, web config |
| Default | `CYW43_DEFAULT_PM` (0x00A50000) | ~20 ms wake-up | Balanced; Bluetooth idle |
| Aggressive | `CYW43_AGGRESSIVE_PM` (0x00A51000) | ~100 ms wake-up | Maximum power saving; acceptable for idle |

Set via:
```cpp
cyw43_wifi_pm(&cyw43_state, CYW43_DEFAULT_PM);
```

For **Bluetooth HID in IDLE state**, recommend `CYW43_DEFAULT_PM` with HCI sniff mode at 40 ms intervals. This maintains connection responsiveness (~40 ms worst-case input latency) while reducing power ~30% vs. PERFORMANCE_PM.

### DORMANT Entry/Exit Sequence

Transitioning to DORMANT requires graceful shutdown of CYW43:

```cpp
// Before DORMANT:
gap_disconnect(connection_handle);                   // Disconnect BT
cyw43_wifi_leave(&cyw43_state, CYW43_ITF_STA);     // Leave WiFi
cyw43_arch_deinit();                                // Power down CYW43

// Enter DORMANT (wake on button GPIO falling edge):
sleep_goto_dormant_until_pin(button_gpio, true, false);

// After wake (in interrupt or main loop):
cyw43_arch_init();                                  // Re-init CYW43
btstack_init();                                     // Re-register services
gap_advertisements_enable(true);                    // Re-advertise BT
```

**CYW43 init/deinit cycle latency:** Estimated 500 ms–2 s on RP2350. This is the "dead time" after pressing a button to wake from DORMANT. Accept this in user expectations for the deepest sleep mode.

### Configuration & User Control

Idle and sleep timeout durations are user-configurable via the web configurator:

```protobuf
// In GamepadOptions or new PowerOptions message:
optional uint32 idle_timeout_ms = 200;       // ACTIVE → IDLE (default 30000)
optional uint32 sleep_timeout_ms = 201;      // IDLE → DEEP_SLEEP (default 300000)
optional bool power_management_enabled = 202; // Default: true if PICO_CYW43_SUPPORTED
```

This allows users to tune power profiles for their use case (competitive gaming may disable DEEP_SLEEP; portable play may reduce IDLE timeout).

### Known Caveats (TBD)

- **CYW43 latency on wake:** Exact wake-from-dormant time including CYW43 re-init and Bluetooth re-advertisement not yet measured on RP2350. May affect UX.
- **Sniff mode gaming impact:** 40 ms sniff interval in IDLE state is acceptable for casual gaming but may introduce input lag in competitive play. Option to disable sniff mode may be needed.
- **Voltage divider ratio:** Assumes Pimoroni standard (3:1); verify from board schematic before deployment.

---

## Implementation Roadmap

This roadmap is the ordered list of work items to implement Bluetooth HID support.

### Phase 0: Exploration & Testing (DONE)
- ✅ Analyze existing `GPDriver` architecture and USB output patterns
- ✅ Confirm CYW43 and BTstack capabilities on Pico W
- ✅ Review Pico SDK 2.2.0 BT HID examples

### Phase 1: Core Bluetooth HID on Pico W (NEXT)

**Dependency:** Phase 0

| Task | Estimate | Description |
|------|----------|-------------|
| **1.1** Add CMake BTstack linkage | 1 day | Add `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device` targets to `CMakeLists.txt` (only for Pico W) |
| **1.2** Implement `BTHIDDriver` class | 2–3 days | New class mimicking `GPDriver` interface but calling `hid_device_send_interrupt_message()` instead of TinyUSB. Include gamepad HID descriptor and report handling. |
| **1.3** CYW43 initialization in firmware | 1 day | Call `cyw43_arch_init()` in `gp2040.cpp::setup()` before `tud_init()`. Ensure WiFi coexistence. |
| **1.4** Output mode persistence | 1 day | Extend `GamepadOptions` protobuf with `output_mode` and `BluetoothConfig` fields. Update config save/load. |
| **1.5** Output mode selection UI | 2–3 days | Add "Output Mode" dropdown to web configurator. Update API endpoints to read/save mode. |
| **1.6** Runtime output switching | 1–2 days | Implement `OutputManager` to switch between drivers at runtime without reboot (or reboot cleanly if needed). |
| **1.7** Bluetooth bonding / pairing | 2–3 days | Integrate BTstack bonding APIs to store and restore bonding keys. Add pairing mode toggle. |
| **1.8** Testing on real hardware | 3–5 days | Test pairing and playback on Switch, PS5, Android, Windows, macOS. Verify USB and BT don't interfere. |

**Success criteria:**
- Pico W successfully pairs with Switch, PS5, and Android as a gamepad
- Output mode switching works (USB → BT → USB)
- Buttons, analog sticks, and triggers work as expected
- No USB HID interference when BT is active

### Phase 2: Documentation & Polish (AFTER 1.8)

| Task | Estimate | Description |
|---|---|---|
| **2.1** Bluetooth troubleshooting guide | 1 day | Pairing issues, re-bonding, clearing bonding cache, host compatibility notes |
| **2.2** API reference for custom boards | 1 day | How to add Bluetooth support to custom RP2040 boards (CYW43 wiring requirements) |
| **2.3** User guide in main README | 1 day | Quick-start: how to pair Pico W to a console and switch output modes |

### Phase 3: Future Enhancements (NO TIMELINE)

- **Pico 2 W support:** Once Raspberry Pi validates CYW43 integration with RP2350, add `configs/Pico2W/` config
- **BLE HID:** Optional BLE gamepad mode for mobile-specific use
- **Simultaneous USB + BT:** If demand exists, explore a proxy mode where USB passes through while BT output is primary (complex; not recommended)
- **Button combo switching:** Add a runtime hotkey to toggle output mode without web configurator
- **BT range testing:** Formal range & interference testing

---

## Known Limitations

### RP2350 + CYW43 Support Confirmed

**Status:** Fully supported in Pico SDK 2.2.0

RP2350 + CYW43 (both Pico 2 W and Pimoroni Pico Lipo 2 XL W) support Bluetooth HID without additional SDK porting work. The CYW43 initialization stack is stable on RP2350's clock speeds and PIO timing. Board configurations will be created during Phase 1 implementation as needed.

### Single Primary Output

Only one HID output (USB or BT) is active at a time. The firmware cannot simultaneously advertise itself as both an XInput device over USB and a Switch Pro Controller over Bluetooth.

**Why:** The operating system expects exclusive ownership of a device. Sending the same input state via two independent transports would lead to ghost inputs and platform confusion.

**What this does NOT prevent:**
- USB HID + WiFi web config (both supported today)
- BT HID + WiFi web config (both can coexist)
- USB HID + BT HID on separate boards in the same setup

### Host Compatibility

**Bluetooth HID Classic is widely supported, but edge cases exist:**

| Host | Support | Notes |
|------|---------|-------|
| Nintendo Switch | ✅ Full | Standard BT gamepad pairing |
| PlayStation 5 | ✅ Full | Standard BT gamepad pairing |
| PlayStation 4 | ✅ Full | Standard BT gamepad pairing |
| Xbox Series X/S | ❌ No | Xbox uses proprietary 2.4 GHz protocol (not BT HID) |
| Xbox One | ❌ No | Xbox One uses proprietary protocol; USB only |
| Android (6.0+) | ✅ Full | Standard BT gamepad, `KEYCODE_*` mapping required |
| iOS (14.3+) | ⚠️ Partial | BT gamepad supported; some games may not recognize |
| macOS (10.15+) | ✅ Full | Standard BT gamepad |
| Windows 10+ | ✅ Full | Standard BT gamepad via Bluetooth settings |

**Limitation:** Xbox users will need to use USB HID mode or an Xbox-specific wireless receiver. GP2040-CE's Bluetooth implementation does not support Xbox proprietary protocols.

### CYW43 WiFi + Bluetooth Coexistence

The CYW43 chip supports WiFi and Bluetooth simultaneously, but:

- **Time-multiplexing:** Internally, CYW43 shares a single radio between WiFi and Bluetooth. The SDK time-multiplexes access.
- **Throughput trade-off:** When both are active, wireless throughput (WiFi bandwidth) may decrease ~10–20% due to radio switching overhead.
- **Low-priority data:** WiFi (web config) is lower priority than BT HID. Under heavy WiFi load, BT latency may increase.

**Recommendation:** For competitive play, disable WiFi on the web config to dedicate the radio to Bluetooth HID.

### TinyUSB + BTStack Header Namespace Conflict

**Status:** Identified and mitigated via translation-unit isolation

When GP2040-CE implements Bluetooth HID, both TinyUSB and BTStack libraries will be linked into the same firmware binary. Both define a `hid_report_type_t` typedef in the global C namespace with incompatible first enum values:

```c
// TinyUSB (lib/tinyusb/src/class/hid/hid.h:84)
typedef enum {
    HID_REPORT_TYPE_INVALID = 0,    // First value differs
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE
} hid_report_type_t;

// BTStack (pico-sdk/lib/btstack/src/btstack_hid.h:109)
typedef enum {
    HID_REPORT_TYPE_RESERVED = 0,   // First value differs
    HID_REPORT_TYPE_INPUT,
    HID_REPORT_TYPE_OUTPUT,
    HID_REPORT_TYPE_FEATURE
} hid_report_type_t;
```

**Compilation error if both included in the same `.cpp` file:** `error: redefinition of 'hid_report_type_t'`

**Mitigation:** Source-file isolation
- All Bluetooth HID driver code lives in dedicated translation units (`src/drivers/bt/BTHIDDriver.cpp` and related files) that **include ONLY BTStack headers**
- Existing USB driver files (`src/drivers/hid/HIDDriver.cpp`, `src/drivers/*.cpp`) continue to include TinyUSB headers as before
- **Firewall rule:** No `.cpp` file may `#include` both `tusb.h` (or TinyUSB class headers) and `btstack_hid.h` (or `classic/hid_device.h`)
- CMake successfully links both libraries in the same binary — the conflict is header-only, not link-time

This isolation is straightforward to enforce in code review and requires no changes to existing USB driver code.

---

## Testing

### Test Hardware

- **Nintendo Switch:** Standard test platform for BT gamepad compatibility
- **PlayStation 5 or PS4:** Console BT pairing and input validation
- **Android phone (API 24+):** Mobile gaming with BT gamepad
- **Windows PC / Mac:** PC gaming compatibility
- **Pico W board:** Target wireless platform

### Test Cases

**Pairing & Bonding:**

```
1. Boot Pico W and enable Bluetooth pairing mode
2. Scan for devices on Switch, PS5, Android (in settings)
3. Select "Pico W" and complete pairing
4. Verify bonding is saved (device reappears after reboot without re-pairing)
5. Re-pair with a second device (test bonding of multiple hosts)
6. Clear bonding and re-pair to verify key management
```

**Gamepad Input:**

```
1. Pair Pico W to Switch
2. In Switch settings, test all button presses (A, B, X, Y, L, R, ZL, ZR, SELECT, START)
3. Test analog stick range (min, center, max) on both sticks
4. Test trigger axes (L2, R2 from full release to full press)
5. Repeat for PS5, Android, PC
```

**Mode Switching:**

```
1. Boot in USB mode (default)
2. Switch output mode to BT via web configurator
3. Verify device disconnects from USB and is available over BT
4. Pair to a console
5. Switch back to USB mode
6. Verify device reconnects to the original USB host
7. Test repeated switching (USB → BT → USB → BT)
```

**Stress & Coexistence:**

```
1. Boot Pico W in USB mode with WiFi active
2. Access web configurator over USB-Ethernet
3. Switch output mode to BT
4. Keep WiFi active, pair to Switch
5. Play for 10 minutes, verify no input lag or dropouts
6. Disable WiFi, play for 10 minutes
7. Re-enable WiFi, play for 10 minutes
8. Measure BT HID latency with and without WiFi active (optional, using frame-rate tools)
```

**Compatibility Matrix:**

Document observed behavior on each host:

| Host | Pairing Works | Input Works | Notes |
|------|---------------|-------------|-------|
| Switch | ✅ | ✅ | List any button mapping surprises |
| PS5 | ✅ | ✅ | List any button mapping surprises |
| Android | ✅ | ✅ | Test with 2–3 games |
| Windows | ✅ | ✅ | Verify in Steam, Dolphin emulator |
| macOS | ? | ? | Test if available |

### Regression Testing

Ensure Phase 1 does not break existing USB HID modes:

```
1. Build firmware with all 17 USB modes enabled
2. Boot in each mode (XInput, PS4, Switch, keyboard, etc.)
3. Verify buttons and sticks work on the target platform
4. Verify web configurator still accessible
5. Verify SNES/TG16 input addons still work (if any boards have them configured)
```

---

## Related Documentation

> **Note:** This document has been updated to reflect confirmed RP2350 + CYW43 support and includes detailed specifications for battery reporting and power management, based on technical analysis for the Pimoroni Pico Lipo 2 XL W reference board. Bluetooth HID itself is **not yet implemented** on any GP2040-CE board. This document serves as the authoritative technical specification for Phase 1 implementation.

- **[RP2350 Support](./rp2350-support.md)** — Chip and board configuration details
- **[Dependency Updates](./dependency-updates.md)** — Pico SDK and library version notes
- **[DDI-SOCD Documentation](./ddi-socd.md)** — Input processing architecture (no BT changes needed)

---

## References

- [Raspberry Pi Pico W Datasheet](https://datasheets.raspberrypi.org/pico_w/pico_w_datasheet.pdf)
- [Pico SDK BTstack Documentation](https://www.raspberrypi.org/documentation/pico-sdk/networking.html#pico_btstack)
- [Bluetooth HID Device Profile Specification](https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=311735)
- [Bluetooth Core Specification 5.3](https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=570290)

