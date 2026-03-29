# Latency Testing Framework — Feature Planning

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Planning / Research  
**Scope:** USB HID (primary), BLE HID / BT Classic HID (secondary)

---

## Overview

This document describes a proposed **latency testing framework** for GP2040-CE — a hardware-and-software test rig that produces repeatable, statistically rigorous, independently verifiable measurements of input-to-output latency across firmware configurations.

The goal is to move the community conversation from subjective claims and anecdote to **published empirical data** with methodology transparent enough that any builder can reproduce the results with off-the-shelf hardware costing under $50.

**What this produces:**
- Precise measurements of the time from physical GPIO state change to USB/BLE HID report receipt by the host
- Statistical distributions (min, max, mean, median, 99th percentile, histogram) across thousands of samples
- Side-by-side comparisons across firmware versions, debounce settings, USB polling rates, and wireless modes
- A reusable, open test fixture design that community members can build themselves

---

## What Latency Actually Means Here

Before measuring, it is important to be precise about what is being measured. "Input latency" in the GP2040-CE context has several distinct components:

```
Physical button press
        │
        ▼  (A) GPIO input propagation
GPIO pin state changes
        │
        ▼  (B) Firmware debounce filter
Debounce threshold crossed → button state = pressed
        │
        ▼  (C) Gamepad loop processing
HID report constructed and queued for USB
        │
        ▼  (D) USB frame boundary wait
Next USB polling interval (host polls device)
        │
        ▼  (E) USB packet transmission
Host USB controller receives IN token → device sends HID report
        │
        ▼  (F) OS HID driver processing
OS delivers HID report to application / game
        │
   ═════╪═════ ← THIS FRAMEWORK STOPS HERE
        │
        ▼  (G) Game engine input polling     ┐
Game reads input state                        │  OUT OF SCOPE
        │                                     │
        ▼  (H) Game logic processing          │  Game-specific,
Collision detection, physics, etc.            │  engine-specific,
        │                                     │  title-specific.
        ▼  (I) Render pipeline                │  Not measured here.
Frame rendered to framebuffer                 │
        │                                     │
        ▼  (J) Display pipeline               │
Pixel appears on screen                       ┘
```

**This framework measures A through F — controller to host OS — and nothing beyond.**

### Why We Stop at the Host OS

The latency from button press to pixel on screen (A–J) is what players ultimately perceive, but components G–J are entirely outside the firmware's control and vary by:
- Game engine and its input polling rate (some games poll at 60Hz, others at 1000Hz)
- Game logic update rate (decoupled from render rate in modern engines)
- GPU render pipeline depth (1–3 frame pipeline latency is common)
- Display refresh rate and response time (60Hz = 16.7ms per frame; 360Hz = 2.8ms)
- Display mode (VSync on/off, G-Sync, FreeSync)

A 1ms firmware improvement is real and measurable. Whether that 1ms is perceptible in a game running at 60fps with a 6ms display depends on factors that have nothing to do with the controller. **These measurements characterize the controller's contribution in isolation** — they are game-agnostic and display-agnostic by design.

Importantly: **a game's rendered frame and the USB polling interval are independent cycles.** The host OS receives a new HID report every 1ms (the USB frame boundary). The game may read that report at 60fps (every 16.7ms), 120fps (8.3ms), or any other rate depending on its engine. The controller does not know and does not care — it delivers the report to the OS on schedule regardless of what the game is doing with it. The test framework measures the USB delivery, not the game's consumption of it.

**Total firmware-controlled latency = A + B + C + D + E + F**

Of these, GP2040-CE firmware directly controls **B** (debounce, configurable) and **C** (gamepad loop speed). Component **D** is bounded by the USB polling interval (1ms for GP2040-CE). Components **A**, **E**, and **F** are hardware/OS constants that are not firmware-controlled.

The test framework must isolate and measure each of these independently to produce meaningful results. Comparing debounce settings without controlling for polling jitter, for example, produces noise, not data.

