# I2C Peripheral Expansion Bus — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning / Design  
**Scope:** I2C master output, standardized gamepad state protocol, satellite MCU support

---

## Overview

GP2040-CE already uses I2C as a master to **read from** peripheral devices — the ADS1219 ADC (`i2canalog1219` addon), the PCF8575 GPIO expander (`i2c_gpio_pcf8575` addon), and Wii extension controllers (`wiiext` addon) all follow this pattern.

This feature extends that infrastructure in the **opposite direction**: GP2040-CE acts as I2C master and **writes** the current gamepad state to one or more satellite MCUs (I2C targets/slaves). Each satellite MCU implements whatever output logic it needs — a retro controller protocol, a custom display, a rumble driver, a second-screen UI, or any peripheral that requires electronics or timing that cannot be directly handled by the RP2040/RP2350.

This creates a standardized expansion bus for add-on hardware that the core firmware never needs to know about. A satellite for N64, SNES, PS1, or any future protocol is a self-contained device that receives the same standardized packet every frame.

---

## Output Ecosystem Context

**USB HID is and will remain the primary output target** for GP2040-CE. All other output methods are optional, additive features that operate alongside USB rather than replacing it.

The full output ecosystem, across current and planned features:

| Output Channel | Status | Doc |
|---|---|---|
| USB HID (XInput, HID, PS4, Switch, etc.) | ✅ Shipping | — (current implementation) |
| I2C expansion bus (this doc) | 📋 Planned | `i2c-peripheral-expansion.md` |
| BLE HID (HOGP over GATT) | 📋 Planned | `bluetooth-support.md` |
| BT Classic HID | 🔧 In progress | `feature/bluetooth-hid` branch |
| HID over I2C (slave mode, for SBCs) | 📋 Planned | `hid-over-i2c.md` |
| WiFi web config (station mode) | 📋 Planned | `wifi-web-config.md` |
| Retro console direct output (N64, SNES, etc.) | 📋 Planned | via I2C expansion satellites |

The I2C expansion bus described in this document is the **wired peripheral extension layer** — it allows the RP2040/RP2350 to delegate complex or timing-critical output hardware to dedicated satellite MCUs over a simple two-wire bus. It is complementary to, not a replacement for, USB output.

Note: **HID over I2C** (GP2040-CE acting as I2C *slave* presenting a HID interface to a host like a Raspberry Pi) is a distinct feature described in `hid-over-i2c.md`. This document covers the opposite direction: GP2040-CE as I2C *master* writing to satellites.

---

## Motivation

### Problems This Solves

**Retro console outputs** (N64, SNES, NES, PS1/PS2, Saturn, etc.) require precise bit-level timing that is difficult or impossible alongside USB HID in a single-core context. The RP2040 PIO can help, but dedicating PIO state machines to retro protocols conflicts with other features. A satellite MCU dedicated entirely to one output protocol eliminates the conflict.

**Complex displays** that require continuous refresh cycles (large OLED matrices, RGB panels, e-ink, TFT with touch input) can consume significant CPU time and I2C bandwidth. Offloading the display pipeline to a satellite MCU frees the main RP chip for gamepad loop work.

**Output-side add-ons** with specialized hardware requirements — force feedback motors, analog output DACs, load cells, contactless sensors — may require driver circuits and operating voltages that don't belong on the main board. A satellite MCU on its own small PCB can handle those requirements.

**Firmware modularity**: without this feature, every new output peripheral requires modifying the GP2040-CE firmware and recompiling. With an I2C expansion bus, new peripherals only require satellite firmware — the main firmware change is a one-time addon registration.

---

## Architecture

### System Diagram

```
┌─────────────────────────────────────────────────────┐
│  RP2040 / RP2350 (GP2040-CE main firmware)           │
│                                                      │
│  Gamepad loop (gp2040.cpp)                           │
│    → reads GPIO buttons                              │
│    → builds GamepadState                             │
│    → runs addons (process())                         │
│       └─ I2CExpansionAddon::process()                │
│             → serializes GamepadState                │
│             → i2c_write_blocking() to each target    │
│                                                      │
│  i2c0  SDA ──────────────────────────────────────►  │
│        SCL ──────────────────────────────────────►  │
└─────────────────────────────────────────────────────┘
                        │ (I2C bus, 400 kHz)
          ┌─────────────┼─────────────┐
          │             │             │
          ▼             ▼             ▼
  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐
  │  Satellite 0  │ │  Satellite 1  │ │  Satellite 2  │
  │  addr: 0x20  │ │  addr: 0x21  │ │  addr: 0x22  │
  │              │ │              │ │              │
  │  N64 output  │ │  SNES output │ │  128×64 OLED │
  │  (RP2040 +   │ │  (ATtiny85)  │ │  (RP2040 +   │
  │   PIO 1-wire)│ │              │ │   custom UI) │
  └──────────────┘ └──────────────┘ └──────────────┘
```

