# GPIO Retro Console Output — Feature Planning

**Last updated:** 2026-03-28  
**Maintained by:** GP2040-CE core team

---

## Overview

GP2040-CE currently supports **controller input from retro consoles** via specialized input adapters. This document outlines support for the **opposite direction**: the GP2040-CE board acting as a **controller for retro consoles via GPIO pins**.

GPIO output mode allows a user to connect their GP2040-CE board directly to a supported retro console (NES, SNES, Genesis, N64, TurboGrafx-16, or Dreamcast) and control games on that hardware while maintaining **full USB functionality simultaneously**. The board acts as a standard gamepad to modern computers while simultaneously presenting itself as a native controller to retro hardware.

### Scope

**In Scope:**
- GPIO-based output to: NES, SNES, N64, Dreamcast, Genesis/Mega Drive (3-button and 6-button), TurboGrafx-16 / PC Engine
- USB HID output remains fully functional alongside GPIO output — they are independent, coexisting output modes
- Firmware-level architecture, GPIO pin assignment, protobuf configuration, addon integration, and implementation roadmap

**Out of Scope:**
- Bluetooth output (covered separately in `docs/development/bluetooth-support.md`)
- USB output changes
- Web configurator UI implementation (responsibility of the web team)
- Physical hardware designs / board schematics (reference architectures only)

---

## Console Compatibility Matrix

| **Console** | **Protocol Type** | **Pins Needed** | **Timing** | **Voltage** | **Level Shift?** | **PIO Required?** | **Complexity** |
|---|---|---|---|---|---|---|---|
| **NES** | Synchronous 8-bit shift | 3 (DATA, CLOCK, LATCH) | Moderate (6–12 µs) | 5V | ✅ Yes | Optional | Low |
| **SNES** | Synchronous 16-bit shift | 3 (DATA, CLOCK, LATCH) | Moderate (6–12 µs) | 5V | ✅ Yes | Optional | Low |
| **Genesis/Mega Drive** | Parallel + SELECT mux | 7–9 (4 dir + 3–6 btn + SELECT) | Loose (~60 Hz polling) | 5V | ✅ Yes | No | Low–Medium |
| **TurboGrafx-16 / PC Engine** | Parallel (OE + SELECT + 4 data) | 6 (OE, SELECT, DATA0–3) | Loose (ms-scale) | 5V | ✅ Yes | No | Low |
| **N64** | Single-wire NRZ serial (1 MHz) | 1 (DATA bidirectional) | **High (±500 ns tolerance)** | 3.3V | ✗ No | ✅ **Mandatory** | Medium |
| **Dreamcast (MAPLE)** | 2-wire half-duplex (~2 Mbps) | 2 (SDCKA, SDCKB) | **Very high** | 3.3V | ✗ No | ✅ **Mandatory** | **High** |

### Protocol Notes

**NES/SNES:** Console drives `CLOCK` and `LATCH`. Board detects `LATCH` rising edge to load button state into a shift register, then responds on each `CLOCK` falling edge by presenting the correct bit on `DATA` (active-low). Timing window per bit: ~12 µs. Existing `SNESpad` input library timing constants are directly reusable for output in reverse.

**Genesis/Mega Drive:** Fully parallel output on dedicated data lines. Console drives `SELECT` at ~60 Hz frame rate to multiplex between two button groups (3-button: SELECT=LOW reads directions + A/Start; SELECT=HIGH reads B/C/unused; 6-button: additional SELECT pulses expose X/Y/Z/Mode). Board monitors `SELECT` with a GPIO IRQ or polls it. No shift register involved. Timing tolerances are very loose.

**TurboGrafx-16 / PC Engine:** 6-pin connector: `OE` (output enable, active-low), `SELECT`, and 4 data lines. Similar to Genesis—parallel data lines with `SELECT` transitions determining which button nibble is presented. `OE` gates the output. Timing tolerances loose (ms-scale).

**N64:** Custom NRZ serial protocol at 1 MHz. Console sends 9-byte command frames; board responds with 4-byte data frames. Bit encoding: `0` = 3 µs LOW + 1 µs HIGH; `1` = 1 µs LOW + 3 µs HIGH. **Absolute tolerance: ±500 ns per bit.** Jitter-free timing is impossible without PIO. 3.3V native—no level shifting.