---

## Latency Budget (USB)

GP2040-CE's theoretical minimum USB latency, based on the actual firmware architecture:

| Component | Typical Value | Source |
|---|---|---|
| GPIO input propagation (A) | < 0.1 µs | RP2040/RP2350 GPIO |
| Firmware debounce (B) | 0–5 ms | `debounceDelay` in `config.proto:18`; `debounceGpioGetAll()` in `gp2040.cpp:253` |
| Gamepad loop (C) | < 0.1 ms | `while(1)` loop at CPU speed (~125 MHz), `gp2040.cpp:298` |
| USB frame wait (D) | 0–1 ms | `bInterval = 1` in `HIDDescriptors.h:176`; uniformly distributed |
| USB packet tx (E) | < 0.1 ms | `tud_hid_report()` in `HIDDriver.cpp:113`; 12-byte report |
| OS HID driver (F) | 0.1–2 ms | OS-dependent; Windows HID jitter varies |
| **Total (0 debounce)** | **~0.1–3 ms** | Dominated by D + F |
| **Total (5ms debounce)** | **~5–8 ms** | Dominated by B |

### Key Firmware Facts

**The gamepad loop** (`gp2040.cpp:298`) runs as fast as the CPU allows with no artificial delay. The loop processes buttons through: debounce → `gamepad->read()` → add-ons → `tud_task()`. The practical rate is bounded by the 1ms USB polling interval — once a report is sent, no new report can be delivered until the next USB frame regardless of how fast the loop runs.

**Debounce** (`gp2040.cpp:253–279`) uses a simple time-threshold filter: a button state change is only accepted if the time since the last state change on that GPIO exceeds `debounceDelay` milliseconds. The timer resolution is **milliseconds** (`getMillis()` / `to_ms_since_boot()`), meaning debounce values below 1ms are effectively zero. Setting `debounceDelay = 0` bypasses the filter entirely.

**USB polling interval** (`HIDDescriptors.h:176`) is set to `bInterval = 1` in the interrupt endpoint descriptor, meaning the host polls the device every **1ms**. This is a compile-time constant in the USB descriptor — it is not runtime-configurable. The HID report is sent via `tud_hid_report(0, report, 12)` only when the report changes and `tud_hid_ready()` is true.

The practical floor without debounce is ~1ms average due to the USB polling interval — this is the physical limit of full-speed USB HID regardless of firmware speed. Any claim of "sub-1ms average USB latency" for a full-speed HID device warrants skepticism.

---

## Measurement Methodology

### Approach: Dual-Signal Oscilloscope / Logic Analyzer Capture

The most rigorous and reproducible approach uses a **test driver microcontroller** that generates a known electrical signal simultaneously with a GPIO toggle on the device under test (DUT), and a **logic analyzer** that captures both signals with microsecond resolution.

This eliminates OS timing jitter entirely from the measurement — USB packet timing is captured at the electrical level, not through software timestamps.

```
┌─────────────────────────────────────────┐
│  Test Driver (Pico)                      │
│                                          │
│  GPIO_OUT ──────────────────────────────┼──► DUT Button Input (simulates press)
│                                          │        │
│  TRIGGER_OUT ──────────────────────────┼──► Logic Analyzer CH0 (reference)
│                                          │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  Device Under Test (GP2040-CE board)     │
│                                          │
│  Button GPIO ◄──── from Test Driver      │
│  USB D+ / D- ──────────────────────────┼──► Logic Analyzer CH1 (USB traffic)
│                                          │
└─────────────────────────────────────────┘

┌─────────────────────────────────────────┐
│  Logic Analyzer                          │
│  CH0: Trigger (button press event)       │
│  CH1: USB D+ (protocol decode)           │
│  → Measures Δt between CH0 edge and      │
│    first HID IN packet containing        │
│    button=pressed                        │
└─────────────────────────────────────────┘
```