### Integration with Existing Infrastructure

The existing `PeripheralManager` (`headers/peripheralmanager.h`) already manages `i2c0` and `i2c1` as shared buses with arbitration. The new addon uses `PMGR.getI2C(block)` exactly as existing I2C addons do, ensuring no bus conflicts with other addons (ADS1219, PCF8575, display, Wii extension).

The new addon fits the standard `GPAddon` interface (`headers/gpaddon.h`):

```cpp
class I2CExpansionAddon : public GPAddon {
public:
    bool available() override;
    void setup() override;
    void preprocess() override {}
    void process() override;      // sends GamepadState to all configured targets
    void postprocess(bool) override {}
    void reinit() override;
    std::string name() override { return "I2CExpansion"; }
private:
    void sendState(uint8_t address, const GamepadState& state);
    bool stateChanged(const GamepadState& a, const GamepadState& b);
    GamepadState _lastState;
};
```

---

## Wire Protocol

### Design Goals

- **Fixed size**: satellite firmware can pre-allocate a buffer and use a simple receive loop
- **Self-describing**: magic byte and version field allow satellites to reject packets from incompatible firmware versions
- **Checksummed**: detect wire errors without requiring I2C SMBUS PEC (which not all MCUs support)
- **Little-endian**: consistent with RP2040 native byte order, simplifying serialization
- **Complete**: carries the full `GamepadState` so no satellite needs to know anything about the main firmware's input processing

### Packet Format — `GP2040_I2C_STATE_PACKET_V1`

```
Offset  Size  Type      Field           Description
──────  ────  ────────  ──────────────  ──────────────────────────────────────
0       1     uint8_t   magic           0xA5 — identifies GP2040-CE packet
1       1     uint8_t   version         0x01 — protocol version
2       1     uint8_t   dpad            GAMEPAD_MASK_UP/DOWN/LEFT/RIGHT bits
3       1     uint8_t   dpad_original   pre-SOCD-clean dpad value
4       4     uint32_t  buttons         GAMEPAD_MASK_B1..E12 button bitmask
8       2     uint16_t  lx              Left stick X (0x0000–0xFFFF, mid=0x7FFF)
10      2     uint16_t  ly              Left stick Y
12      2     uint16_t  rx              Right stick X
14      2     uint16_t  ry              Right stick Y
16      1     uint8_t   lt              Left trigger (0x00–0xFF)
17      1     uint8_t   rt              Right trigger
18      2     uint16_t  aux             AUX_MASK_FUNCTION and other aux flags
20      1     uint8_t   checksum        XOR of bytes 0–19 (magic through aux[1])
────────────────────────────────────────────────────────────────────────────
Total: 21 bytes
```

**Note:** This packet serializes output gamepad state fields only. Internal signal smoothing state (such as float EMA fields: `ema_1_x`, `ema_1_y`, etc.) is excluded from transmission — satellites receive only the finalized button/axis values.

#### Button Bitmask Reference

The `buttons` field uses GP2040-CE's internal button numbering, documented in `headers/gamepad/GamepadState.h`:

| Bit | Mask Name | XInput | Switch | PS |
|---|---|---|---|---|
| 0 | `GAMEPAD_MASK_B1` | A | B | Cross |
| 1 | `GAMEPAD_MASK_B2` | B | A | Circle |
| 2 | `GAMEPAD_MASK_B3` | X | Y | Square |
| 3 | `GAMEPAD_MASK_B4` | Y | X | Triangle |
| 4 | `GAMEPAD_MASK_L1` | LB | L | L1 |
| 5 | `GAMEPAD_MASK_R1` | RB | R | R1 |
| 6 | `GAMEPAD_MASK_L2` | LT | ZL | L2 |
| 7 | `GAMEPAD_MASK_R2` | RT | ZR | R2 |
| 8 | `GAMEPAD_MASK_S1` | Back | − | Select |
| 9 | `GAMEPAD_MASK_S2` | Start | + | Start |
| 10 | `GAMEPAD_MASK_L3` | LS | LS | L3 |
| 11 | `GAMEPAD_MASK_R3` | RS | RS | R3 |
| 12 | `GAMEPAD_MASK_A1` | Guide | Home | — |
| 13 | `GAMEPAD_MASK_A2` | — | Capture | — |
| 20–31 | `GAMEPAD_MASK_E1`–`E12` | Extra buttons | | |