**Dreamcast (MAPLE Bus):** Proprietary 2-wire protocol at ~2 Mbps half-duplex. Host (Dreamcast) sends command frame; board must parse it, validate 8-bit CRC, and respond within a ~200 µs window. Response frame format encodes function data, CRC, and control bits. **Bidirectional and complex.** 3.3V native. Community implementations use dual-PIO state machines (one for TX, one for RX). No existing GP2040-CE code to reference.

---

## Hardware Requirements

### Level Shifting — The Critical 3.3V ↔ 5V Problem

**Critical hardware constraint:** RP2040 and RP2350 GPIO pins output 3.3V and are **NOT 5V tolerant on inputs**. Connecting a 5V console directly to GPIO will damage the MCU.

#### For 5V Consoles (NES, SNES, Genesis, TG16)

**Voltage levels:**
- RP2040/RP2350 GPIO output: 3.3V
- 5V TTL VIH minimum: 2.0–2.4V
- 3.3V output may marginally work with some 5V-tolerant inputs, but is unreliable

**Standard solution:** Use a **74AHCT125** quad 3-state buffer for unidirectional level shifting (3.3V → 5V):
- Connect MCU GPIO → 74AHCT125 input
- Connect 74AHCT125 output → console data pin
- Power 74AHCT125 from 5V (board must supply 5V rail)
- Common in community Pico-to-SNES/NES adapter designs

**For bidirectional buses** (if needed in future): Use **74LVC245** octal bus transceiver or **74LVC1T45** single-channel shifters with direction control.

#### For 3.3V Consoles (N64, Dreamcast)

**No level shifting required.** N64 and Dreamcast both operate natively at 3.3V, matching RP2040/RP2350 GPIO output levels directly. This simplifies board design significantly.

### Voltage Tolerances & Input Safety

| Parameter | Value | Implication |
|---|---|---|
| RP2040/RP2350 GPIO input max | 3.3V | **Cannot accept 5V input** |
| N64 signal voltage | 3.3V | ✅ Safe; no conversion needed |
| Dreamcast MAPLE voltage | 3.3V | ✅ Safe; no conversion needed |
| NES/SNES/Genesis/TG16 voltage | 5V | ⚠️ **Must use level shifter** |

### Recommended Board-Specific Hardware

**For dedicated 5V retro output boards:**
- One **74AHCT125** (or equivalent quad buffer) per console connector, or a shared octal shifter for multi-console designs
- 5V power supply rail (often available from USB VBUS, but confirm current budget)
- Standard pull-up resistors on GPIO inputs if the console does not provide them

**For 3.3V native consoles (N64, Dreamcast):**
- Direct GPIO connections; no additional shifter hardware

---

## Board-Specific Pin Availability

### Standard Pico / Pico 2 (RP2040 / RP2350A)

Available GPIO pins after standard button assignments (GPIO 2–21):

| GPIO | Standard Use | Retro Output Candidate? | Notes |
|---|---|---|---|
| GPIO 22 | Unassigned | ✅ **Yes** | Primary candidate |
| GPIO 23 | SMPS mode (internal) | ⚠️ Usable but affects power supply regulation |
| GPIO 24 | VBUS detect (input only) | ⚠️ Read-only sense pin |
| GPIO 25 | Onboard LED | ✅ **Yes** (if LED not used) |
| GPIO 26 | Unassigned (ADC0) | ✅ **Yes** | Good for analog input, but GPIO capable |
| GPIO 27 | Unassigned (ADC1) | ✅ **Yes** | Same as GPIO 26 |
| GPIO 28 | NeoPixel LED data | ✅ **Yes** (if LEDs not configured) |
| GPIO 29 | ADC VREF / VSYS sense | ⚠️ Usable but impacts voltage sensing |

**Practical free GPIO:** 5–7 pins on standard Pico if avoiding risky pins. **Sufficient for NES, SNES, N64, TG16, Dreamcast. Genesis 6-button is tight (needs 9 pins).**

### Pico W / Pico 2 W (RP2040 + CYW43 / RP2350A + CYW43)

CYW43 wireless chip reserves GPIO 23–25 for SPI/SDIO bus to the WiFi module:

| GPIO | CYW43 Function | Retro Output Usable? |
|---|---|---|
| GPIO 23 | `WL_ON` (SMPS control) | ✗ **Reserved** |
| GPIO 24 | `SDIO_CLK` (SPI clock) | ✗ **Reserved** |
| GPIO 25 | LED / SPI data | ✗ **Reserved** |

