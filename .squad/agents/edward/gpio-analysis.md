# GPIO Retro Console OUTPUT — Technical Survey

**Author:** Edward (Firmware Developer)  
**Date:** 2026-03-28  
**Purpose:** Deep technical survey for Hughes's feature doc on GPIO output mode — emulating retro console controllers.  
**Requested by:** Fortinbra  

---

## 1. Existing Retro INPUT Adapter Code — Protocol Reverse Engineering

### 1.1 SNESpadInput / NES Protocol (from `lib/SNESpad/SNESpad.cpp`)

**Pins:** 3 — CLOCK (MCU output), LATCH (MCU output), DATA (MCU input, active-low, pull-up)

**Protocol role:** The CONSOLE is master. It drives CLOCK and LATCH. The controller responds with data.

**Timing extracted from `SNESpad::latch()` and `SNESpad::clock()`:**

| Signal | Timing |
|--------|--------|
| Latch HIGH pulse | 12 µs |
| Latch LOW → first clock | 6 µs setup |
| Clock LOW (sample phase) | 6 µs |
| Clock HIGH (recovery) | 6 µs |
| Total clock period | 12 µs (~83 kHz) |
| Extra byte gap (SNES mouse) | 12 µs delay |

**Frame structure:**
- **NES:** 8 bits, active-low (button pressed = 0), sequence: A, B, Select, Start, Up, Down, Left, Right
- **SNES:** 16 bits (12 buttons + 4 device-ID bits). Bits 12–15 are device ID: `0b0000` = SNES gamepad, `0b1000` = SNES mouse. Mouse reads 32 bits total.
- Button-pressed = low (logic 0); released = high (logic 1)
- After bit 15: if MSB was 1 (mouse indicator), 16 more bits follow

**For OUTPUT mode (what must change):**
- MCU flips roles: DATA pin becomes OUTPUT (driven by MCU, read by console)
- LATCH and CLOCK become INPUTS (console drives them)
- MCU must respond to the falling edge of CLOCK by holding the correct bit on DATA
- LATCH edge loads the shift register; each subsequent CLOCK shifts one bit out
- Timing tolerance is loose (~±2 µs) — bit-bang is feasible but an interrupt-driven approach is safer
- PIO option: use `gpio_set_irq_enabled_with_callback` for LATCH/CLOCK edges, or a PIO SM that watches LATCH then clocks out bits in sync

**Voltage:** 5V TTL. RP2040/RP2350 GPIO outputs are 3.3V — **level shifting required.**

### 1.2 TG16padInput / PC Engine Protocol (from `src/addons/tg16_input.cpp`)

**Pins:** 6 — OE (MCU output, active-low), SELECT (MCU output), DATA0–DATA3 (4 MCU inputs, active-low, pull-up)

**Protocol role in INPUT mode:** MCU is master (drives OE and SELECT), controller is passive.

**For OUTPUT mode (what must change):**
- DATA0–DATA3 become MCU OUTPUTS (driven by MCU to report button state)
- OE and SELECT become MCU INPUTS (console drives them)
- MCU presents the correct 4-bit nibble on DATA0–DATA3 depending on SELECT state
- Timing: console's SELECT transitions are slow (µs–ms scale) — bit-bang feasible

**Bit layout (from OUTPUT perspective):**
- SELECT=HIGH: present nibble {I, II, Select, Run} (active-low)
- SELECT=LOW: present nibble {Up, Right, Down, Left} (active-low)
- For 6-button: third SELECT-HIGH cycle presents {III, IV, V, VI}

**Voltage:** 5V TTL. **Level shifting required.**

### 1.3 Wii Extension Protocol (`src/addons/wiiext.cpp`)

Not directly applicable to retro OUTPUT; uses I2C (not a native console port). Noted for completeness only.

### 1.4 Genesis/Mega Drive Protocol (inferred from `configs/ReflexCtrlGenesis6/BoardConfig.h`)

