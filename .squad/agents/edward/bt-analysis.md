# Bluetooth & Multi-Output Architecture — Technical Survey

**Author:** Edward (Firmware Developer)  
**Date:** 2026-03-28  
**Purpose:** Deep codebase survey for Hughes's feature doc on Bluetooth gamepad support and multi-output architecture.

---

## 1. USB HID / Output Layer

### Key Files and Classes

| File | Role |
|------|------|
| `headers/gpdriver.h` | Abstract base class `GPDriver` — defines the entire output interface |
| `headers/drivermanager.h` | Singleton `DriverManager` — selects and owns the active driver |
| `src/drivermanager.cpp` | Factory: maps `InputMode` enum → concrete driver instance |
| `src/usbdriver.cpp` | TinyUSB callback glue — delegates every USB callback to `DriverManager::getDriver()` |
| `proto/enums.proto:143–160` | `InputMode` enum — authoritative list of all output modes |
| `src/gp2040.cpp:129–195` | Boot-time mode selection and `DriverManager::setup()` call |
| `src/gp2040.cpp:281–353` | Main run loop: reads GPIO → processes addons → calls `inputDriver->process(gamepad)` → `tud_task()` |

### How USB HID Is Implemented

The firmware uses **TinyUSB** (`lib/tinyusb/`) for all USB device functionality. The TinyUSB callback surface is exposed in `src/usbdriver.cpp`, which delegates every callback to `DriverManager::getInstance().getDriver()`:

- `usbd_app_driver_get_cb()` → returns the active driver's `class_driver` (a `usbd_class_driver_t` struct)
- `tud_hid_get_report_cb()` / `tud_hid_set_report_cb()` → forwards to driver
- `tud_descriptor_*_cb()` → forwards to driver (device, configuration, HID report descriptors)
- `tud_vendor_control_xfer_cb()` → forwards to driver (used by XInput, Xbox One auth)

All 17 USB output profiles are concrete subclasses of `GPDriver`. Every class in `headers/drivers/*/` follows this pattern exactly.

### The GPDriver Abstraction (Critical for BT Design)

`GPDriver` (`headers/gpdriver.h`) is a pure-virtual interface with these key methods:

```cpp
virtual void initialize() = 0;             // called once by DriverManager::setup()
virtual void initializeAux() = 0;          // called by GP2040Aux (Core1)
virtual bool process(Gamepad * gamepad) = 0; // called every main loop iteration
virtual void processAux() = 0;             // called every Core1 iteration
virtual uint16_t get_report(...) = 0;      // TinyUSB GET_REPORT callback
virtual void set_report(...) = 0;          // TinyUSB SET_REPORT callback
virtual bool vendor_control_xfer_cb(...) = 0;
virtual const uint16_t * get_descriptor_string_cb(...) = 0;
virtual const uint8_t * get_descriptor_device_cb() = 0;
virtual const uint8_t * get_hid_descriptor_report_cb(...) = 0;
virtual const uint8_t * get_descriptor_configuration_cb(...) = 0;
virtual const uint8_t * get_descriptor_device_qualifier_cb() = 0;
virtual uint16_t GetJoystickMidValue() = 0;
virtual USBListener * get_usb_auth_listener() = 0;  // for PS4/XOne USB auth
```

**Critical finding:** `GPDriver` is thoroughly USB-centric. `get_descriptor_*_cb`, `get_report`, `set_report`, and `vendor_control_xfer_cb` are all TinyUSB-specific concepts. There is no abstraction layer for "output transport type" above `GPDriver`. Adding Bluetooth as a peer output type would require architectural work (see Section 5).

### How Output Mode (USB Profile) Is Selected

1. `proto/enums.proto` defines `InputMode` enum with 17 modes (values 0–16) plus `INPUT_MODE_CONFIG = 255`.
2. The active mode is **persisted in flash** as part of `GamepadOptions` protobuf struct.
3. At boot, `gp2040.cpp::setup()` (lines 92–195) reads the stored mode and checks for button-hold overrides (e.g., hold B1 at boot → XInput).
4. `DriverManager::setup(inputMode)` is called **once at boot** (`gp2040.cpp:195`). It instantiates the correct `GPDriver` subclass and calls `driver->initialize()`.
5. There is **no runtime mode switching** — once the driver is created, it runs for the entire session. Changing mode requires saving and rebooting.