**Practical free GPIO on Pico W:** GPIO 22, 26, 27, 28 = **4 pins**. **Sufficient for NES, SNES, N64, TG16, Dreamcast. Genesis unsuitable without redesign.**

### RP2350B (48 GPIO Variant)

GPIO 0–47 available. Existing buttons use GPIO 0–21; GPIO 30–47 are additional free pins beyond RP2040 compatibility. **RP2350B is the best choice for multi-console dedicated output boards.**

### Pin Conflict Avoidance

**Reserved system-wide (all boards):**
- GPIO 0–1: I2C0 (OLED display) — do not reassign
- GPIO 14–15: Turbo button and turbo LED (Pico/Pico 2) — check your board config before reuse
- GPIO 28: NeoPixel LED data if WS2812 addon is configured

**When creating a dedicated retro-output board config:**
- Create new `BoardConfig.h` in `configs/YourBoardName/`
- Explicitly mark console GPIO pins as `ASSIGNED_TO_ADDON` in the config
- Document which addon (GPIOOutput) claims which pins
- Leave standard button GPIO unmolested unless retrofitting existing designs

---

## Architecture & Implementation Pattern

### GPIOOutputAddon — The Recommended Vehicle

GPIO output is implemented as a new **`GPAddon` subclass** (not a `GPDriver`), following the same pattern as existing addons like `NeoPicoLEDs`, `TG16Input`, and others.

**Why `GPAddon` and not `GPDriver`?**
- `GPDriver` is USB-centric; its interface (descriptor callbacks, vendor control, etc.) has no GPIO analog
- **USB and GPIO can coexist simultaneously** — a `GPAddon` runs in the main loop after `inputDriver->process(gamepad)`, reading the current gamepad state and updating GPIO pins in lockstep with USB HID reports
- This design is the cleanest enablement of simultaneous USB + GPIO output

### Class Skeleton

```cpp
// headers/addons/gpio_output.h
class GPIOOutputAddon : public GPAddon {
public:
    bool available() override;      // Check enabled + valid pin config
    void setup() override;          // Initialize GPIO, load PIO programs if needed
    void process() override;        // Read gamepad->state, update output pins/FIFO
    void preprocess() override {}   // No action
    void postprocess(bool) override {} // No action
    void reinit() override;         // Reinitialize on config reload
    std::string name() override { return GPIOOutputName; }

private:
    ConsoleProtocol protocol;       // Enum: SNES, NES, N64, GENESIS, TG16, DREAMCAST
    uint8_t dataPins[MAX_PINS];     // Protocol-specific pin assignments
    uint8_t numPins;
    // ... protocol-specific state (shift register, FIFO pointer, etc.)
};
```

### Protobuf Configuration

New message in `proto/config.proto`:

```proto
enum ConsoleProtocol {
    PROTOCOL_UNSET = 0;
    PROTOCOL_SNES = 1;
    PROTOCOL_NES = 2;
    PROTOCOL_N64 = 3;
    PROTOCOL_GENESIS_3BUTTON = 4;
    PROTOCOL_GENESIS_6BUTTON = 5;
    PROTOCOL_TG16 = 6;
    PROTOCOL_DREAMCAST_MAPLE = 7;
}

message GPIOOutputOptions {
    bool enabled = 1;
    ConsoleProtocol protocol = 2;
    
    // Common pins (most protocols use a subset)
    int32 dataPin = 3;              // SNES/NES/N64: data line
    int32 clockPin = 4;             // SNES/NES: clock input
    int32 latchPin = 5;             // SNES/NES: latch input
    
    // Genesis / TG16
    int32 selectPin = 6;            // Genesis/TG16: SELECT input
    int32 oePin = 7;                // TG16: output enable (OE) input
    
    // TG16 data lines (4 parallel)
    int32 tg16Data0Pin = 8;
    int32 tg16Data1Pin = 9;
    int32 tg16Data2Pin = 10;
    int32 tg16Data3Pin = 11;
    
    // Genesis data lines (6 parallel)
    int32 genesisUp = 12;
    int32 genesisDown = 13;
    int32 genesisLeft = 14;
    int32 genesisRight = 15;
    int32 genesisA = 16;
    int32 genesisB = 17;
    int32 genesisC = 18;
    int32 genesisStart = 19;
    // ... (X, Y, Z, Mode for 6-button variant)
    
    // Dreamcast MAPLE
    int32 dreamcastSDCKA = 20;
    int32 dreamcastSDCKB = 21;
}
```