### Why Not a Software Timer on the Host?

Software-based approaches (Python HID timestamps, DirectInput polling) are **not suitable** for sub-millisecond measurement:
- Windows HID driver adds 0–2ms of unpredictable jitter
- Python `time.perf_counter()` has ~15ms timer resolution on Windows without HPET
- OS thread scheduling introduces noise that drowns out the firmware differences being measured

The logic analyzer approach measures at the wire level — results are independent of host OS, driver stack, and application timing.

---

## Required Hardware

### Minimum Configuration (~$35–50 total)

| Item | Purpose | Recommended Part | Approx. Cost |
|---|---|---|---|
| Test driver MCU | Generates precise button press signals | Raspberry Pi Pico (RP2040) | $4 |
| Logic analyzer | Captures USB timing at wire level | Cypress FX2-based 8ch (Saleae clone, sigrok-compatible) | $10–15 |
| Small breadboard + jumper wires | Connections | Any | $3–5 |
| USB breakout board | Access USB D+/D- signals | USB-A male breakout or USB sniffing cable | $5–8 |
| DUT board | Device under test | Any GP2040-CE-compatible board (Pico recommended) | $4 |

**Total: ~$26–36** (not counting the GP2040-CE DUT board you already have)

### Recommended Upgrade (~$150)

| Item | Purpose | Recommended Part | Approx. Cost |
|---|---|---|---|
| Saleae Logic 8 | Higher-quality logic analyzer, official sigrok support, better software | Saleae Logic 8 | $150 |

The Saleae Logic 8 provides better timing accuracy and the official Logic 2 software has built-in USB protocol decoding, which significantly simplifies result extraction.

### For BLE Testing (Additional ~$30)

| Item | Purpose | Recommended Part | Approx. Cost |
|---|---|---|---|
| Nordic nRF Sniffer dongle | Capture BLE radio packets with timestamps | Nordic nRF52840 Dongle + nRF Sniffer firmware | $10 |
| Wireshark | Decode BLE packets | Free | $0 |

The nRF Sniffer captures BLE advertising and connection events with microsecond timestamps, allowing the same analysis applied to USB.

---

## Test Driver Firmware

The test driver (a second Raspberry Pi Pico) runs a simple firmware that:
1. Asserts a GPIO line (simulating a button press on the DUT) at a precise known time
2. Simultaneously asserts a trigger output to the logic analyzer
3. Holds the press for a configurable duration (e.g., 50ms — long enough to guarantee the DUT samples it)
4. Releases and waits a configurable idle time (e.g., 200ms) before the next press
5. Repeats for a configurable number of iterations (e.g., 10,000)

The test driver GPIO is connected to the DUT's button input via a resistor divider or direct connection (with appropriate level matching if needed, though both boards are 3.3V logic).

```c
// Pseudocode — test driver main loop
void run_test(uint iterations, uint press_ms, uint idle_ms) {
    for (uint i = 0; i < iterations; i++) {
        gpio_put(TRIGGER_PIN, 1);   // Signal logic analyzer
        gpio_put(BUTTON_PIN, 1);    // Assert button on DUT
        sleep_ms(press_ms);
        gpio_put(TRIGGER_PIN, 0);
        gpio_put(BUTTON_PIN, 0);    // Release button
        sleep_ms(idle_ms);
    }
}
```

The trigger and button assert happen in consecutive instructions — the timing delta between them is deterministic and sub-microsecond on RP2040. For the test to be valid, this delta must be characterized and subtracted from results (it will be < 100ns and negligible).

---

## Measurement Procedure

### USB Latency Test