Satellite firmware that implements a specific console output simply maps the relevant bits to that console's button assignments.

#### Timing

At 400 kHz I2C (Fast Mode):
- Each byte costs ~22.5 µs (9 bit-times including ACK)
- 21-byte payload + I2C start/address/stop overhead ≈ **500 µs per satellite**
- Two satellites on one bus: ~1 ms — at the limit of the 1 ms USB frame budget

For builds with multiple satellites, the addon should use **change detection**: skip the I2C write if `GamepadState` has not changed since the last transmission. In practice, during idle frames (no input) the bus is silent, and the write only occurs on active frames where a button is pressed or an axis moves. Multiple satellites are only a throughput concern during continuous axis input (joystick movement).

If multiple satellites require guaranteed per-frame updates regardless of state change, **i2c1** can be used for a second bus, distributing satellites across two physical buses for parallel transmission.

---

## Addon Configuration

### Protobuf Schema Addition

`proto/config.proto` already defines `I2COptions` (used by `PeripheralOptions.blockI2C0` / `blockI2C1`) with `enabled`, `sda`, `scl`, and `speed` fields. The new addon options embed this pattern rather than duplicating it:

```protobuf
message I2CExpansionOptions {
    optional bool enabled = 1;
    optional int32 i2c_block = 2;              // 0 = i2c0, 1 = i2c1
    optional int32 sda_pin = 3;
    optional int32 scl_pin = 4;
    optional uint32 speed = 5;                 // I2C clock in Hz, default 400000
    repeated uint32 target_addresses = 6 [(nanopb).max_count = 8];
    optional bool change_detect = 7;           // only send on state change (default true)
}
```

This message is added to `AddonOptions` at the next available field number. Existing field numbers in `AddonOptions` run up through field 31 — the next available field is **32**.

### Web Configurator UI

A new "I2C Expansion" section in the Add-Ons page:
- Enable/disable toggle
- I2C block selector (i2c0 / i2c1)
- SDA / SCL pin dropdowns (same pattern as existing I2C addons)
- Speed dropdown (100 kHz Standard, 400 kHz Fast, 1 MHz Fast+)
- Target address list (add/remove address entries, 0x00–0x7F)
- Change detection toggle
- "Scan Bus" button — runs `PeripheralManager::scanForI2CDevice()` and populates detected addresses

---

## Satellite MCU Reference Implementations

### Recommended Satellite Hardware

| MCU | Cost | GPIO | Reason |
|---|---|---|---|
| **RP2040 Pico** | $4 | 26 usable | PIO for timing-critical protocols (N64, SNES); TinyUSB available if USB output also needed |
| **RP2350 Pico 2** | $5 | 26 usable | Same as above, more SRAM, faster — overkill for simple protocols |
| **ATtiny85** | $1–2 | 5 usable | Minimal; good for single-output simple protocols with limited GPIO needs |
| **Arduino Nano (ATmega328P)** | $3–5 | 18 usable | Familiar development environment; Wire library makes I2C target easy |
| **STM32G0 series** | $1–3 | 18–37 | Fast, cheap, hardware I2C target support |

The **RP2040 is the recommended satellite MCU** for complex protocols. Its PIO subsystem is ideal for implementing console-specific 1-wire or serial protocols (N64, SNES, NES) that require cycle-accurate bit timing independent of interrupt latency.

### Satellite Firmware Pattern (Arduino/C pseudocode)