### Configuration & Activation

Add `GPIOOutputOptions gpio_output = X;` to `AddonOptions` in `proto/config.proto` (same as `SNESOptions`, `TG16Options`, etc.).

**Mode activation:** Via web configurator → choose console protocol and GPIO pins → save to flash → board uses new config on next boot (or runtime reload if the addon supports hot-reload).

**No new `InputMode` enum needed.** USB HID mode and GPIO output protocol are independent axes of configuration. Users can set `InputMode = HID` and separately enable GPIO output for their chosen console.

### USB & GPIO Coexistence

**Yes, they work together.** The gamepad state is processed by the USB HID driver, then passed to the `GPIOOutputAddon::process()` method. GPIO output reflects the same button state as the USB report, with negligible skew (< 1 ms, the main loop iteration time).

---

## Console Implementation Details

### NES Output

**Pins:** 3 (DATA, CLOCK, LATCH)  
**Timing:** 6–12 µs edges; looser than SNES  
**Implementation:** Interrupt-driven GPIO or PIO (optional)

The console sends LATCH HIGH for ~12 µs to load the shift register, then clocks out 8 bits on CLOCK (falling edges). Board must:
1. Detect LATCH rising edge → load current button state into an 8-bit shift register
2. On each CLOCK falling edge → present the next bit on DATA (active-low)
3. After 8 bits → wait for next LATCH

Existing `SNESpad` library implements the inverse; timings are directly reusable. Use active-low button encoding (button pressed = 0, released = 1).

**PIO optional.** Interrupt-driven `gpio_set_irq_enabled_with_callback()` on CLOCK and LATCH edges is simpler and sufficient given the loose timing tolerances.

### SNES Output

**Pins:** 3 (DATA, CLOCK, LATCH)  
**Timing:** 6–12 µs edges  
**Implementation:** Interrupt-driven or PIO

Same protocol as NES, but 16 bits per frame (SNES) instead of 8. Button sequence (active-low):
- Bits 0–11: B, Y, Select, Start, Up, Down, Left, Right, A, X, L, R
- Bits 12–15: Device ID bits (0b0000 = standard gamepad, 0b1000 = mouse)

For standard controller output, set device ID = 0b0000.

**Existing `SNESpad` library constants are directly reusable:** `SNES_LATCH_WIDTH`, `SNES_CLOCK_LOW_WIDTH`, `SNES_CLOCK_HIGH_WIDTH`.

### Genesis / Mega Drive Output

**Pins:** 7–9 (4 directions + 3–6 buttons + SELECT input)  
**Timing:** ~60 Hz polling (very loose)  
**Implementation:** Simple GPIO polling or IRQ on SELECT edge

Console reads up to 8 parallel data lines, multiplexed via SELECT:

**3-button mode:**
- SELECT=LOW → Board presents: {Up, Down, GND=0, GND=0, A, Start} (6 lines)
- SELECT=HIGH → Board presents: {Up, Down, Left, Right, B, C}

**6-button mode:** Additional SELECT pulses expose X, Y, Z, Mode buttons on the same pins.

Board monitors SELECT with GPIO IRQ and updates output lines accordingly. No shift register. Very simple implementation.

### TurboGrafx-16 / PC Engine Output

**Pins:** 6 (OE, SELECT, DATA0–3)  
**Timing:** ms-scale (very loose)  
**Implementation:** Simple GPIO polling

6-pin connector. OE (output enable, active-low) gates the output. SELECT drives which nibble is presented:

- SELECT=LOW → Board presents: {Up, Right, Down, Left} (active-low)
- SELECT=HIGH → Board presents: {I, II, Select, Run} (active-low)
- 6-button variant: additional SELECT pulses present {III, IV, V, VI}

Board monitors SELECT and presents the appropriate nibble on DATA0–3. Timing tolerances are ms-scale; simple bit-bang is completely sufficient.

### N64 Output

**Pins:** 1 (DATA, bidirectional + pull-up)  
**Timing:** 1 MHz NRZ (±500 ns tolerance)  
**Implementation:** PIO mandatory  
**Voltage:** 3.3V (no level shifting)

**Hardware note:** DATA is open-drain/open-collector. Both console and board drive it LOW as needed; an external pull-up (typically 3.3 kΩ to 3.3V) holds it HIGH when neither drives it. RP2040 PIO can be configured for open-drain output.

