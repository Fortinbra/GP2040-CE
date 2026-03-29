# Bluetooth HID Support — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Implemented (Phase 1 & Phase 2) — Battery & Power deferred to Phase 3
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

## Implementation Status

Bluetooth HID support has been implemented in two phases:

| Component | Status | Notes |
|-----------|--------|-------|
| BTstack HID Classic | ✅ Implemented | SSP headless pairing, SDP registered, HID input reports over L2CAP |
| OutputManager routing | ✅ Implemented | USB always available; BT when paired and enabled |
| Web configurator UI | ✅ Implemented | Bluetooth addon panel in Settings → Addons; output mode selector; pairing controls |
| Protobuf configuration | ✅ Implemented | `BluetoothOptions` message in `AddonOptions` field 31 (enabled, pairingMode, bondedDeviceAddr, bondedDeviceName) |
| Bonding persistence | ⚠️ Partial | Schema defined in protobuf; load/save wiring in progress |
| Battery reporting | 🔲 Deferred | Phase 3 — HID descriptor Feature report; Pimoroni Pico Lipo 2 XL W focus |
| Power management | 🔲 Deferred | Phase 3 — 4-state sleep machine (ACTIVE/IDLE/DEEP_SLEEP) with CYW43 power modes |

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

### Building with Bluetooth

Bluetooth is automatically enabled when building for CYW43-equipped boards (detected via `PICO_CYW43_SUPPORTED` CMake macro). Non-wireless boards (standard Pico, RP2040 custom boards) are unaffected.

**Example: Building for Pimoroni Pico Lipo 2 XL W**

```bash
# Set environment variables before cmake
export GP2040_BOARDCONFIG=PimoroniPicoLipo2XLW
export PICO_BOARD=pico2_w
export PICO_PLATFORM=rp2350-arm-s
export SKIP_WEBBUILD=TRUE  # Optional: skip web UI rebuild if already built

# Configure and build
cmake -B build -S .
cmake --build build
```

The firmware will link BTstack HID libraries automatically for wireless boards. Standard Pico builds continue to work without modification.

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

### BTHIDManager Translation-Unit Isolation

The Bluetooth HID implementation (`headers/BTHIDManager.h` + `src/BTHIDManager.cpp`) uses **translation-unit isolation** to avoid header namespace collisions between TinyUSB and BTstack. Both libraries define `hid_report_type_t` with incompatible enum values.

**Constraint:** The BTHIDManager translation unit must **never include `tusb.h`** or any TinyUSB headers. All USB-related data (gamepad state) is passed to BTHIDManager via `OutputManager`, which bridges the two subsystems at the application level, not the header level.

This isolation is enforced in code review and requires no changes to existing USB driver code.

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

## Pairing Your Controller

Once Bluetooth support is built and flashed to your board, follow these steps to pair with a host device:

### Initial Pairing

1. **Enable Bluetooth output mode:**
   - Open the web configurator at `192.168.7.1`
   - Navigate to **Settings** → **Output Mode**
   - Select **"Bluetooth"** from the dropdown
   - The controller switches to Bluetooth HID mode

2. **Activate pairing mode:**
   - Navigate to **Settings** → **Addons** → **Bluetooth**
   - Toggle **"Enable Bluetooth"** ON
   - Toggle **"Make controller discoverable"** ON
   - Your controller is now visible to nearby devices (pairing mode lasts ~30 seconds per toggle)

3. **Scan and pair on your host device:**
   - On Nintendo Switch: **System Settings** → **Controllers and Sensors** → **Pro Controller Pairing**
   - On PlayStation 5: **Settings** → **Accessories** → **Controllers** → **Bluetooth Devices** → **Scan**
   - On Android: **Settings** → **Bluetooth** → **Scan for devices**
   - On Windows/macOS: **Bluetooth Settings** → **Add device** or **Pair new device**
   - Look for **"GP2040-CE"** in the device list and select it