### All Active Input Modes

From `proto/enums.proto:143–160`:
```
INPUT_MODE_XINPUT = 0       XInputDriver
INPUT_MODE_SWITCH = 1       SwitchDriver
INPUT_MODE_PS3 = 2          PS3Driver
INPUT_MODE_KEYBOARD = 3     KeyboardDriver
INPUT_MODE_PS4 = 4          PS4Driver
INPUT_MODE_XBONE = 5        XBOneDriver
INPUT_MODE_MDMINI = 6       MDMiniDriver (Sega Mega Drive Mini USB)
INPUT_MODE_NEOGEO = 7       NeoGeoDriver (SNK NeoGeo Mini USB)
INPUT_MODE_PCEMINI = 8      PCEngineDriver (PCEngine Mini USB)
INPUT_MODE_EGRET = 9        EgretDriver (Taito Egret II Mini USB)
INPUT_MODE_ASTRO = 10       AstroDriver (Astro City Mini USB)
INPUT_MODE_PSCLASSIC = 11   PSClassicDriver
INPUT_MODE_XBOXORIGINAL = 12 XboxOriginalDriver
INPUT_MODE_PS5 = 13         PS4Driver (PS4_ARCADESTICK variant)
INPUT_MODE_GENERIC = 14     HIDDriver
INPUT_MODE_SWITCH_PRO = 15  SwitchProDriver
INPUT_MODE_P5GENERAL = 16   P5GeneralDriver
INPUT_MODE_CONFIG = 255     NetDriver (RNDIS web config)
```

---

## 2. Existing GPIO / Retro Console Output Support

### Critical Distinction: INPUT vs OUTPUT

There are two very different usages of retro console GPIO in the codebase that must not be confused:

| Role | What it does | Code location |
|------|-------------|---------------|
| **GPIO INPUT from retro controllers** | Reads SNES/NES/TG16 controller state into the gamepad | `src/addons/snes_input.cpp`, `src/addons/tg16_input.cpp` |
| **GPIO OUTPUT to retro consoles** | Would emulate a retro controller for a console | **Does not exist** |

### GPIO INPUT add-ons (retro controller readers)

**SNESpadInput** (`src/addons/snes_input.cpp`, `headers/addons/snes_input.h`):
- Reads SNES/NES controllers via the serial shift-register protocol
- Protocol: clock pin (output), latch pin (output), data pin (input)
- Pins are **runtime-configurable** via `SNESOptions` protobuf (stored in flash)
- Auto-detects controller type: `SNES_PAD_BASIC`, `SNES_PAD_NES`, `SNES_PAD_MOUSE`
- Polls on a timer (`uIntervalMS`); translates button reads into `gamepad->state`
- This is an **input adapter** — it reads a physical SNES/NES controller and lets it drive the GP2040's gamepad state, which is then output via whatever USB mode is active

**TG16padInput** (`src/addons/tg16_input.cpp`, `headers/addons/tg16_input.h`):
- Reads TurboGrafx-16 / PC Engine controllers
- Protocol: OE pin (output/active-low), select pin (output), 4 data pins (input, active-low, with pull-up)
- Reads two nibbles (SELECT high / SELECT low) for standard 2-button or three nibbles for 6-button mode
- All pins configurable via `TG16Options` protobuf

**Boards using these addons:**

The "Reflex CTRL" family of boards use GPIO to interface legacy controller ports:
- `ReflexCtrlSNES` — SNES controller port (GPIO 2–13, 16–21 for button inputs)
- `ReflexCtrlNES` — NES controller port
- `ReflexCtrlGenesis6` — Sega Genesis/Mega Drive 6-button
- `ReflexCtrlSaturn` — Sega Saturn (with USB pass-through on GPIO 14)
- `ReflexCtrlVB` — Virtual Boy

**Key finding:** These boards read FROM retro controllers, then output TO a modern console/PC via USB (XInput, Switch, PS4, etc.). The GP2040-CE acts as a protocol translator, not as a retro console peripheral.