**Protocol:** Console sends 9-byte command frames; board responds with 4-byte data frames.

**Bit encoding (strict, ±500 ns tolerance):**
- `0` = 3 µs LOW + 1 µs HIGH
- `1` = 1 µs LOW + 3 µs HIGH

**Frame structure:**
- Command frame: 9 bytes (72 bits), starting with 0xFF
- Response frame: 4 bytes (32 bits)
- CRC-8 included in command; board echoes it in response

**PIO implementation:** A single state machine can:
1. Wait for falling edge (command start)
2. Clock in 72 bits at NRZ timing
3. Parse command bytes
4. Clock out 32-bit response at NRZ timing

Community implementations (e.g., "pico64") provide reference PIO programs (~15–20 instructions). Consult those for timing details.

### Dreamcast (MAPLE Bus) Output

**Pins:** 2 (SDCKA, SDCKB, half-duplex)  
**Timing:** ~2 Mbps burst (very high precision)  
**Implementation:** PIO mandatory (2 state machines recommended)  
**Voltage:** 3.3V (no level shifting)  
**Complexity:** **Highest of all targets**

**MAPLE Bus fundamentals:**

The Dreamcast polls each of 4 ports in sequence. Each port has two signal lines (SDCKA, SDCKB). The protocol is **half-duplex**: console sends a command frame, then listens for a response frame from the attached peripheral (board, in this case).

**Frame format:**
- Command frame: Start pattern → 4-byte header → payload (0–252 bytes) → 1-byte CRC → stop pattern
- Response frame: Same structure

**Header bytes:**
1. **Command code** (bits 7–4) + **Destination address** (bits 3–0)
2. **Source address** (bits 7–4) + **Number of 32-bit words in payload** (bits 3–0)
3. **Function code** (bits 7–4) + **Reserved** (bits 3–0)
4. **Partition number** (bits 7–0)

**CRC:** 8-bit XOR checksum of all header and payload bytes.

**Timing constraints:**
- Bit rate: ~2 Mbps (0.5 µs per bit)
- Dreamcast expects response within ~150–250 µs of command completion
- Bit transitions must be precise to ±50 ns (challenging without PIO)

**Bidirectional complexity:**

Unlike SNES (where the controller just shifts bits on a console-driven clock), MAPLE requires:
1. **RX state machine:** Detect and clock in the command frame, validate CRC, parse header
2. **TX state machine:** Assemble response frame (header + gamepad state payload), compute CRC, clock out bits

The two state machines typically run in parallel:
- PIO0 SM0: RX (watches for command start, clocks in bits)
- PIO0 SM1: TX (queued by RX SM; clocks out response)

**Implementation strategy:**

1. Use two PIO state machines (TX + RX), or a single SM that time-multiplexes (requires careful state management)
2. RX SM handles command reception and validation; on valid command, raises IRQ to Core0
3. Core0 response handler reads gamepad state, assembles response frame, feeds bytes to TX SM FIFO
4. TX SM clocks out response within the timing window

**Existing code:** **Zero existing Dreamcast/MAPLE code in GP2040-CE.** This is greenfield. Community Dreamcast-on-RP2040 projects exist; consult them for reference PIO implementations. The protocol is well-documented in Dreamcast community documentation.

**Timeline note:** Dreamcast is significantly more complex than other consoles. Recommend tackling it **last** in the implementation roadmap, after NES, SNES, Genesis, N64, and TG16 are working.

---

## Pin Assignment Strategy

### Recommended Allocation (Per Console)

**Generic Pico Configuration (NES output):**
```
GPIO 22: DATA (output)
GPIO 26: CLOCK (input)
GPIO 27: LATCH (input)
```

Add **74AHCT125** level shifter for 5V conversion.

**Generic Pico Configuration (N64 output):**
```
GPIO 22: DATA (bidirectional, open-drain)
No level shifter needed (3.3V native)
Pull-up resistor (3.3 kΩ) to 3.3V on DATA line
```

**Dedicated SNES Output Board (RP2350B, multi-console possible):**
```
GPIO 22: SNES DATA (output) → 74AHCT125 → pin 4 (red) of SNES connector
GPIO 26: SNES CLOCK (input) ← pin 2 (white) of SNES connector
GPIO 27: SNES LATCH (input) ← pin 3 (brown) of SNES connector

GPIO 28: N64 DATA (bidirectional) with pull-up
GPIO 4: Dreamcast SDCKA (output/input)
GPIO 5: Dreamcast SDCKB (output/input)
```

