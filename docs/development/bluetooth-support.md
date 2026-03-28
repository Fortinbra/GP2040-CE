# Bluetooth HID Support — Feature Planning

**Last updated:** 2026-03-28  
**Maintained by:** GP2040-CE core team  
**Status:** Planning / Not yet implemented

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
- **Today:** Raspberry Pi Pico W (RP2040 + CYW43 wireless chip)
- **Future:** Raspberry Pi Pico 2 W (RP2350 + CYW43) — pending CYW43 stack porting to RP2350
- All other boards retain USB-only output

**Out of scope for this document:** GPIO output to retro consoles (SNES, N64, Dreamcast, Genesis, etc.). GP2040-CE reads FROM retro controllers via GPIO input add-ons (SNESpadInput, TG16padInput) but does not emit GPIO signals to emulate controllers for retro consoles. Retro console output is a separate architectural concern not addressed here and will be covered in a future document.

---

## Supported Hardware

### Wireless-Capable Boards

| Board | Chip | Wireless Hardware | Status |
|-------|------|-------------------|--------|
| Pico W | RP2040 + CYW43439 | WiFi + Bluetooth HID | ✅ Target |
| Pico 2 W | RP2350 + CYW43439 | WiFi + Bluetooth HID | 🟡 Blocked (CYW43 porting) |
| All other boards | RP2040 / RP2350 | None | USB only |

### Minimum Requirements

**For Bluetooth HID support:**
- Pico SDK version: **2.2.0 or later** (enforced in CMakeLists.txt)
- Pico W board with CYW43439 chip
- Available GPIO and flash storage for bonding keys
- CMake target linkage: `pico_cyw43_arch_lwip_threadsafe_background`, `pico_btstack_cyw43`, `pico_btstack_hid_device`

**Note on Pico 2 W:** The hardware supports Bluetooth, but as of Pico SDK 2.2.0, CYW43 integration with RP2350 has not been validated by the Raspberry Pi Foundation. A new `configs/Pico2W/` configuration will be added once CYW43 wireless stack support for RP2350 is confirmed.

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

### Pico 2 W Support Blocked

**Status:** Not available; blocked pending CYW43 wireless stack porting to RP2350

The Pico 2 W hardware (RP2350 + CYW43) exists and is supported by Pico SDK 2.2.0 at the board definition level. However, the CYW43 wireless initialization stack (`cyw43_arch_init`, CYW43 clock configuration) has not been validated for RP2350's clock speeds and PIO timing.

Once Raspberry Pi Foundation or the open-source community validates CYW43 for RP2350, a new `configs/Pico2W/` configuration can be added in ~1 day.

**Workaround:** Use Pico W (RP2040 variant) for Bluetooth support until Pico 2 W becomes available.

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

> **Note:** `rp2350-support.md` references "Bluetooth HID" in the context of the Pico 2 W blocker. As of this writing, Bluetooth HID is **not yet implemented** on any GP2040-CE board, including Pico W. `rp2350-support.md` was written anticipating this feature; this document is the authoritative planning reference for Bluetooth HID.

- **[RP2350 Support](./rp2350-support.md)** — Chip and board configuration details
- **[Dependency Updates](./dependency-updates.md)** — Pico SDK and library version notes
- **[DDI-SOCD Documentation](./ddi-socd.md)** — Input processing architecture (no BT changes needed)

---

## References

- [Raspberry Pi Pico W Datasheet](https://datasheets.raspberrypi.org/pico_w/pico_w_datasheet.pdf)
- [Pico SDK BTstack Documentation](https://www.raspberrypi.org/documentation/pico-sdk/networking.html#pico_btstack)
- [Bluetooth HID Device Profile Specification](https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=311735)
- [Bluetooth Core Specification 5.3](https://www.bluetooth.org/docman/handlers/downloaddoc.ashx?doc_id=570290)