### What Does NOT Exist: GPIO Output to Retro Consoles

There is **no driver or addon** that:
- Drives clock/latch/data lines to emulate a SNES/NES controller
- Drives the 6-wire Dreamcast VMU/MapleBus protocol
- Drives N64's single-wire asynchronous serial protocol
- Drives a Genesis/MD gamepad port output

This is a significant gap if the multi-output architecture is to include native retro console connectivity.

---

## 3. Wireless / Bluetooth

### Wireless-Capable Boards

Only **one** wireless-capable board config exists:

| Config | `PICO_BOARD` | `PICO_PLATFORM` | Chip | Wireless |
|--------|-------------|----------------|------|---------|
| `PicoW` | `pico_w` | `rp2040` | RP2040 + CYW43439 | WiFi + BT (HW) |

Config file: `configs/PicoW/PicoW.cmake` (2 lines: PICO_BOARD and PICO_PLATFORM).

`Pico2W` (RP2350 + CYW43) does **not** exist. See Section 3.3.

### Existing Use of CYW43 (WiFi Only)

The PicoW's CYW43439 wireless chip is used **exclusively for WiFi** in the current firmware:
- `lib/lwip-port/` — lwIP TCP/IP stack port for Pico
- `src/drivers/net/NetDriver.cpp` — RNDIS/ECM/NCM USB gadget driver (creates a USB-Ethernet interface)
- `src/gp2040.cpp:295` — `rndis_init()` initializes the RNDIS web config stack
- The web configurator runs as an HTTP server at `192.168.7.1` over the USB-Ethernet gadget

**No CYW43 Bluetooth APIs are called anywhere in the project.** No `pico_btstack` CMake target is linked. No `btstack_*` headers are included. The CYW43's Bluetooth capability is completely unused.

### Bluetooth Stubs or Placeholders

**There are none.** A full-text search of `src/`, `headers/`, `configs/`, `proto/`, and `CMakeLists.txt` for `bluetooth`, `btstack`, `cyw43`, `BT_`, `hid_bt`, and `wireless` returns zero hits in project source code. The only `bluetooth` string in the entire non-library source is:

```cpp
// src/drivers/switchpro/SwitchProDriver.cpp:300–301
case SwitchCommands::BLUETOOTH_PAIR_REQUEST:
    // ...
    report[13] = 0x81;
```

This is **not BT functionality**. It is a USB HID command ID that the Nintendo Switch console sends to a Pro Controller over USB when pairing a previously-Bluetooth-paired controller. The firmware acknowledges it but does not implement BT.

### CYW43 Bluetooth Capability

The CYW43439 chip DOES have hardware Bluetooth Classic + BLE. The Raspberry Pi Pico SDK provides `pico_btstack` as a CMake target that enables BTstack over CYW43. This includes:

- `pico_btstack_hid_device` — Bluetooth HID device role
- `pico_btstack_ble` — BLE (Low Energy) stack

**For HID over BT Classic:** standard HID profile, SPP or HID-over-GATT for BLE.

**Key hardware constraint for PicoW:** The CYW43439 connects to RP2040 via SPI/SDIO on GPIO 24, 25, 29. USB device mode uses the RP2040's built-in USB PHY — a completely separate hardware interface. Therefore, **USB HID and CYW43 Bluetooth CAN run simultaneously on PicoW.** These are not competing for the same hardware resource.

### Pico2W (RP2350 + CYW43) Absence

`Pico2W` does not exist because the CYW43 driver stack has not been ported to work with the RP2350 variant of the Pico W. This is consistent with the prior analysis:
- Base firmware runs fine on RP2350 (three RP2350 configs are CI-tested)
- The gap is the CYW43 wireless stack integration for RP2350 (`PICO_BOARD=pico2_w`, `PICO_PLATFORM=rp2350-arm-s`)
- As of Pico SDK 2.2.0, `pico2_w` board support is present in the SDK but requires validation of the CYW43 integration layer at RP2350 clock speeds and PIO timing

---

## 4. Add-on / Driver Architecture

### Two Independent Plugin Systems