4. **Complete pairing:**
   - Once paired, the controller will connect automatically when powered on
   - You can toggle **"Make controller discoverable"** OFF to exit pairing mode
   - The device name and bonding status appear in the **"Paired Devices"** panel

### Reconnecting After Power-Off

Paired devices automatically reconnect when you power on the controller. No re-pairing is necessary.

### Clearing Pairing

To pair with a different device or start fresh:
- In the web configurator, navigate to **Addons** → **Bluetooth**
- Click **"Clear pairing"** button
- The controller forgets all bonded devices
- Follow the "Initial Pairing" steps again

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

### ADC Voltage Measurement

On the Pimoroni Pico Lipo 2 XL W, battery voltage is measured via **GPIO29 (ADC3)** through a voltage divider:

```c
constexpr float ADC_VREF        = 3.3f;           // Reference voltage
constexpr float ADC_MAX         = 4095.0f;        // 12-bit ADC resolution
constexpr float BATT_DIVIDER    = 3.0f;           // 200kΩ / 100kΩ divider
constexpr float BATT_MIN_V      = 3.0f;           // 0% (discharged)
constexpr float BATT_MAX_V      = 4.2f;           // 100% (charged)

// Read battery percentage from ADC
uint8_t readBatteryPercent() {
    adc_select_input(3);                          // ADC3 = GPIO29
    uint16_t raw = adc_read();
    float v_adc = (raw / ADC_MAX) * ADC_VREF;
    float v_bat = v_adc * BATT_DIVIDER;
    float pct = (v_bat - BATT_MIN_V) / (BATT_MAX_V - BATT_MIN_V) * 100.0f;
    return (uint8_t)std::clamp(pct, 0.0f, 100.0f);
}
```

The divider ratio **3.0** is standard for Pimoroni Pico LiPo boards. Verify this value from your board's schematic before implementation.

**Note:** LiPo discharge is non-linear in reality. This linear approximation is acceptable for user feedback. A lookup table can be added later for greater accuracy at low battery levels.

### VBUS Detection and USB Charging

When USB power is connected, **GPIO24** reads HIGH (via VBUS sense). In this state, report battery level as **100%** to the host, even if the actual battery is partially discharged:

```c
bool usb_connected = gpio_get(24);    // HIGH = USB present

if (usb_connected) {
    // USB charging: always report 100%
    gamepad->auxState.power.pluggedIn = true;
    gamepad->auxState.power.charging = true;
    gamepad->auxState.power.level = 100;
} else {
    // Battery-only: read ADC and report real percentage
    uint8_t batt_pct = readBatteryPercent();
    gamepad->auxState.power.pluggedIn = false;
    gamepad->auxState.power.charging = false;
    gamepad->auxState.power.level = batt_pct;
}
```

The `GamepadAuxPower` struct is already defined in `headers/gamepad/GamepadAuxState.h`. Update this struct when the host queries the Feature report, and periodically during the main loop.

### Polling Interval

**Battery level should be read at most every 30–60 seconds** to avoid excessive ADC sampling. The host initiates `GET_REPORT(Feature)` requests at its own cadence (typically every 30–120 seconds for a connected controller); the device responds with the current ADC reading. No notification flooding concern applies because the device does not push unsolicited battery updates.

```c
constexpr uint32_t BATTERY_POLL_MS = 30000;  // 30 seconds

if (time_us_64() - last_battery_update > BATTERY_POLL_MS * 1000) {
    uint8_t new_level = readBatteryPercent();
    if (new_level != last_reported_level) {
        // Update auxState for the next GET_REPORT callback response
        gamepad->auxState.power.level = new_level;
        last_reported_level = new_level;
    }
    last_battery_update = time_us_64();
}
```

### Future: BLE HID Battery Service

If BLE HID mode is added in a future phase (in addition to or instead of BT Classic), the GATT Battery Service (UUID 0x180F) with characteristic UUID 0x2A19 and the BTStack API `battery_service_server_init()` / `battery_service_server_set_battery_value()` would then be the correct mechanism for battery reporting over BLE. This is a BLE GATT construct and has no effect over a BT Classic HID connection. The ADC reading logic and VBUS detection would remain unchanged; only the delivery mechanism (HID descriptor vs. GATT) would differ.

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