**Pins:** 9-pin DE-9 connector. Signal lines: Up, Down, Left, Right, A (SELECT=LOW), B, C (SELECT=HIGH), Start, plus SELECT input from console.

**Protocol:** Fully parallel + SELECT mux. Console drives SELECT at poll rate (~60 Hz).
- SELECT=LOW: data lines carry {Up, Down, Low=0, Low=0, A, Start} (6 lines active)
- SELECT=HIGH: data lines carry {Up, Down, Left, Right, B, C}
- 6-button extension: additional SELECT pulses expose X, Y, Z, Mode

**For OUTPUT mode:**
- MCU drives 4 direction + up to 4 button data lines (open-collector with pull-up, or push-pull)
- Console drives SELECT; MCU reads SELECT level and updates output accordingly
- No precise timing needed — SELECT transitions are slow (~µs); GPIO IRQ or polling is fine

**Pin count:** 6 signal lines minimum (3-button mode), 8 for 6-button mode

**Voltage:** 5V TTL. **Level shifting required.**

---

## 2. RP2040/RP2350 GPIO Capabilities

### 2.1 Available GPIO Count

| Chip | Total GPIO | Notes |
|------|-----------|-------|
| RP2040 | 30 (0–29) | GPIO 23, 24, 25 reserved on standard Pico; 29 = ADC/VBUS |
| RP2350A | 30 (0–29) | Same bank0 footprint as RP2040 |
| RP2350B | 48 (0–47) | QFN-80 package; 18 additional GPIO in bank0 |

The firmware already handles this correctly: `isValidPin()` in `headers/helper.h:38–40` uses `NUM_BANK0_GPIOS` from the SDK — returns 30 or 48 at compile time. No hardcoding.

### 2.2 GPIO Voltage Levels — The 3.3V vs 5V Problem

| Parameter | Value | Implication |
|-----------|-------|-------------|
| RP2040/RP2350 GPIO output high | 3.3V | Underdrive for 5V TTL consoles |
| 5V TTL VIH minimum | 2.0–2.4V | 3.3V output **may** work for some consoles |
| RP2040/RP2350 GPIO input tolerance | 3.3V max (not 5V tolerant) | **Cannot connect 5V logic directly to GPIO input** |
| Dreamcast MAPLE bus | 3.3V | **No level shifting needed** |
| N64 data line | 3.3V | **No level shifting needed** |
| NES/SNES/Genesis/TG16 | 5V | **Level shifting required both directions** |

**Critical point:** RP2040/RP2350 GPIO inputs are NOT 5V tolerant. Connecting a 5V console directly to a GPIO input pin will damage the MCU. This is the most important hardware constraint for the feature.

### 2.3 Level Shifting — Standard Community Approach

The standard solution is a **74AHCT125** quad 3-state buffer/level shifter:
- Input: 3.3V from MCU GPIO → Output: 5V for console (unidirectional, output mode)
- For input direction: use **74LVC1T45** single-channel bidirectional shifter, or resistor voltage dividers