**GPDriver (output transport layer):**
- Base class: `GPDriver` (`headers/gpdriver.h`)
- Manager: `DriverManager` singleton (`headers/drivermanager.h`)
- One active driver at a time — selected at boot, never changed at runtime
- Drives all USB output: report descriptors, HID reports, auth callbacks
- 17 concrete implementations in `src/drivers/*/`

**GPAddon (input/processing layer):**
- Base class: `GPAddon` (`headers/gpaddon.h`)
- Manager: `AddonManager` (`headers/addonmanager.h`)
- Multiple addons active simultaneously
- Per-addon lifecycle: `available()` → `setup()` → per-loop: `preprocess()` / `process()` / `postprocess(bool reportSent)`
- `reinit()` support for addons that need to rebuild GPIO masks on profile change
- USB host addons register a `USBListener` via `LoadUSBAddon()` and `USBHostManager`

### Main Loop Integration (Core0)

`src/gp2040.cpp::run()` (lines 281–353):
```
while (true) {
    getReinitGamepad()        // handle profile switches
    debounceGpioGetAll()      // debounced GPIO read
    gamepad->read()           // read button states
    USBHostManager::process() // USB host (pass-through)
    addons.PreprocessAddons() // pre-processing (e.g., tilt, SOCD)
    gamepad->process()        // MPGS (button remapping, profiles)
    addons.ProcessAddons()    // input addons (SNES, TG16, analog, etc.)
    gamepad->hotkey()         // hotkey detection
    inputDriver->process(gamepad) // USB report generation + TinyUSB send
    tud_task()                // TinyUSB task (pump USB stack)
    addons.PostprocessAddons(sent) // post-USB (turbo timing, etc.)
}
```

### Core1 (GP2040Aux)

`src/gp2040aux.cpp` — runs display, LEDs, buzzer, DRV8833 rumble:
```
while (true) {
    addons.PreprocessAddons()
    addons.ProcessAddons()
    inputDriver->processAux() // driver aux work (PS4 auth sends, SwitchPro rumble, etc.)
}
```

### Runtime Output Switching — Current State

**There is no runtime output switching.** `DriverManager::setup()` is called once in `gp2040.cpp::setup()`. To change output mode, the user must:
1. Hold a button during boot (boot action mapping)
2. Or change mode in web config, which saves to flash and triggers a reboot

The `setInputMode()` → `save()` pattern (lines 199–201) forces a reboot path. No hot-swap code exists.

---

## 5. Gaps and Architectural Considerations

### What Would Need to Exist for Runtime Output Switching (USB ↔ BT ↔ GPIO)

**Problem:** `GPDriver` is USB-centric. Its interface is built around TinyUSB callbacks (`get_descriptor_*`, `vendor_control_xfer_cb`, etc.) that have no BT or GPIO analog.

**Proposed approach — Output Transport Abstraction Layer:**

```
┌─────────────────────────────────────────────┐
│              GPOutputTransport               │  ← NEW abstract class
│  + initialize()                              │
│  + process(Gamepad*)                         │
│  + processAux()                              │
│  + getType() → {USB, BT, GPIO}              │
└──────────┬──────────────┬───────────────────┘
           │              │              │
    ┌──────▼──────┐  ┌───▼────┐  ┌─────▼──────┐
    │  GPDriver   │  │BTDriver│  │GPIOOutDriver│
    │ (existing,  │  │(new)   │  │(new)        │
    │  USB-only)  │  │        │  │             │
    └─────────────┘  └────────┘  └─────────────┘
```

Alternatively, a simpler approach: keep `GPDriver` but add BT and GPIO as parallel tracks managed by a new `OutputManager` that can fan-out to multiple active transports simultaneously.

**Required new components for Bluetooth HID:**

1. **CMake linkage:** Add `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device` to `target_link_libraries` — but only for PicoW builds (conditional on `PICO_BOARD == pico_w`).
2. **BTHIDDriver class:** Implements a new transport interface that calls `hid_device_send_interrupt_message()` (BTstack API) instead of `tud_hid_report()`.
3. **CYW43 initialization:** `cyw43_arch_init()` before `tud_init()` in `gp2040.cpp::setup()`. Mutex-safe since WiFi already uses CYW43 in `lwip-port`.
4. **BT/WiFi coexistence:** CYW43 internally time-multiplexes BT and WiFi. SDK manages this, but users may experience throughput trade-offs when both are active.
5. **Pairing / bonding state:** BT HID requires a bonding mechanism (stored keys). Would need a new storage section in the flash protobuf config.
6. **BT HID Report Descriptor:** Can reuse the existing `hid_report_descriptor` from `HIDDriver.cpp` or define a BT-specific one.