### Conflict Avoidance

1. **Don't reassign standard button GPIO (2–21)** — breaks button functionality
2. **Avoid GPIO 0–1** on display-enabled boards (I2C0)
3. **Avoid GPIO 23–25 on Pico W** (CYW43 SPI/SDIO)
4. **Avoid GPIO 29 on boards with VSYS monitoring**
5. **Document all pin assignments in your board's `BoardConfig.h`** using `ASSIGNED_TO_ADDON` markers

### Multi-Console Boards

For boards supporting multiple retro consoles simultaneously (rare but possible on RP2350B):

- **Total simultaneous PIO usage:** Max 4 state machines (SNES via PIO + N64 via PIO + Dreamcast via 2 SMs + WS2812 via 1 SM = 5 total, which exceeds 8 only if using the heaviest configuration)
- **RP2350B headroom:** 48 GPIO gives plenty of room for multiple console connectors
- **Hardware:** Multiple 74AHCT125 level shifters (one per 5V console) required

For most users, **a single dedicated output per board is recommended** (one board for SNES, another for N64, etc.). Multi-console boards are an advanced use case for enthusiasts.

---

## Implementation Roadmap

Phases are ordered by **implementation complexity and protocol maturity** (not by demand).

### Phase 1: Core NES/SNES Output (Bit-Bang or Simple PIO)

**Estimated timeline:** 5–7 days  
**Dependencies:** None  
**Deliverables:**
- `GPIOOutputAddon` class skeleton
- `GPIOOutputOptions` protobuf integration
- Interrupt-driven LATCH/CLOCK handling for SNES/NES
- Unit tests: shift register correctness, timing validation
- Hardware test: physical SNES console on a custom board with 74AHCT125 shifter

**Success criteria:**
- Board correctly shifts 16 bits on each LATCH cycle
- Gamepad buttons appear in correct bit positions on console
- No USB regression — USB HID output still works while GPIO enabled

### Phase 2: Genesis & TG16 Output (Parallel, Polling-Based)

**Estimated timeline:** 3–4 days  
**Dependencies:** Phase 1 complete  
**Deliverables:**
- Genesis 3-button and 6-button protocols
- TG16 parallel nibble output
- SELECT edge detection and output multiplexing
- Hardware tests: physical Genesis and TG16 consoles

**Success criteria:**
- SELECT transitions trigger correct button group on data lines
- 6-button detection sequences work (3-button falls back gracefully)
- Timing tolerances verified (loose, so easy to pass)

### Phase 3: N64 Output (PIO-Based, High Precision)

**Estimated timeline:** 5–7 days  
**Dependencies:** Phase 1 complete, PIO skill development  
**Deliverables:**
- N64 NRZ protocol PIO state machine (~15–20 instructions)
- 9-byte command RX + 4-byte response TX
- CRC validation
- Unit tests: bit-level timing, frame parsing
- Hardware test: physical N64 with appropriate board

**Success criteria:**
- Board correctly receives N64 command frames (±500 ns tolerance)
- Board responds with valid button state frame within timing window
- Physical N64 recognizes board as standard gamepad

### Phase 4: Dreamcast MAPLE Output (PIO-Based, Bidirectional, Highest Complexity)

**Estimated timeline:** 10–14 days  
**Dependencies:** Phase 3 complete, deep protocol study  
**Deliverables:**
- Dual-PIO (TX + RX) MAPLE frame handling
- Command parsing and CRC validation
- Response frame assembly from gamepad state
- RX → TX handoff via PIO IRQ
- Unit tests: frame formats, CRC, timing
- Hardware test: physical Dreamcast with appropriate board

**Success criteria:**
- Board correctly receives Dreamcast MAPLE command frames
- Board responds with valid function data within timing window
- Physical Dreamcast recognizes board and responds to button presses
- Polling continues across multiple frames (~60 Hz)

### Phase 5: Documentation & Troubleshooting (Ongoing)

**Estimated timeline:** 3–5 days  
**Dependencies:** All phases  
**Deliverables:**
- User guide: "How to build a GPIO output board"
- Per-console pinout diagrams and wiring guides
- Troubleshooting guide: common hardware/config mistakes
- API reference: `GPIOOutputAddon` class and protobuf config
- Migration guide: adapting existing input adapters to output