1. Connect test driver GPIO → DUT button input GPIO
2. Connect test driver TRIGGER → Logic Analyzer CH0
3. Connect USB D+ from DUT → Logic Analyzer CH1 (via USB breakout)
4. Connect DUT USB → Host PC (running in intended USB mode: XInput, HID, etc.)
5. Configure DUT with the debounce setting under test
6. Start logic analyzer capture (continuous, 24 MHz sample rate minimum)
7. Run test driver for N=10,000 iterations
8. Stop capture
9. Export to CSV or use sigrok scripting to extract Δt for each iteration

**What to extract from the capture:**
- CH0 rising edge timestamp = `t_press`
- Next USB IN transaction containing button=1 = `t_report`
- `latency = t_report - t_press`
- Repeat for all N iterations → latency distribution

### Statistical Reporting

For each test configuration, report:
- **N** (sample count)
- **Min** latency (µs)
- **Max** latency (µs)
- **Mean** latency (µs)
- **Median / p50** (µs)
- **p95** (95th percentile)
- **p99** (99th percentile)
- **Std deviation** (µs)
- **Histogram** (50µs buckets, plotted as ASCII or image)

### Test Matrix

Run the full measurement procedure for each combination:

| Variable | Values to Test | Config Location |
|---|---|---|
| Debounce time | 0ms, 1ms, 2ms, 5ms (default) | `GamepadOptions.debounceDelay` (`config.proto:18`) |
| USB mode | XInput, Generic HID | `GamepadOptions.inputMode` |
| USB polling interval | 1ms (default) | `HIDDescriptors.h:176` — requires recompile to change |
| Firmware version | Current release vs. each PR under test | git SHA |
| Host OS | Windows 11, Linux (Ubuntu) | — |

Each cell in this matrix = one run of N=10,000 samples = one row in the results table.

---

## BLE / Bluetooth Classic Latency

Wireless latency measurement requires a different capture method since there is no physical wire to probe.

### BLE Latency

**Method:** Nordic nRF Sniffer + Wireshark
- The sniffer dongle captures all BLE packets on the air with hardware timestamps
- After a test run, Wireshark post-processing extracts:
  - Test driver trigger signal (via a second GPIO connected to a BLE GATT notification or a BLE beacon from the test driver — or use oscilloscope for trigger + sniffer for BLE)
  - BLE HID report packet timestamps

**Alternative method for BLE:** Oscilloscope on GPIO + BLE sniffer in parallel, correlate timestamps using a shared reference.

**Expected BLE latency components:**

| Component | Typical Value | Notes |
|---|---|---|
| GPIO → firmware | < 0.1ms | Same as USB |
| Debounce | 0–5ms | Same as USB |
| BLE connection interval | 7.5–45ms | Negotiated at pairing; gamepad profile requests minimum |
| BLE packet tx | < 1ms | Short BLE advertisement |
| OS BLE HID driver | 1–5ms | More variable than USB |
| **Total (0 debounce, min interval)** | **~8–15ms** | BLE connection interval dominates |

BLE latency is fundamentally higher than USB due to the connection interval. The BLE HID spec allows requesting a 7.5ms minimum connection interval, which Windows and Android may or may not grant.

### BT Classic Latency

BT Classic HID latency is more complex (L2CAP scheduling, sniff mode intervals) and is secondary in priority. The same nRF sniffer approach applies but the sniffer must be configured for BR/EDR capture, which requires different hardware (the nRF52840 dongle supports BR/EDR sniffing with appropriate firmware).

---

## Analysis Tooling

### Automated Extraction Script

A Python script (`tools/latency_analyze.py`) that takes a sigrok/Logic2 CSV export and produces the statistical report and histogram:

```python
# tools/latency_analyze.py
# Usage: python latency_analyze.py --input capture.csv --output results.json --plot histogram.png
```

This script:
- Parses logic analyzer CSV (CH0 = trigger, CH1 = USB protocol decode)
- Matches each trigger edge to the next USB HID IN report with button=pressed
- Computes the full statistics table
- Outputs JSON (machine-readable, for CI integration) and PNG histogram

### Reproducibility Requirements