```c
#include <Wire.h>  // or equivalent I2C library

#define GP2040_MAGIC    0xA5
#define GP2040_VERSION  0x01
#define PACKET_SIZE     21
#define MY_ADDRESS      0x20

typedef struct {
    uint8_t  magic;
    uint8_t  version;
    uint8_t  dpad;
    uint8_t  dpad_original;
    uint32_t buttons;
    uint16_t lx, ly, rx, ry;
    uint8_t  lt, rt;
    uint16_t aux;
    uint8_t  checksum;
} __attribute__((packed)) GP2040Packet;

volatile GP2040Packet packet;
volatile bool newData = false;

void onReceive(int count) {
    if (count != PACKET_SIZE) { /* drain and discard */ return; }
    Wire.readBytes((uint8_t*)&packet, PACKET_SIZE);

    uint8_t xor = 0;
    for (int i = 0; i < PACKET_SIZE - 1; i++) xor ^= ((uint8_t*)&packet)[i];
    if (xor != packet.checksum) return;  // bad packet
    if (packet.magic != GP2040_MAGIC) return;
    if (packet.version != GP2040_VERSION) return;

    newData = true;
}

void setup() {
    Wire.begin(MY_ADDRESS);
    Wire.onReceive(onReceive);
    // initialize console output hardware
}

void loop() {
    if (newData) {
        newData = false;
        updateConsoleOutput(&packet);  // implement protocol-specific output here
    }
}
```

### Example: N64 Controller Satellite (RP2040)

```
RP2040 satellite GPIO map:
  GP0 = I2C SDA (from main board)
  GP1 = I2C SCL (from main board)
  GP2 = N64 DATA line (1-wire, open-drain, 3.3V logic)
  GP3 = N64 3.3V power sense (optional)

PIO program handles:
  - Waiting for N64 console poll command (0x01)
  - Responding with 4-byte N64 controller state packet
  - Bit timing: 1µs low / 3µs high = 0 bit; 3µs low / 1µs high = 1 bit
```

The satellite's PIO state machine runs the N64 protocol continuously. When a new I2C packet arrives from the main board, the satellite updates the N64 state buffer that the PIO reads. The PIO handles console timing independently of the I2C receive handler — no contention.

### Example: SNES Controller Satellite (ATtiny85 or RP2040)

```
Satellite GPIO map:
  CLOCK  (input from SNES console)
  LATCH  (input from SNES console)
  DATA   (output to SNES console, active-low serial)
  SDA    (from GP2040-CE main board)
  SCL    (from GP2040-CE main board)

Protocol: on LATCH rising edge, shift 16 bits out on DATA on each CLOCK
rising edge. Button bit order defined by SNES spec.
```

SNES protocol is simple enough for an ATtiny85 interrupt handler without PIO. The satellite maps the GP2040-CE `buttons` bitmask bits to SNES button positions and shifts them out.

---

## Addressing and Bus Topology

### Default Address Assignments

To allow multiple satellite types on the same bus without configuration:

| Address | Reserved For |
|---|---|
| `0x20` | Satellite slot 0 (generic / primary retro output) |
| `0x21` | Satellite slot 1 |
| `0x22` | Satellite slot 2 |
| `0x23` | Satellite slot 3 |
| `0x60`–`0x6F` | Display satellites (mirrors existing display I2C range) |

These are defaults only. All addresses are configurable via the web UI. The web UI "Scan Bus" feature will detect any responding device regardless of address.

### Multi-Bus Configuration

The RP2040/RP2350 has two I2C controllers. Boards that expose both can run two independent expansion buses:

| Bus | Pins | Use case |
|---|---|---|
| `i2c0` | Configurable (e.g., GP4/GP5) | Retro output satellites |
| `i2c1` | Configurable (e.g., GP6/GP7) | Display satellites or second retro output |

Existing addons (display, ADS1219, PCF8575) may already occupy one bus. The expansion addon's pin configuration must not conflict with pins already claimed by `PeripheralManager`.

---

## Timing and Latency Impact

### Write Budget per Frame

At 1ms USB polling (current default):

| Scenario | Write time | Remaining budget |
|---|---|---|
| 1 satellite, 400 kHz | ~530 µs | ~470 µs |
| 2 satellites, 400 kHz, sequential | ~1.06 ms | over budget |
| 2 satellites, 1 per bus, parallel | ~530 µs | ~470 µs |
| 1 satellite, 1 MHz Fast+ | ~215 µs | ~785 µs |
| Change-detect, no state change | 0 µs | 1 ms |

**Recommendation**: Enable change detection by default. For multiple satellites on the same bus, use 1 MHz Fast+ mode (RP2040 and most modern MCUs support it). For more than two satellites, distribute across both I2C buses.

### Latency from Press to Satellite Update

```
GPIO press → debounce → gamepad loop → I2CExpansionAddon::process() → i2c_write_blocking()
→ satellite receives packet → satellite updates output
```