For a bidirectional bus (like N64's single-wire or a shared DATA line):
- **74LVC245** or **74AHCT245** octal bus transceiver with direction control
- Community Pico-to-N64/SNES adapters universally use level shifters

A custom retro-output board would require one 74AHCT125 (or equivalent) per console connector, or a single quad/octal shifter for a multi-pin protocol.

### 2.4 PIO (Programmable I/O) Subsystem

**Resource inventory:**

| Resource | RP2040/RP2350A | RP2350B |
|---------|----------------|---------|
| PIO blocks | 2 (PIO0, PIO1) | 3 (PIO0, PIO1, PIO2) |
| State machines per block | 4 | 4 |
| Total state machines | 8 | 12 |
| Instruction memory per block | 32 instructions | 32 instructions |

**Current PIO consumption in GP2040-CE:**

| Use | PIO Block | SM(s) | Source |
|-----|-----------|--------|--------|
| PIO-USB host TX | PIO0 (default) | SM0 | `lib/pico_pio_usb/src/pio_usb_configuration.h:28` |
| PIO-USB host RX | PIO1 (default) | SM0, SM1 | `lib/pico_pio_usb/src/pio_usb_configuration.h:32–34` |
| WS2812 NeoPixel LEDs | PIO0 | SM0 | `src/addons/neopicoleds.cpp:282` |
| **Available** | PIO0 | SM1, SM2, SM3 | (if USB passthrough + LEDs both active) |
| **Available** | PIO1 | SM2, SM3 | |

**Important:** PIO-USB is only active when `USB_PERIPHERAL_ENABLED` is defined in a board's `BoardConfig.h` (e.g., `ReflexCtrlSaturn`). WS2812 is only active when LEDs are configured. A dedicated retro-output board may have neither, leaving all 8 SMs available.

**PIO capabilities relevant to retro output:**
- Precise clock generation down to single-cycle resolution (at 125 MHz: 8 ns per cycle)
- IRQ-driven handoff between Core0 and PIO
- GPIO pin ownership by PIO (exclusive access, no CPU overhead once running)
- Hardware FIFO (4×32-bit words, joinable to 8×32-bit)
- A SNES output PIO program needs ~8 instructions; N64 output needs ~15–20

---

## 3. Dreamcast VMU / Controller Protocol (MAPLE Bus)

### 3.1 What Is in the Codebase

**Zero Dreamcast-related code exists.** Full-text search across `src/`, `headers/`, `lib/`, and `configs/` for `dreamcast`, `maple`, `VMU`, `SDCKA`, `SDCKB` returns no results in project source files. (References exist only in `.squad/` team documentation as scope notes.)

### 3.2 MAPLE Bus Protocol — Technical Summary

Dreamcast uses a proprietary **MAPLE bus** protocol. Key characteristics:

| Parameter | Value |
|-----------|-------|
| Physical wires | 2 signal lines: SDCKA and SDCKB |
| Logic level | **3.3V** (no level shifting needed for RP2040/RP2350) |
| Data rate | ~2 Mbit/s burst |
| Topology | Star bus; host (Dreamcast) has 4 ports, each with 2 signal lines |
| Direction | Half-duplex — host sends command frame, peripheral responds |
| Encoding | NRZ-like: transitions encode clock and data together |

**Physical connector:** MapleBus uses a custom 5-wire connector: 5V (power), GND, SDCKA, SDCKB, and a 5V pin for accessories. The signal pins operate at 3.3V even though power is 5V.

**Protocol complexity:**
- Each frame starts with a start pattern (SDCKA/SDCKB both HIGH then a specific transition sequence)
- Frame header encodes: command, recipient address, sender address, payload length
- CRC check (8-bit XOR) on every frame
- Peripheral must respond within a tight window (~150–250 µs) after receiving a valid command frame
- The Dreamcast polls each port at ~60 Hz

**Bidirectional complexity:** Unlike SNES (where the controller just shifts bits on a host-controlled clock), MAPLE requires the peripheral to:
1. Detect and parse the incoming command frame
2. Validate the CRC
3. Assemble a response frame
4. Transmit the response within the timing window

This is significantly more complex than SNES or N64. **PIO is strongly recommended — not just for timing, but for frame assembly/disassembly.** A PIO state machine would handle the physical encoding/decoding; an IRQ handler would feed it frame data from Core0 or Core1.

**Community implementations:** MAPLE bus implementations targeting RP2040 exist in the open-source community (e.g., using two PIO SMs — one for TX, one for RX). The RP2040's 3.3V GPIO eliminates the level-shifting problem entirely. This makes Dreamcast actually one of the more achievable targets from a hardware perspective, despite protocol complexity.

---

## 4. Output Architecture — What Needs to Be Built

### 4.1 GPIOOutputAddon — Architecture Proposal

The correct implementation vehicle is a new **`GPAddon` subclass**, NOT a new `GPDriver`. Here's why:

- `GPDriver` is USB-centric. Its interface (`get_descriptor_*`, `vendor_control_xfer_cb`, etc.) has no GPIO analog.
- GPIO output can and should **coexist with USB output**. The user wants to play a retro console while the board is still recognized as a USB device.
- `GPAddon::process()` already runs after `inputDriver->process(gamepad)` in the main loop, which means GPIO output fires after USB output each frame — exactly correct.

**Proposed class skeleton:**

```cpp
// headers/addons/gpio_output.h
class GPIOOutputAddon : public GPAddon {
public:
    bool available() override;   // check GPIOOutputOptions.enabled + valid pins
    void setup() override;       // init GPIO pins, load PIO programs
    void process() override;     // read gamepad->state, update output pins/PIO FIFO
    void preprocess() override {}
    void postprocess(bool) override {}
    void reinit() override;
    std::string name() override { return GPIOOutputName; }

private:
    ConsoleProtocol protocol;    // SNES, NES, N64, GENESIS, TG16, DREAMCAST
    // pin assignments from GPIOOutputOptions protobuf
};
```

**New protobuf fields needed (`proto/config.proto`):**

```proto
message GPIOOutputOptions {
    bool enabled = 1;
    ConsoleProtocol protocol = 2;  // new enum
    int32 dataPin = 3;
    int32 clockPin = 4;   // SNES/NES only
    int32 latchPin = 5;   // SNES/NES only
    int32 selectPin = 6;  // TG16, Genesis
    int32 oePin = 7;      // TG16 only
    // ... additional pins per protocol
}
```

A new `ConsoleProtocol` enum and new `INPUT_MODE_*` aliases may not be needed — the protocol is selected within the addon, not at the `InputMode` level. This is a deliberate design choice: it keeps USB mode and GPIO output mode as **independent axes of configuration**.

### 4.2 Mode Selection Strategy

**Recommended approach:** Add `GPIOOutputOptions` as a new section in the existing `AddonOptions` protobuf message (alongside `SNESOptions`, `TG16Options`, etc.). Enable/disable via the web configurator like any other addon. No new `InputMode` enum value required.

**Alternative: boot-time button hold.** Map a button combo (e.g., hold B3+B4 at boot) to enable GPIO output mode. Uses the existing boot-action mapping infrastructure in `gp2040.cpp:129–195`. Simpler but less user-friendly for permanent configurations.

**Hot-switch at runtime:** Not currently architecturally supported (DriverManager is boot-time only). For GPIO output, this doesn't matter — the addon can be enabled/disabled at runtime through the web configurator writing to flash, followed by a reboot. This is the same mechanism all other addons use.

### 4.3 USB + GPIO Simultaneous Operation

**Yes, simultaneous operation is feasible and the recommended design.** GPIO output runs as a `GPAddon` that reads `gamepad->state` after it's been updated by USB processing. The USB report and the GPIO signals reflect the same gamepad state at effectively the same time (one main loop iteration, ~1 ms).

**PIO and USB coexistence:** PIO state machines operate completely independently of the USB stack. PIO-USB host (when active) uses its own PIO block configuration. A GPIO output PIO program on a different SM (or even a different PIO block) runs in parallel with no interference.

### 4.4 Pin Assignment Strategy Across Boards

**For a generic/Pico configuration** (Pico, Pico2, Pico W, Pico 2 W):

The standard Pico button layout uses GPIO 2–21 for buttons/functions, with GPIO 0–1 for I2C display and GPIO 28 for LEDs. The following GPIO pins are typically **not assigned** to buttons and are candidates for retro output:

| GPIO | Standard Pico Use | Available for Retro Output? |
|------|-------------------|----------------------------|
| 22 | Unassigned | ✅ Yes |
| 23 | SMPS power supply mode (internal) | ⚠️ Usable but affects power regulation |
| 24 | VBUS detect | ⚠️ Read-only sense pin |
| 25 | Onboard LED | ✅ Yes (if LED not needed) |
| 26 | Unassigned (ADC0) | ✅ Yes |
| 27 | Unassigned (ADC1) | ✅ Yes |
| 28 | NeoPixel LED data | ✅ Yes (if LED not configured) |
| 29 | ADC VBUS measurement | ⚠️ Usable but affects voltage sensing |

**Practical conclusion:** A retro-output board would be a dedicated hardware design with its own `BoardConfig.h` that explicitly allocates pins for the console connector and marks them `ASSIGNED_TO_ADDON`. Users should not repurpose standard button GPIO pins for retro output on a fighting game stick.

---

## 5. Console Compatibility Matrix

| Console | Protocol Type | Pins Needed | Timing Criticality | RP2040 Voltage | Level Shift? |
|---------|--------------|-------------|-------------------|----------------|-------------|
| **NES** | Synchronous serial, 8-bit shift register | 3 (DATA, CLOCK, LATCH) | Moderate (6–12 µs edges) — IRQ or PIO recommended | 5V | ✅ Required |
| **SNES** | Synchronous serial, 16/32-bit shift register | 3 (DATA, CLOCK, LATCH) | Moderate (6–12 µs edges) — IRQ or PIO recommended | 5V | ✅ Required |
| **N64** | Single-wire half-duplex serial (NRZ 1 MHz) | 1 (DATA bidirectional) + pull-up | **High — PIO mandatory** | 3.3V | ✅ Not needed |
| **Dreamcast** | MAPLE bus, 2-wire half-duplex ~2 Mbps | 2 (SDCKA, SDCKB) | **Very high — PIO mandatory** | 3.3V | ✅ Not needed |
| **Genesis/Mega Drive** | Parallel + SELECT mux | 7–9 (4 dir + 3–6 btn + SELECT) | Low (SELECT at ~60 Hz) — bit-bang feasible | 5V | ✅ Required |
| **TurboGrafx-16 / PC Engine** | Parallel + OE + SELECT + 4 data | 6 (OE, SELECT, DATA0–3) | Low (SELECT pulses ~ms) — bit-bang feasible | 5V | ✅ Required |

### Protocol Detail Notes

**NES/SNES:** The console drives CLOCK and LATCH. For output, MCU must detect LATCH rising edge (load shift register with current button state) then shift one bit onto DATA on each CLOCK falling edge. The timing window per bit is 12 µs (total frame ~200 µs). An interrupt-driven approach works; PIO provides jitter-free response. Existing `SNESpad` library is the inverse of what's needed — its timing constants are directly reusable.

**N64:** Uses a custom NRZ serial protocol at 1 MHz (1 µs per bit). 0 = 3µs LOW + 1µs HIGH; 1 = 1µs LOW + 3µs HIGH. Commands are 9 bytes from console; response is up to 4 bytes from controller. Absolute tolerance ±500 ns per bit. **Bit-banging is impossible — PIO is mandatory.** Community "pico64" and similar projects have published working PIO programs for this. The 3.3V logic level matches RP2040 natively — no level shifting.

**Dreamcast (MAPLE):** Protocol described in Section 3. PIO mandatory. Two SMs needed (TX + RX). Higher implementation complexity than any other console. 3.3V native.

**Genesis/Mega Drive:** Console reads 9-pin DE-9 directly. No shift register. MCU drives data lines continuously; console reads at its own rate. SELECT changes every 60 Hz frame. MCU monitors SELECT with a GPIO IRQ or polls it. Fully compatible with bit-bang; PIO adds nothing here.

**TurboGrafx-16:** Similar parallel approach. Slightly more complex due to 6-button detection sequence. OE is active-low enable. Timing tolerances are loose (ms-scale). Bit-bang is completely sufficient.

### Existing Community Implementations (Reference Only)

| Console | Community Approach | Notes |
|---------|--------------------|-------|
| N64 | PIO state machine (NRZ protocol) | Multiple open implementations on RP2040; PIO code ~15–20 instructions |
| SNES | Interrupt-driven GPIO or PIO | Projects target original hardware; both approaches verified |
| Dreamcast | Dual-PIO (TX+RX) MAPLE bus | Fewer projects; most complex; 3.3V native is an advantage |
| Genesis | Bit-bang GPIO + SELECT IRQ | Simple enough no PIO projects needed |
| NES | Same as SNES (subset) | 8-bit protocol is simpler |
| TG16 | Bit-bang GPIO | Trivial implementation |

---

## 6. Board-Specific Considerations

### 6.1 Which Boards Can Support GPIO Output

Any board with enough free GPIO pins after button assignment can support retro output. Minimum requirements:

| Console | Min Free GPIOs | Additional HW |
|---------|---------------|---------------|
| NES | 3 | Level shifter (74AHCT125 or equivalent) |
| SNES | 3 | Level shifter |
| N64 | 1 + PIO SM | None (3.3V native) |
| Dreamcast | 2 + 2 PIO SMs | None (3.3V native) |
| Genesis (3-btn) | 7 | Level shifter |
| Genesis (6-btn) | 9 | Level shifter |
| TG16 (2-btn) | 6 | Level shifter |
| TG16 (6-btn) | 6 | Level shifter |

**Boards with sufficient GPIO headroom:**
- `Pico` / `Pico2`: GPIO 22–29 (minus reserved) → ~5–7 free pins. Adequate for NES/SNES/N64/TG16/Dreamcast. Genesis 6-button is tight.
- `PicoW` / `Pico2W` (future): Same physical GPIO exposure, but GPIO 23–25 reserved. See section 6.2.
- `RP2040AdvancedBreakoutBoard`: Large breakout — many free pins. Good candidate.
- `ReflexCtrlSNES`, `ReflexCtrlNES`, etc.: Currently input-only. Repurposing for output would require hardware changes (data direction flip + level shifting).

**Boards with insufficient GPIO headroom:**
- Small form factor boards (SeeedXIAORP2040, WaveshareZero): most GPIO exposed as buttons; fewer than 3 free. Not suitable for Genesis output without hardware changes.

### 6.2 Pico W CYW43 GPIO Conflicts

The CYW43439 wireless chip on the Pico W connects to RP2040 via dedicated GPIO:

| GPIO | Pico W Function | Impact on Retro Output |
|------|----------------|----------------------|
| GPIO 23 | `WL_ON` — CYW43 SMPS control | **Reserved — must not be used** |
| GPIO 24 | `SDIO_CLK` / SPI CLK to CYW43 | **Reserved — must not be used** |
| GPIO 25 | `WL_D` / `LED` (multiplexed via CYW43) | **Reserved — must not be used** |
| GPIO 29 | `ADC_VREF` / VSYS sense | Shared, typically reserved |

**On the standard Pico (non-W):** GPIO 23 (`SMPS_MODE`) and 24 (`VBUS_SENSE`) are internal connections but typically available as weak digital I/O if the user accepts the side effects. GPIO 25 is the onboard LED — usable if the LED is not needed. **These pins are safe to repurpose on non-W Pico boards.** On Pico W, GPIO 23–25 are definitively off-limits for user GPIO due to CYW43 SPI/SDIO bus.

**Net effect on PicoW:** GPIO 22, 26, 27, 28 are the primary candidates for retro output. That's 4 pins — sufficient for NES/SNES/N64/Dreamcast/TG16 but tight for Genesis.

### 6.3 Existing Board Config GPIO Reservations

Reviewing `configs/*/BoardConfig.h`, the following GPIO pins are reserved system-wide:

| GPIO | Common Usage | Boards |
|------|-------------|--------|
| GPIO 0–1 | I2C0 (OLED display: SDA/SCL) | Pico, Pico2, most boards |
| GPIO 14 | Turbo button | Pico, Pico2 |
| GPIO 15 | Turbo LED | Pico, Pico2 |
| GPIO 28 | NeoPixel LED data pin | Pico, Pico2 |
| GPIO 14 | USB passthrough D+ | ReflexCtrlSaturn |

Pins marked `ASSIGNED_TO_ADDON` in a `BoardConfig.h` are claimed by other addons and should not be reassigned to GPIO output without modifying the board config.

### 6.4 PIO State Machine Budget for Retro Output Boards

For a dedicated retro-output board (no USB passthrough, optional LEDs):

| Feature | PIO SMs Consumed |
|---------|-----------------|
| PIO-USB host (passthrough disabled) | 0 |
| WS2812 NeoPixels (if configured) | 1 (PIO0 SM0) |
| SNES/NES output (PIO approach) | 1 SM |
| N64 output | 1 SM |
| Dreamcast MAPLE output | 2 SMs |
| **Total worst case (DC + N64 + LEDs)** | **4 SMs** |
| **Available on RP2040** | **8 SMs total** |

Even the most demanding combination leaves 4 spare SMs. This is not a bottleneck.

---

## 7. Summary: What Needs to Be Built

### Minimum Viable Implementation (SNES/NES output)

1. **New `GPIOOutputAddon` class** (`src/addons/gpio_output.cpp`, `headers/addons/gpio_output.h`)
   - Extends `GPAddon`
   - `process()` reads `gamepad->state` and responds to LATCH/CLOCK edges
   - Interrupt-driven (GPIO IRQ) or PIO-assisted

2. **New `GPIOOutputOptions` protobuf message** in `proto/config.proto`
   - `enabled`, `protocol` (enum), pin assignments
   - Wired into `AddonOptions` alongside `SNESOptions`

3. **Level shifting hardware** on the board (external component, not firmware)
   - Required for all 5V consoles (NES, SNES, Genesis, TG16)
   - Not required for N64 or Dreamcast

4. **Web configurator extension** — new addon settings panel (out of scope for Edward; web team task)

### PIO State Machine Programs (per console)

| Console | New PIO Program Needed? | Complexity |
|---------|------------------------|-----------|
| NES/SNES output | Optional (IRQ approach is viable) | Low |
| N64 output | **Mandatory** | Medium |
| Dreamcast output | **Mandatory** | High |
| Genesis output | No | Trivial |
| TG16 output | No | Low |

### Architecture Constraints for Hughes's Doc

1. **GPIO output is a `GPAddon`, not a `GPDriver`** — enables USB + GPIO simultaneous operation by design.
2. **USB mode selection is independent of GPIO output** — the user selects an HID USB profile separately from the retro console output protocol.
3. **Level shifting hardware is mandatory for NES, SNES, Genesis, TG16** — this is a hardware design constraint, not a firmware one. Documentation must state this clearly.
4. **N64 and Dreamcast are 3.3V native** — no level shifting needed; simpler hardware design.
5. **Dreamcast is significantly more complex than other consoles** — bidirectional MAPLE bus with CRC and frame protocol. Should be documented as a future/advanced target.
6. **PIO state machines are not a bottleneck** — even a full-featured implementation uses at most 4 of 8 available SMs.
7. **GPIO output and PIO-USB passthrough can conflict** — a board with USB passthrough enabled uses 3 PIO SMs. If simultaneously running N64 (1 SM) + Dreamcast (2 SMs) + WS2812 (1 SM), all 8 SMs would be consumed. Board configs enabling all features simultaneously would need to explicitly manage PIO resource allocation.
8. **Pico W loses GPIO 23–25 to CYW43** — reduces free GPIO headroom from ~7 to ~4 pins for retro output.
9. **Mode activation via addon enable flag in web config** — no new `InputMode` enum value required; does not require a reboot beyond saving addon settings (same as all existing addons).
10. **RP2350B's 48 GPIO pins** make it the ideal chip for multi-console output boards.