For a test result to be considered independently validated:
1. The firmware version under test must be identified by git commit SHA
2. The test driver firmware version must be published
3. The test hardware BOM must match the specified hardware list
4. N ≥ 1,000 samples (N ≥ 10,000 recommended for percentile accuracy)
5. The raw capture file (or the extracted CSV) must be published alongside the results

---

## Publishing Results

Results are published in `docs/latency/` as versioned Markdown tables, one file per firmware release:

```
docs/latency/
  README.md          — methodology summary and hardware BOM
  v0.7.12.md         — results for release 0.7.12
  v0.7.13.md         — results for release 0.7.13
```

Each results file includes:
- Firmware version + git SHA
- Test hardware used
- Host OS + driver version
- Full statistics table (one row per test configuration)
- Histogram images

---

## Community Reproducibility

The test fixture is designed to be buildable by any community member with basic electronics knowledge. The total BOM cost is under $50. All software (sigrok, Python analysis script) is open source. The test driver firmware will be published in the GP2040-CE repository under `tools/latency-tester/`.

This satisfies the core requirement: any claim about GP2040-CE latency — whether from the core team or a community member tweaking debounce — can be verified against a published standard methodology. "I measured X ms" means nothing without methodology; "I ran the GP2040-CE latency test rig and got X ms at config Y" is independently verifiable.

---

## Implementation Plan

### Phase 1 — USB Latency (Primary)

1. Design and publish test driver firmware (`tools/latency-tester/pico/`)
2. Publish hardware BOM and wiring diagram (`docs/latency/hardware.md`)
3. Write `tools/latency_analyze.py` analysis script
4. Run baseline measurements for current firmware release
5. Publish first results file (`docs/latency/v0.7.12.md`)

### Phase 2 — Firmware Integration

1. Add latency test results to CI as a benchmark regression check (fail if p99 latency increases by >0.5ms vs baseline)
2. Run test matrix for each PR that touches the gamepad loop or debounce logic
3. Publish results in PR description

### Phase 3 — BLE / Wireless

1. Characterize BLE connection interval negotiation with Windows and Android
2. Build BLE capture setup (nRF sniffer + correlation methodology)
3. Publish BLE latency results alongside USB for direct comparison
4. Establish expected BLE latency range so community has realistic expectations

---

## Acceptance Criteria

- [ ] Hardware BOM published with links and estimated cost
- [ ] Wiring diagram published
- [ ] Test driver firmware published and buildable
- [ ] `latency_analyze.py` script published and documented
- [ ] Baseline results published for current firmware release at standard configurations
- [ ] Results are reproducible: a second person following the published methodology gets results within 5% of published values
- [ ] Histogram and statistics table format is standardized and consistent across releases

---

## Open Questions

1. **USB sniffing method:** Direct logic analyzer on D+/D- (requires USB breakout) vs. USB protocol analyzer (e.g., Total Phase Beagle — expensive but purpose-built). The cheap sigrok analyzer approach should be validated first.

2. **High-speed USB:** RP2040 and RP2350 use full-speed USB (12 Mbps, 1ms frame). Is high-speed USB (480 Mbps, 125µs microframes) achievable via the Pico's PIO USB or an external USB hub? If so, the polling interval floor drops from 1ms to 125µs — potentially significant. This should be measured separately.

3. **Debounce algorithm:** GP2040-CE currently uses a simple time-based debounce. Are there lower-latency debounce algorithms (e.g., integrating filter, hardware schmitt trigger) that reduce input latency without increasing false positives? This is a separate firmware research question but the test rig would be the tool to evaluate it.

4. **Sample count:** N=10,000 takes approximately 35 minutes at 200ms/press. Is this practical for a CI run? Recommendation: N=1,000 for CI (fast), N=10,000 for published release benchmarks (thorough).

5. **Independent validation:** Should the project formally invite 2–3 community members to run the test rig independently before publishing results, to establish credibility? Recommended for the first published results.