---

## Known Limitations

1. **5V Level Shifting Is Mandatory** — Boards targeting NES, SNES, Genesis, or TG16 **must include a 74AHCT125 or equivalent**. Direct GPIO-to-console connections at 3.3V may marginally work but are unreliable and not recommended. Documentation must state this clearly.

2. **Pico W Pin Constraints** — GPIO 23–25 are reserved for CYW43. Only 4 free pins (22, 26, 27, 28) available for retro output. Sufficient for most single-console boards; Genesis 6-button unsuitable without redesign.

3. **Dreamcast MAPLE Complexity** — Bidirectional protocol with precise timing, CRC validation, and command parsing. Significantly more complex than other consoles. Estimated 10–14 days to implement + test. No existing reference code in GP2040-CE; community projects required for guidance. Consider this a future/Phase 4 target, not Phase 1.

4. **No Runtime Output Switching** — Current architecture activates GPIO output at boot time via configuration. Runtime switching between USB and GPIO (without reboot) would require a new `OutputManager` layer above `GPDriver` and `GPIOOutputAddon`. Not in scope for Phase 1.

5. **PIO State Machine Budget** — Worst-case (N64 + Dreamcast + WS2812 all enabled) uses 4 of 8 available SMs on RP2040. RP2350B has 12 SMs, so overhead is not an issue there. A board with USB passthrough enabled (uses 3 SMs by default) + full GPIO output would exhaust resources. Documented in board configs via `ASSIGNED_TO_ADDON` markers.

6. **No GPIO Output on RP2040-Zero or Ultra-Small Boards** — Boards like the SeeedXIAO RP2040 and Waveshare Zero expose most GPIO as button pins. Fewer than 3 free pins remain. GPIO output not feasible on these without significant hardware redesign.

7. **USB Regression Risk** — Early PIO work must be carefully validated to ensure no interference with existing PIO-USB host passthrough on boards like `ReflexCtrlSaturn`. Comprehensive regression tests required.

---

## Testing Strategy

### Unit Tests

- **Shift register correctness:** Verify SNES/NES shift register loads and clocks correctly
- **CRC validation:** Dreamcast frame CRC assembly and checking
- **Button-to-bit mapping:** Confirm each button appears at the correct position in output frames
- **Pin assignment validation:** Check that board config GPIO assignments are within valid ranges and don't conflict

### Hardware Tests (Per Console)

| Console | Hardware Test | Pass Criteria |
|---|---|---|
| **NES** | Connect to real NES, play Super Mario Bros. | Player input on console matches board buttons |
| **SNES** | Connect to real SNES, play Super Metroid or F-Zero | Analog sticks (if supported), button inputs work correctly |
| **Genesis** | Connect to real Genesis, play Sonic the Hedgehog (3-button) and Castlevania IV (6-button) | Button input maps correctly; 6-button detection works |
| **TG16** | Connect to real PC Engine, play Bonk's Adventure | Directional and button input works |
| **N64** | Connect to real N64, play Super Mario 64 | Board detected as standard gamepad; movement and buttons responsive |
| **Dreamcast** | Connect to real Dreamcast, play a sports or fighting game | Board detected as controller; button input polling works across multiple frames |

### Regression Tests

- **USB functionality:** Board still enumerates as standard HID gamepad with GPIO enabled
- **Other addons:** WS2812 LEDs, I2C display, turbo button still function normally
- **Board configs:** Existing configs (Pico, ReflexCtrlSaturn, etc.) still build and function with GPIO output disabled
- **Web configurator:** New GPIO output settings appear and can be saved/loaded without corruption

### CI/CD Pipeline

- Firmware builds successfully for Pico, Pico 2, and RP2350 variants with GPIO enabled
- Protobuf definitions compile without error
- Unit tests pass (shift register, CRC, frame parsing)
- No compiler warnings related to GPIO addon code

---

## Cross-Reference

- **Related feature:** See `docs/development/bluetooth-support.md` for the parallel USB/Bluetooth coexistence architecture — GPIO output follows the same multi-output pattern
- **Board configurations:** See `docs/development/rp2350-support.md` for RP2350 GPIO availability and custom board config creation
- **Dependency updates:** See `docs/development/dependency-updates.md` for firmware stack version requirements (Pico SDK 2.2.0+, nanopb, etc.)

---

**Revision history:**
- 2026-03-28: Initial feature planning document created

