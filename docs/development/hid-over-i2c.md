# HID over I2C (HoI2C) — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning / Design  
**Scope:** GP2040-CE as I2C slave presenting a standard HID interface to a host system

---

## Overview

This feature allows a GP2040-CE controller to connect to a host computer (such as a Raspberry Pi or other single-board computer) via I2C and present itself as a standard HID gamepad — without USB. The host's operating system uses its native `i2c-hid` kernel driver to enumerate the device, exactly as it would with a USB HID controller. No special software is required on the host side.

**This is the inverse of the I2C expansion bus** (`i2c-peripheral-expansion.md`), where GP2040-CE is the I2C *master* writing to satellite MCUs. Here, GP2040-CE is the I2C *slave/target*, responding to reads from the host.

### Primary Use Case

A Raspberry Pi running RetroPie, Batocera, or a custom emulation front-end has exposed I2C pins on its 40-pin GPIO header. A GP2040-CE controller connected via four wires (SDA, SCL, INT#, GND) appears to the Linux kernel as a standard HID gamepad — no USB port consumed, no USB hub needed, works at the driver level so every application that supports HID gamepads works automatically.

---

## Background: HID over I2C Protocol

**HID over I2C** is a Microsoft-standardized protocol (spec v1.0, originally introduced for Windows 8 touchpads and sensors) that carries the full USB HID protocol over an I2C bus. The spec is publicly available and has been implemented in the Linux `i2c-hid` kernel driver since kernel 3.6.

The key insight is that USB HID and HID over I2C share the same **report descriptor format** — the same binary descriptor that describes buttons, axes, and other controls to USB HID hosts is reused verbatim for I2C HID. GP2040-CE already has these descriptors (`headers/drivers/hid/HIDDescriptors.h`); HoI2C reuses them directly.

### Protocol Summary

The host (Raspberry Pi) is the I2C master. The GP2040-CE is the I2C slave at a configured address. Communication is register-based:

1. **Enumeration** (once at startup):
   - Host reads the HID Descriptor register → receives device metadata (report descriptor length, register addresses, VID/PID)
   - Host reads the Report Descriptor register → receives the full HID report descriptor
   - Host registers the device as a HID input device

2. **Normal operation** (every input event):
   - GP2040-CE asserts INT# line low when a new input report is ready
   - Host detects INT# falling edge (GPIO interrupt)
   - Host reads from the Input Register → receives the HID input report
   - Host releases INT# (GP2040-CE deasserts by releasing the line)

3. **Output reports** (optional — for LEDs, rumble):
   - Host writes to the Output Register with an HID output report
   - GP2040-CE processes the output report (rumble, player LEDs, etc.)

### Register Map

```
Register Address  Name                  Content
────────────────  ────────────────────  ──────────────────────────────────────────
0x0001            HID Descriptor        30-byte structure: lengths, register addrs,
                                        wVendorID, wProductID, wVersionID
0x0002            Report Descriptor     Full HID report descriptor (variable length)
0x0003            Input Register        Current input report (length-prefixed)
0x0004            Output Register       Output reports from host (LEDs, rumble)
0x0005            Command Register      Control commands (reset, get/set report)
0x0006            Data Register         Data for commands
```

### HID Descriptor Structure (30 bytes)

```c
typedef struct __attribute__((packed)) {
    uint16_t wHIDDescLength;      // 0x001E (30 bytes)
    uint16_t bcdVersion;          // 0x0100 (v1.0)
    uint16_t wReportDescLength;   // length of report descriptor
    uint16_t wReportDescRegister; // 0x0002
    uint16_t wInputRegister;      // 0x0003
    uint16_t wMaxInputLength;     // max input report length + 2
    uint16_t wOutputRegister;     // 0x0004
    uint16_t wMaxOutputLength;    // max output report length + 2 (0 if no output)
    uint16_t wCommandRegister;    // 0x0005
    uint16_t wDataRegister;       // 0x0006
    uint16_t wVendorID;           // USB VID (reused from USB descriptor)
    uint16_t wProductID;          // USB PID (reused from USB descriptor)
    uint16_t wVersionID;          // firmware version
    uint32_t reserved;            // 0x00000000
} HoI2C_HIDDescriptor;
```

---

## Hardware Interface

### Wiring

```
GP2040-CE board          Raspberry Pi (40-pin header)
───────────────          ───────────────────────────
SDA  (configurable) ────► GPIO 2 (pin 3) — I2C1 SDA
SCL  (configurable) ────► GPIO 3 (pin 5) — I2C1 SCL
INT# (configurable) ────► GPIO 4 (pin 7) — interrupt input
GND                 ────► GND (pin 6 or any GND pin)
```

The INT# line is **open-drain, active low** — GP2040-CE drives it low when a new report is ready, and releases it (high-Z) after the host reads the report. The Raspberry Pi (or any host) configures its GPIO as input with pull-up and triggers an interrupt on the falling edge.

**RP2040 GPIO open-drain implementation:**

The RP2040 has no true hardware open-drain mode. Use the **direction-toggle method**:
- **Assert (drive low):** `gpio_set_dir(pin, GPIO_OUT); gpio_put(pin, 0);`
- **Release (high-Z):** `gpio_set_dir(pin, GPIO_IN);` — external pull-up takes over

An alternative is to use `gpio_set_oeover()` to disable the output driver.

If INT# is not available (no spare GPIO on the board), the host can fall back to **polling mode**: read the Input Register on a timer (e.g., every 1ms). This works but adds host CPU overhead and slightly degrades latency. INT# is strongly recommended.

### Voltage Compatibility

| Host | I2C voltage | Compatible? |
|---|---|---|
| Raspberry Pi (all models) | 3.3V | ✅ Direct connection |
| Raspberry Pi Zero | 3.3V | ✅ Direct connection |
| Arduino Mega / Uno (5V) | 5V | ⚠️ Level shifter required |
| Orange Pi, Rock Pi, Radxa | 3.3V | ✅ Typically direct |
| BeagleBone Black | 3.3V | ✅ Direct connection |

### I2C Speed

| Mode | Speed | Recommendation |
|---|---|---|
| Standard Mode | 100 kHz | Compatible with all hosts, sufficient for 1ms report delivery |
| Fast Mode | 400 kHz | Recommended default |
| Fast Mode Plus | 1 MHz | Supported by RP2040/RP2350; verify host support |

At 400 kHz and a 12-byte HID report (+ 2-byte length prefix = 14 bytes), each report read takes ~350µs — well within the 1ms target.

---

## RP2040/RP2350 I2C Slave Implementation

### Pico SDK Slave Mode

**Note: RP2040 I2C slave mode requires low-level IRQ handling — there is no high-level callback API in the Pico SDK. This is expert-level embedded work.**

The Pico SDK provides `i2c_set_slave_mode()` (in `hardware/i2c.h`) to configure the I2C hardware as a slave, but it does **not** provide a high-level callback mechanism. Developers must implement a low-level IRQ handler from scratch.

**Slave mode setup:**
1. Call `i2c_set_slave_mode(i2c1, true, I2C_SLAVE_ADDR)` to configure hardware
2. Register interrupt handler: `irq_set_exclusive_handler(I2C1_IRQ, hoi2c_irq_handler)`
3. Enable interrupt: `irq_set_enabled(I2C1_IRQ, true)`
4. In the handler: manually inspect `i2c_get_hw(i2c)->raw_intr_stat` and `data_cmd` registers to determine the transaction state (read vs. write, data byte vs. register address, transaction complete)

The slave firmware implements a simple register-read state machine:
- On first write byte: treat it as the register address
- On subsequent read requests: stream bytes from the selected register's buffer
- On transaction complete: reset internal state, deassert INT# if input report was read

### Coexistence with I2C Master Addons

The RP2040/RP2350 has two I2C controllers. HoI2C **requires exclusive use of one I2C block in slave mode** — a block cannot simultaneously be a master (for display, ADS1219, PCF8575, Wii extension) and a slave.

| Block | Recommended role |
|---|---|
| `i2c0` | Master — existing addons (display, ADS1219, PCF8575, Wii extension, expansion satellites) |
| `i2c1` | Slave — HID over I2C to host |

Board configs that expose both I2C buses can use this split. Boards with only one exposed I2C bus must choose: HoI2C displaces master-mode addons on that bus, unless the master addons are moved to SPI or alternative GPIO pins.

### Firmware Architecture

```
Core0: gamepad loop
  → reads buttons → builds GamepadState
  → I2CExpansionAddon::process()     (master write to satellites, if enabled)
  → after state update: checks HoI2C output addon
       → if state changed: updates _inputReportBuffer
       → asserts INT# pin low

Core0: I2C slave IRQ (i2c1_irq_handler)
  → I2C_SLAVE_RECEIVE: receive register address
  → I2C_SLAVE_REQUEST: stream bytes from _inputReportBuffer (or descriptor)
  → I2C_SLAVE_FINISH:  deassert INT# after input report read
```

The IRQ fires on the I2C bus event — no polling needed. The shared state between the gamepad loop and the IRQ handler (`_inputReportBuffer`) must be protected to avoid tearing.

**Synchronization with critical_section_t:**

The RP2040 lacks native C11 atomic primitives for this use case. Use Pico SDK's `critical_section_t` (disables/re-enables IRQs — appropriate for Core0/IRQ shared state):

```c
critical_section_t report_lock;
critical_section_init(&report_lock);

// In IRQ handler (read):
critical_section_enter_blocking(&report_lock);
// ... read buffer ...
critical_section_exit(&report_lock);

// In Core0 gamepad loop (write):
critical_section_enter_blocking(&report_lock);
// ... write buffer ...
critical_section_exit(&report_lock);
```

Alternatively, use `spin_lock_t` (hardware spinlock, IRQ-safe) if multi-core coordination is needed.

---

## Host-Side Setup

### Linux: `i2c-hid` Kernel Module

The Linux `i2c-hid` kernel module (`i2c-hid.ko`, available since kernel 3.6, standard in all modern distros) handles the full HoI2C protocol. When the device is properly described to the kernel, it appears as `/dev/input/eventX` — indistinguishable from a USB HID gamepad.

The kernel needs to know the device exists via either **Device Tree** (Raspberry Pi / embedded) or **ACPI** (x86 systems). For Raspberry Pi, a device tree overlay is used.

### Raspberry Pi Device Tree Overlay

Create `/boot/overlays/gp2040-hoi2c.dts`:

```dts
/dts-v1/;
/plugin/;

/ {
    compatible = "brcm,bcm2835";

    fragment@0 {
        target = <&i2c1>;
        __overlay__ {
            #address-cells = <1>;
            #size-cells = <0>;
            status = "okay";

            gp2040_gamepad: gamepad@20 {
                compatible = "hid-over-i2c";           /* matches i2c-hid.ko kernel module */
                reg = <0x20>;                          /* I2C address */
                hid-descr-addr = <0x0001>;             /* HID descriptor register */
                interrupt-parent = <&gpio>;
                interrupts = <4 8>;                    /* GPIO4, IRQ_TYPE_LEVEL_LOW */
            };
        };
    };
};
```

Compile and install:
```bash
dtc -@ -I dts -O dtb -o gp2040-hoi2c.dtbo gp2040-hoi2c.dts
sudo cp gp2040-hoi2c.dtbo /boot/overlays/
```

Add to `/boot/config.txt`:
```
dtoverlay=gp2040-hoi2c
```

After reboot, `dmesg | grep i2c-hid` should show the device being enumerated, and `/dev/input/eventX` appears for use with jstest, sdl2, RetroArch, etc.

### Verification

```bash
# Verify I2C device is visible
i2cdetect -y 1    # should show 0x20 (or configured address)

# Verify HID device
ls /dev/input/
cat /proc/bus/input/devices    # should list the gamepad

# Test input
sudo jstest /dev/input/js0
```

---

## Addon Integration

### New Addon: `HoI2CAddon`

```cpp
class HoI2CAddon : public GPAddon {
public:
    bool available() override;
    void setup() override;       // init i2c slave, INT# GPIO, descriptor buffers
    void preprocess() override {}
    void process() override;     // update input report buffer if state changed; assert INT#
    void postprocess(bool) override {}
    void reinit() override;
    std::string name() override { return "HoI2C"; }
private:
    void buildInputReport(const GamepadState& state);
    void assertInterrupt();
    void deassertInterrupt();

    uint8_t _hidDescBuffer[30];
    uint8_t _reportDescBuffer[256];   // reuse existing HID report descriptor
    uint8_t _inputReportBuffer[16];   // length-prefixed report
    uint8_t _outputReportBuffer[16];
    uint8_t _activeRegister;
    uint8_t _byteIndex;
    int _intPin;
    GamepadState _lastState;
};
```

### Protobuf Config

```protobuf
message HoI2COptions {
    optional bool enabled = 1;
    optional int32 i2c_block = 2;       // must be a block not used by master addons; default 1 (i2c1)
    optional int32 sda_pin = 3;
    optional int32 scl_pin = 4;
    optional uint32 speed = 5;          // default 400000 (Fast Mode)
    optional int32 int_pin = 6;         // INT# GPIO; -1 to disable (polling fallback)
    optional uint32 address = 7;        // I2C slave address; default 0x20
    optional bool polling_mode = 8;     // true = no INT# line, host polls
}
```

---

## HID Report Descriptor Reuse

GP2040-CE already maintains HID report descriptors for USB HID mode in `headers/drivers/hid/HIDDescriptors.h`. The HoI2C addon reuses **the same report descriptor bytes** verbatim — the format is identical between USB HID and HID over I2C. This means:

- The host sees the same button/axis layout over I2C as it would over USB
- The input report format is identical — the same `GamepadState` → report serialization used by `HIDDriver.cpp` is reused
- VID/PID from the USB descriptor are reused in the HoI2C HID Descriptor, so the host identifies the device consistently regardless of connection method

---

## Latency Characteristics

| Component | Time |
|---|---|
| GPIO → firmware debounce → report ready | same as USB path (~0–5ms depending on debounce) |
| INT# assertion → host GPIO IRQ latency | < 1µs (hardware interrupt) |
| Host kernel IRQ handler → I2C read | ~50–200µs (Linux IRQ latency) |
| I2C read (14 bytes @ 400 kHz) | ~350µs |
| Host kernel → application (evdev) | ~100–500µs |
| **Total (0 debounce)** | **~0.5–1ms** |

HoI2C latency is expected to be comparable to USB HID latency on Linux hosts. The dominant variable is Linux kernel IRQ latency, which varies with system load. On a dedicated RetroPie/emulation system with a real-time kernel patch, IRQ latency can be reduced to < 50µs.

---

## Scope and Limitations

### In Scope

- HID over I2C in **device/slave** mode (GP2040-CE responds to host reads)
- XInput-equivalent and generic HID report formats over I2C
- Raspberry Pi and Linux-based SBC integration with device tree overlay
- Coexistence with USB output (both active simultaneously on different interfaces)
- INT# interrupt line for low-latency host notification

### Out of Scope

- **Windows / ACPI hosts**: Windows x86 systems use ACPI to describe I2C HID devices; this requires BIOS/UEFI firmware changes and is not practical for a general-purpose controller addon
- **macOS**: macOS does not expose a general-purpose I2C bus for external devices in the same way as Linux SBCs
- **I2C host enumeration without device tree**: Raspberry Pi OS Lite, older kernels, or bare-metal systems without `i2c-hid` support require manual setup beyond this feature's scope

### Relationship to Other Output Features

| Feature | Direction | GP2040-CE role | Transport |
|---|---|---|---|
| USB HID | GP2040-CE → host | USB device | USB |
| HoI2C (this doc) | GP2040-CE → host | I2C slave | I2C |
| I2C expansion bus | GP2040-CE → satellite | I2C master | I2C |
| BLE HID (HOGP) | GP2040-CE → host | BLE peripheral | BLE |
| BT Classic HID | GP2040-CE → host | BT device | BT Classic |

HoI2C and I2C expansion can coexist on the same board using separate I2C blocks (HoI2C on i2c1 in slave mode, expansion on i2c0 in master mode).

---

## Implementation Plan

### Phase 1 — Core Slave Implementation

1. Implement `HoI2CAddon` in `src/addons/hoi2c.cpp` / `headers/addons/hoi2c.h`
2. Implement I2C slave IRQ handler with register-read state machine
3. Wire up INT# GPIO assertion on state change
4. Add `HoI2COptions` to `proto/config.proto` `AddonOptions`
5. Add web configurator UI: enable toggle, I2C block, SDA/SCL/INT# pin selectors, address, speed

### Phase 2 — Host Integration

1. Publish Raspberry Pi device tree overlay (`tools/hoi2c/gp2040-hoi2c.dts`)
2. Publish setup guide (`docs/expansion/hoi2c-raspberry-pi.md`): wiring, overlay install, verification steps
3. Test with RetroPie, Batocera, Lakka (major emulation front-ends)
4. Test with `sdl2` and `evdev` directly

### Phase 3 — Output Reports and Extended Features

1. Handle HID output reports from host (player LEDs, rumble forwarding)
2. Implement `RESET_DEVICE` command from HoI2C Command Register
3. Evaluate support for multiple report IDs (if `HoI2COptions` selects a multi-report-ID descriptor)

---

## Acceptance Criteria

- [ ] GP2040-CE enumerates as HID gamepad on Raspberry Pi via `i2c-hid` driver without USB connected
- [ ] `/dev/input/eventX` appears and responds correctly to `jstest`
- [ ] All buttons and axes map correctly to the HID report descriptor
- [ ] INT# line asserts on every new report and deasserts after host read
- [ ] HoI2C and I2C expansion bus coexist on the same board (i2c0 master + i2c1 slave)
- [ ] USB output remains functional simultaneously with HoI2C active
- [ ] Device tree overlay published, tested, and documented
- [ ] Latency measured with the latency test rig (see `latency-testing-framework.md`)

---

## Open Questions

1. **USB + HoI2C simultaneously**: When both USB and HoI2C are active, the controller reports to two hosts at the same time. Should there be a mode where HoI2C is only active when USB is disconnected? Or always active in parallel?

2. **Report descriptor selection**: The USB HID report descriptor (generic HID mode) is straightforward to reuse. XInput uses a different, vendor-specific descriptor. Should HoI2C always use the generic HID descriptor, or should it follow the configured USB mode?

3. **I2C address conflicts**: The default address `0x20` may conflict with other I2C devices on the host bus (e.g., PCF8574 GPIO expanders, common at `0x20`–`0x27`). The address should be user-configurable and the web UI should document common conflicts.

4. **Polling fallback without INT#**: Not all board designs will have a free GPIO for INT#. The polling fallback (host polls at 1ms interval) should be documented clearly and the device tree overlay should include a no-INT# variant.

5. **Buffer safety**: The `_inputReportBuffer` is written by the gamepad loop (Core0) and read by the I2C slave IRQ (also Core0 in the current architecture). Since both run on Core0, the IRQ handler preempts the gamepad loop — a partial write to the buffer is possible. Use `critical_section_t` or a double-buffer swap to ensure atomicity; this must be in the initial implementation, not deferred.