**Required new components for GPIO output to retro consoles (e.g., SNES output):**

1. **New GPIO output driver:** A `GPIOOutputDriver` that drives clock/latch/data lines to emulate a controller. Protocol is the inverse of `SNESpadInput` — instead of reading bits shifted in by the console's clock, the MCU must respond to the console's latch and clock signals with the correct bit stream.
2. **PIO or interrupt-driven:** SNES protocol requires precise sub-microsecond timing. Best implemented in RP2040/RP2350 PIO (Programmable I/O) rather than busy-waiting. A PIO state machine for SNES output already conceptually exists as the inverse of the read path.
3. **Separate from USB:** GPIO output is independent of USB state. A board could simultaneously output via USB HID AND drive a SNES controller port if enough GPIOs are available.

### Hardware Constraints Summary

| Output Mode | Board Required | Can Coexist With USB? | Notes |
|------------|---------------|----------------------|-------|
| USB HID | Any board | — (is USB) | Works today |
| BT HID | PicoW (CYW43) | ✅ Yes | Different hardware (CYW43 vs USB PHY) |
| GPIO → SNES | Any board | ✅ Yes | Pure GPIO, no USB interaction |
| GPIO → N64 | Any board | ✅ Yes | Single-wire async serial, PIO recommended |
| GPIO → Dreamcast | Any board | ✅ Yes | MapleBus — complex, PIO strongly recommended |

**RP2040 USB constraint:** Single USB PHY. Can only be USB device OR USB host at one time via hardware. PIO-USB (second port, `CFG_TUH_RPI_PIO_USB 1`) works around this by using PIO for the host port. USB device mode (for HID output) and PIO-USB host mode (for USB passthrough) coexist today — this is not a new constraint for BT.

**CYW43 and USB simultaneous operation:** Confirmed feasible. The existing PicoW config already uses USB (device mode for HID) and WiFi (CYW43) simultaneously in web config mode. BT would be an additional CYW43 feature, not a new hardware path.

### Pico2W Build Gap

To add a `Pico2W` config:
```cmake
# configs/Pico2W/Pico2W.cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
```

Plus a `BoardConfig.h` (can clone PicoW's). The blocker is validating that CYW43 WiFi and BT initialization work correctly under the RP2350 clock configuration and that the existing `lib/lwip-port` is compatible. The base gamepad firmware will compile and run fine — the risk is exclusively in the CYW43 stack.

### Minimum Board Capability Per Output Mode

| Output Mode | Minimum Board |
|------------|---------------|
| Any USB HID mode | Any RP2040 or RP2350 board with USB |
| Bluetooth HID | PicoW (RP2040+CYW43) or Pico2W (RP2350+CYW43) |
| GPIO output (SNES/NES) | Any board with ≥3 free GPIOs + PIO state machine |
| GPIO output (N64) | Any board with ≥1 free GPIO + PIO |
| GPIO output (Dreamcast) | Any board with ≥2 free GPIOs + PIO (MapleBus complex) |

---

## Summary for Hughes

The architecture is **well-prepared for multi-output extension** but requires deliberate work:

1. **USB HID:** Fully functional, abstracted behind `GPDriver`. No changes needed.
2. **Bluetooth HID:** The hardware (CYW43) supports it, the SDK (`pico_btstack`) supports it, zero firmware code exists. A new `BTHIDDriver` class and CYW43 BT initialization are needed. USB and BT CAN run simultaneously on PicoW — they are separate hardware.
3. **GPIO retro console output:** There is NO existing retro-console output code. The retro-console support in the codebase reads FROM retro controllers (as inputs), not writes TO consoles. PIO-based state machines would be the correct implementation path.
4. **Runtime switching:** Not architecturally supported today. `DriverManager` is a boot-time selector. Adding live switching requires an `OutputManager` layer above `GPDriver`.