Total satellite update latency ≈ USB frame latency (1ms) + I2C write time (~500µs) = **~1.5ms** from button press to satellite state update. This is acceptable for retro console output; N64 and SNES console polling rates are 60Hz (16.7ms between polls), so the satellite state is always fresh well within one console poll cycle.

---

## Implementation Plan

### Phase 1 — Core Protocol and Addon

1. Define `GP2040_I2C_STATE_PACKET_V1` struct in a new header (`headers/interfaces/i2c/gp2040_expansion/gp2040_expansion_protocol.h`)
2. Implement `I2CExpansionAddon` in `src/addons/i2c_expansion.cpp` / `headers/addons/i2c_expansion.h`
3. Register addon in `src/gp2040.cpp` Core0 addon block (lines 107–127, alongside other input/output addons)
4. Add `I2CExpansionOptions` to `proto/config.proto` `AddonOptions` message
5. Add web configurator UI section for the addon settings

> **Note:** This addon introduces the first retro console **output** adapter category in GP2040-CE. All existing retro-related addons (`snes_input`, `tg16_input`) are **input** adapters that read from retro controllers. This feature enables the reverse: outputting GP2040-CE gamepad state to hardware that drives retro console ports.

### Phase 2 — Reference Satellite Firmware

1. Publish RP2040 satellite firmware template (`tools/i2c-satellite/pico/`)
   - I2C target receive handler
   - Packet validation and checksum
   - Generic button/axis state buffer
   - Example N64 PIO output
2. Publish ATtiny85 / Arduino satellite template (`tools/i2c-satellite/arduino/`)
   - Wire library I2C target
   - Example SNES output
3. Publish satellite hardware BOM and wiring guide (`docs/expansion/satellite-hardware.md`)

### Phase 3 — Extended Protocol

1. Bidirectional communication: allow satellites to send data back to the main board (e.g., haptic feedback request, satellite status, button inputs from satellite-side hardware)
2. Protocol version negotiation: main board reads satellite version register before first write to confirm compatibility
3. Broadcast mode: single write to address `0x00` (I2C general call) to update all satellites simultaneously — eliminates per-satellite write overhead at the cost of no per-satellite addressing

---

## Acceptance Criteria

- [ ] Protocol spec finalized and versioned (v1 frozen before any satellite firmware ships)
- [ ] `I2CExpansionAddon` builds and integrates without conflicts with existing I2C addons
- [ ] Web UI allows configuring SDA/SCL pins, bus, speed, and target address list
- [ ] Change-detection mode verified: no I2C traffic on idle frames
- [ ] Reference RP2040 satellite firmware receives and validates packets correctly
- [ ] Reference ATtiny85/Arduino satellite template published and documented
- [ ] Latency from button press to satellite state update measured and documented (expected: ~1.5ms)
- [ ] Coexistence with display addon, ADS1219, PCF8575 on shared I2C bus verified (no bus conflicts)

---

## Open Questions

1. **Protocol versioning strategy**: Should version negotiation be a prerequisite for the first write, or can the satellite simply ignore packets with an unknown version byte? The latter is simpler but could cause a satellite running old firmware to silently fail on a new main board.

2. **Bidirectional data**: Phase 3 calls for satellites to send data back. This requires the main board to issue I2C read transactions in addition to writes. Should the main board poll satellites on a configurable interval, or should satellites signal readiness via an interrupt pin (IRQ)?

3. **Broadcast address**: I2C general call (address 0x00) would allow updating all satellites in a single transaction. However, some chips use 0x00 for other purposes and it may cause unintended responses from non-satellite I2C devices on the bus. This needs validation against the full list of devices that may share the bus.

4. **Bus pull-up resistors**: I2C requires pull-ups on SDA and SCL. `lib/PicoPeripherals/peripheral_i2c.cpp` already calls `gpio_pull_up()` on both lines during `setup()` — the software pull-ups are always active. For short on-board traces they are sufficient. For external cables or satellite boards connected via header, add 4.7kΩ external pull-ups to 3.3V on the main board and disable software pull-ups on the satellite end. For runs longer than ~30cm at 400 kHz, reduce to 2.2kΩ.

5. **Level shifting**: All GP2040-CE boards operate at 3.3V logic. Satellites at 5V (e.g., Arduino Uno, ATmega328 running at 5V) require a level shifter on SDA/SCL. Recommended: TXS0102 or BSS138 open-drain level shifter. 3.3V-native satellites (ATtiny85 running at 3.3V, RP2040) need no level shifter.