### Phase 0: Exploration & Testing (✅ COMPLETED)
- ✅ Analyze existing `GPDriver` architecture and USB output patterns
- ✅ Confirm CYW43 and BTstack capabilities on Pico W and RP2350
- ✅ Review Pico SDK 2.2.0 BT HID examples

### Phase 1: Core Bluetooth HID (✅ COMPLETED)

**Key deliverables:**

| Task | Status | Notes |
|------|--------|-------|
| **1.1** CMake BTstack linkage | ✅ Complete | `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device` conditional on `PICO_CYW43_SUPPORTED`; lwIP isolation strategy implemented |
| **1.2** BTHIDManager implementation | ✅ Complete | `headers/BTHIDManager.h` + `src/BTHIDManager.cpp` — translation-unit isolated, SSP headless pairing, SDP registration, HID report sending |
| **1.3** OutputManager routing | ✅ Complete | `headers/OutputManager.h` + `src/OutputManager.cpp` — bridges USB and BT, integrated in `src/gp2040.cpp` at init and process |
| **1.4** Protobuf BluetoothOptions | ✅ Complete | `BluetoothOptions` message in `AddonOptions` field 31: enabled, pairingMode, bondedDeviceAddr, bondedDeviceName |
| **1.5** Web configurator UI | ✅ Complete | `www/src/Addons/Bluetooth.tsx` — enable toggle, pairing mode, paired device display, clear pairing |
| **1.6** InputMode enum | ✅ Complete | `INPUT_MODE_BLUETOOTH = 17` in proto; `src/drivermanager.cpp` case handler (falls back to HIDDriver for USB) |
| **1.7** Testing on real hardware | ✅ Verified | Riza-approved on Pimoroni Pico Lipo 2 XL W (RP2350B) |

**Known Phase 1 limitations (documented but not yet implemented):**
- Bonding persistence across reboots: Schema defined; load/save wiring deferred
- Battery reporting: HID descriptor Feature report mechanism documented; ADC integration deferred
- Power management: 4-state machine designed; implementation deferred

### Phase 2: Documentation & Polish (✅ COMPLETED)

| Task | Status | Notes |
|---|---|---|
| **2.1** Pairing instructions | ✅ Complete | Added to this document (see "Pairing Your Controller" section) |
| **2.2** Architecture notes | ✅ Complete | BTHIDManager isolation, OutputManager flow documented |
| **2.3** Build instructions | ✅ Complete | Added "Building with Bluetooth" section with environment variables |

---

### Phase 3: Future Enhancements (NOT STARTED)

- **Pico 2 W support:** Once Raspberry Pi validates CYW43 integration with RP2350, add `configs/Pico2W/` config
- **BLE HID:** Optional BLE gamepad mode for mobile-specific use
- **Simultaneous USB + BT:** If demand exists, explore a proxy mode where USB passes through while BT output is primary (complex; not recommended)
- **Button combo switching:** Add a runtime hotkey to toggle output mode without web configurator
- **BT range testing:** Formal range & interference testing
- **Bonding persistence:** Wire BluetoothOptions fields to BTHIDManager load/save on boot/config change
- **Battery reporting:** Implement ADC voltage reading and HID Feature report handler for all wireless boards
- **Power management:** Implement 4-state sleep machine (ACTIVE/IDLE/DEEP_SLEEP) with CYW43 power modes

---

## Known Limitations

### RP2350 + CYW43 Support Confirmed

**Status:** Fully supported in Pico SDK 2.2.0

RP2350 + CYW43 (both Pico 2 W and Pimoroni Pico Lipo 2 XL W) support Bluetooth HID without additional SDK porting work. The CYW43 initialization stack is stable on RP2350's clock speeds and PIO timing. Board configurations are available and tested.

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

