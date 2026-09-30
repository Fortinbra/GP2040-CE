# USB 3 / High-Speed USB Gamepad — Feasibility Research

**Last updated:** 2026-03-29  
**Maintained by:** GP2040-CE core team  
**Status:** Research / Feasibility Study  
**Scope:** USB 3 SuperSpeed, USB 2.0 High-Speed, MCU options, latency implications

---

## Overview

USB 3 (SuperSpeed, 5 Gbps) gamepads are theoretically possible. The question is whether they offer a meaningful latency or performance improvement over the current full-speed USB HID implementation — and if so, what hardware changes would be required to get there while preserving as much of the existing GP2040-CE software stack as possible.

**Short answer:** The practical target is **USB 2.0 High-Speed**, not USB 3. USB 2.0 HS already achieves the same minimum HID polling interval as USB 3 SuperSpeed (125µs vs. 1ms for full-speed). USB 3 offers no additional benefit for gamepad HID reports. However, reaching USB 2.0 HS requires a platform change — neither the RP2040 nor RP2350 supports high-speed USB natively.

---

## USB Speed Tiers and HID Polling

Understanding the relationship between USB speed and HID polling is essential before evaluating hardware options.

### USB Speed Tiers

| USB Version | Marketing Name | Bandwidth | Frame / Microframe | Min Interrupt Interval |
|---|---|---|---|---|
| USB 1.1 | Full-Speed (FS) | 12 Mbps | 1ms frames | 1ms |
| USB 2.0 | High-Speed (HS) | 480 Mbps | 125µs microframes | 125µs |
| USB 3.0 | SuperSpeed (SS) | 5 Gbps | 125µs microframes (carried from USB 2 model) | 125µs |
| USB 3.1 Gen 2 | SuperSpeed+ | 10 Gbps | 125µs microframes | 125µs |
| USB 3.2 / USB4 | SuperSpeed+ / USB4 | 20–40 Gbps | 125µs microframes | 125µs |

### The Key Finding: USB 3 ≠ Faster HID Polling Than USB 2 HS

**USB 3.0 and USB 2.0 High-Speed share the same minimum interrupt endpoint service interval: 125µs.** When `bInterval = 1` is set on a SuperSpeed interrupt endpoint, the host polls the device every 2^(1-1) × 125µs = 125µs — identical to USB 2.0 HS.

The bandwidth increase from USB 2 HS → USB 3 SS (480 Mbps → 5 Gbps) is entirely irrelevant for a 12-byte HID gamepad report. A USB 3 gamepad would gain:

- ✅ 125µs polling interval (vs. 1ms at full-speed) — **same as USB 2.0 HS**
- ✅ Compatibility with USB 3 hub topology without negotiating back to FS
- ❌ No latency improvement vs. USB 2.0 HS
- ❌ No meaningful bandwidth benefit (HID reports are tiny)
- ❌ Significantly more complex hardware (USB 3 PHY, separate SuperSpeed terminations, increased power)

**The practical upgrade path for latency is USB 2.0 High-Speed, not USB 3.**

### Latency Impact of USB 2.0 HS

At 125µs polling vs. the current 1ms:

| Metric | Current (FS, 1ms) | USB 2.0 HS (125µs) | Improvement |
|---|---|---|---|
| Minimum USB frame wait (D) | 0–1ms | 0–0.125ms | 8× reduction |
| Average USB frame wait | ~0.5ms | ~0.0625ms | 8× reduction |
| Theoretical minimum total (no debounce) | ~0.1–3ms | ~0.07–2ms | Modest |

In practice, the OS HID driver processing time (component F) is 0.1–2ms on Windows and adds jitter that partially masks the polling improvement. However, p99 and maximum latency improve significantly since D is no longer the worst-case term.

---

## Current Platform: RP2040 and RP2350

### What the RP Chips Support

Both the RP2040 and RP2350 include the **same USB controller**: a USB 1.1 Full-Speed device/host controller running at 12 Mbps. This is a hard silicon limitation — there is no USB 2.0 High-Speed PHY on either chip, and no path to it through firmware.

| Feature | RP2040 | RP2350A | RP2350B |
|---|---|---|---|
| USB controller | USB 1.1 FS | USB 1.1 FS | USB 1.1 FS |
| Max USB speed | 12 Mbps (FS) | 12 Mbps (FS) | 12 Mbps (FS) |
| USB 2.0 HS | ❌ No | ❌ No | ❌ No |
| USB 3.0 SS | ❌ No | ❌ No | ❌ No |
| Min HID poll interval | 1ms | 1ms | 1ms |

### Can PIO Implement USB 2.0 HS?

The RP2040 PIO subsystem runs at up to the system clock (max ~300 MHz when overclocked). USB 2.0 High-Speed requires a 480 Mbps differential serial stream — signal transitions at **480 million per second**. This exceeds PIO's maximum effective throughput even at maximum overclock, and USB HS also requires differential NRZI encoding, bit stuffing, CRC16 generation, and precise eye pattern compliance that PIO state machines cannot reliably produce.

GP2040-CE already uses `tinyusb_pico_pio_usb` for the USB host port (`CFG_TUH_RPI_PIO_USB = 1` in `headers/tusb_config.h`), but PIO USB is documented as full-speed only. No viable PIO-based USB 2.0 HS device implementation exists for RP2040/RP2350.

### External USB PHY / Controller Chips on a Custom RP PCB

This is the most architecturally interesting path: keep the RP chip as the gamepad logic core and add a dedicated USB chip to handle HS device-side USB. There are two distinct categories of chip to consider.

#### Category A: ULPI PHY Only (USB3320C, SMSC USB3343, etc.)

A ULPI (UTMI+ Low Pin Interface) PHY chip handles only the **physical and electrical layer** of USB HS. It translates between the ULPI bus (8-bit parallel @ 60 MHz) and the USB D+/D- differential pair at 480 Mbps. It does not handle USB protocol, enumeration, endpoints, or packets — it expects a USB controller to drive it.

The RP2040/RP2350 USB controller is hardwired to the internal full-speed PHY and does not expose a UTMI/ULPI interface externally. There is no documented or practical way to replace the internal PHY with an external ULPI chip while still using the RP's USB controller.

The ULPI interface itself (8 data pins + CLK + DIR + NXT + STP = 12 pins, running at 60 MHz DDR) could theoretically be driven by PIO — RP2040 PIO can run at system clock (125–300 MHz), and 60 MHz DDR is achievable. However, driving the ULPI PHY from PIO still requires a USB device controller stack above it (packet assembly, endpoint management, transaction sequencing) — and the RP's built-in USB controller cannot be redirected to the external PHY. You would need to implement the full USB device controller logic in software/PIO, which is equivalent to writing a USB HS device stack from scratch. This has not been done and is not a practical path.

**ULPI PHY alone does not work with RP chips. A full USB device controller is also required.**

#### Category B: USB HS Device Controller with Parallel / SPI Interface

These chips include both the USB controller and PHY, and expose a simpler interface (FIFO, SPI, or parallel bus) to the host processor. The RP chip communicates with the USB controller chip over this bus, forwarding HID report data, and the controller chip handles all USB HS protocol complexity.

| Chip | USB Speed | Interface to RP | Notes |
|---|---|---|---|
| **Infineon EZ-USB FX2LP** (CY7C68013A) | USB 2.0 HS | GPIF (parallel, up to 48 MB/s) or SPI-like | Industry standard; has on-chip 8051 MCU for USB firmware; ~$3 |
| **Infineon EZ-USB FX3** (CYUSB301x) | USB 3.0 SS | GPIF-II (up to 400 MB/s) or UART/SPI | Has on-chip ARM9 @ 200 MHz; supports USB 3.0 SS; ~$8–12 |
| **FTDI FT600 / FT601** | USB 3.0 SS | 16/32-bit FIFO | Pure FIFO bridge, no user firmware; designed for data streaming; ~$6 |

**The FX2LP dual-MCU approach is the most practical "keep RP" path:**

```
┌────────────────────────────────────────────┐
│  RP2040 / RP2350                            │
│  • GPIO button reading                      │
│  • Debounce + gamepad logic                 │
│  • Builds HID report (12 bytes)             │
│  • Sends report over SPI / UART at ~1 MHz   │
└────────────────┬───────────────────────────┘
                 │  SPI or 8-bit parallel
                 ▼
┌────────────────────────────────────────────┐
│  Infineon EZ-USB FX2LP (CY7C68013A)         │
│  • Runs vendor USB firmware (8051 @ 48 MHz) │
│  • Presents USB 2.0 HS HID device to host  │
│  • Forwards RP-provided reports as HID IN   │
│  • bInterval = 1 → 125µs polling            │
└────────────────┬───────────────────────────┘
                 │  USB 2.0 HS (480 Mbps)
                 ▼
            Host PC / console
```

**Advantages:**
- RP2040 retains all gamepad logic, GPIO handling, add-ons, web config
- FX2LP handles USB HS — no RP USB stack changes needed
- FX2LP firmware is small (USB HID + FIFO read loop, ~200 lines of C)
- FX2LP is well-documented with open-source USB HID examples
- Pico SDK and GP2040-CE codebase unchanged (RP is no longer the USB device)

**Disadvantages:**
- Two MCUs on every board — added BOM cost (~$3 for FX2LP) and PCB complexity
- Power sequencing between RP and FX2LP must be managed
- The RP's TinyUSB stack is entirely bypassed — web config RNDIS and XInput enumeration now occur on the FX2LP, requiring separate FX2LP firmware for each USB mode (XInput, HID, PS4, etc.)
- Latency between RP and FX2LP adds a small inter-MCU communication delay (~10–50µs at SPI 1 MHz) — small but measurable
- Community firmware maintenance now spans two separate firmware images

**FX3 for USB 3 SuperSpeed:** The FX3 approach is analogous to FX2LP but exposes USB 3.0 SS to the host. The inter-MCU communication overhead becomes a larger fraction of the 125µs polling budget. As established earlier, USB 3 SS offers no polling advantage over USB 2.0 HS for HID, so FX3 provides a "USB 3 gamepad" marketing claim without a measurable latency improvement over FX2LP.

**FTDI FT600/FT601:** These are pure FIFO bridges intended for high-throughput data streaming (logic analyzers, cameras). They have no HID firmware capability — you would need an additional USB controller above them. Not suitable.

#### Category C: Dedicated USB HS HID Bridge (Theoretical)

A chip that accepts a simple serial report format and presents a configurable USB HID device to the host — essentially an "HID report coprocessor". No mass-market chip fills this role exactly, but the concept exists in some industrial/embedded specialised controllers. Worth monitoring as the USB HID peripheral chip market evolves.

#### Summary: External Chip Options on RP PCB

| Approach | USB Speed | BOM Complexity | Software Complexity | Recommended? |
|---|---|---|---|---|
| ULPI PHY only (USB3320C) | HS capable, but no controller — unusable | Low chip count, high PIO complexity | Effectively impossible | ❌ No |
| FX2LP as USB HS front-end | USB 2.0 HS (125µs) | +1 chip, +SPI traces | Moderate (FX2LP firmware) | ⚠️ Feasible, complex |
| FX3 as USB 3 SS front-end | USB 3.0 SS (125µs, same as HS) | +1 chip, +GPIF-II | High (FX3 firmware) | ❌ Not worth the effort vs. FX2LP |
| Switch MCU (iMX RT1062) | USB 2.0 HS integrated | Single chip | Medium (HAL port) | ✅ Cleaner path to HS |

---

## MCU Options for USB 2.0 High-Speed

Achieving USB 2.0 HS requires an MCU with either:
- An integrated USB 2.0 HS PHY (rare), or
- A USB OTG HS controller + external ULPI (USB UTMI+ Low Pin Interface) PHY chip

### Option 1: STM32H7 Series (Recommended)

| Property | Value |
|---|---|
| Core | ARM Cortex-M7 |
| Clock | 480 MHz |
| USB | USB OTG FS + USB OTG HS (with external ULPI PHY) |
| ULPI PHY required | Yes (e.g., USB3320C, ~$1.50) |
| Min HID poll interval | 125µs |
| TinyUSB support | ✅ Full (STM32H7 is a first-class TinyUSB target) |
| GPIO count | Up to 114 (LQFP144) |
| SRAM | 1 MB on-chip + 4 MB external possible |
| Cost (MCU only) | ~$5–10 (STM32H743) |
| Toolchain | ARM GCC (same as RP) |
| SDK | STM32 HAL or libopencm3; not Pico SDK |

The STM32H7 is the most practical path to USB 2.0 HS while retaining TinyUSB and the ARM GCC toolchain. The Pico SDK (`pico/stdlib.h`, PIO, etc.) is RP-specific and would require replacement with STM32 HAL, but the TinyUSB layer above it (HID descriptors, report logic) is largely portable.

The external ULPI PHY (e.g., Microchip USB3320C, SMSC USB334x) adds one chip and ~10 PCB traces but is a small, inexpensive, well-understood component.

### Option 2: NXP i.MX RT1062 (Teensy 4.x platform)

| Property | Value |
|---|---|
| Core | ARM Cortex-M7 |
| Clock | 600 MHz |
| USB | USB 2.0 HS OTG (integrated HS PHY — no external PHY needed) |
| ULPI PHY required | ❌ No (integrated) |
| Min HID poll interval | 125µs |
| TinyUSB support | ✅ Full (iMX RT is a first-class TinyUSB target) |
| GPIO count | Up to 124 |
| SRAM | 1 MB tightly coupled + 512 KB general |
| Cost (MCU only) | ~$8–14 (MIMXRT1062) |
| Toolchain | ARM GCC (same as RP) |
| SDK | NXP MCUXpresso SDK; not Pico SDK |

The iMX RT1062 has an **integrated USB 2.0 HS PHY** — no external ULPI chip needed, which simplifies the PCB. Teensy 4.0/4.1 boards are based on this chip and demonstrate USB HS gamepad operation in practice (the Teensy platform has existing USB HID gamepad implementations at HS speeds).

### Option 3: STM32H5 Series (Smaller, Newer)

| Property | Value |
|---|---|
| Core | ARM Cortex-M33 (same architecture as RP2350) |
| Clock | 250 MHz |
| USB | USB 2.0 FS only on most variants; select variants have HS |
| TinyUSB support | ✅ (H5 FS-only confirmed; HS support emerging) |
| Cost | ~$3–6 |

The M33 core is architecturally very similar to the RP2350, which eases code migration. However, USB HS availability varies by variant — confirm part number before committing.

### Option 4: Remain on RP + Accept Full-Speed Limits

If staying on RP is a hard requirement (community familiarity, existing tooling, existing PCB designs), the current 1ms / full-speed USB remains the ceiling. The firmware can still be optimised within this constraint (zero debounce, fastest gamepad loop, XInput mode to minimise report parsing overhead) but the 1ms polling floor cannot be bypassed.

---

## Software Stack Compatibility Analysis

GP2040-CE uses the following layers, from hardware upward:

```
[Physical GPIO / buttons]
        │
[RP2040/RP2350 Pico SDK]  ← RP-specific; requires replacement
        │
[TinyUSB]                 ← Portable; STM32H7/iMX RT are first-class targets
        │
[GP2040-CE USB descriptors / HID reports]  ← Portable (pure C++)
        │
[GP2040-CE gamepad logic / add-ons]         ← Portable (pure C++)
        │
[Web configurator (React/TypeScript)]       ← Fully portable; no MCU dependency
```

### What Would Port Easily

- **TinyUSB layer**: HID descriptors, report format, XInput/PS4/HID driver — these are portable across any TinyUSB-supported target. Critically, `headers/tusb_config.h` lines 64–70 **already contains conditional HS speed logic**:

  ```c
  #if (CFG_TUSB_MCU == OPT_MCU_LPC18XX || CFG_TUSB_MCU == OPT_MCU_LPC43XX || \
       CFG_TUSB_MCU == OPT_MCU_MIMXRT10XX || CFG_TUSB_MCU == OPT_MCU_NUC505 || \
       CFG_TUSB_MCU == OPT_MCU_CXD56)
    #define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_HIGH_SPEED
  #else
    #define BOARD_DEVICE_RHPORT_SPEED   OPT_MODE_FULL_SPEED
  #endif
  ```

  `OPT_MCU_MIMXRT10XX` (the iMX RT1062) is already in this list. Targeting the iMX RT1062 would automatically enable HS mode in TinyUSB with no `tusb_config.h` changes. The bInterval change (1 = 1ms at FS → 1 = 125µs at HS) is handled automatically by TinyUSB's HS endpoint scheduling.

- **Gamepad logic**: `gamepad.cpp`, `addons/`, `gp2040.cpp` main loop — all pure C++, no RP-specific headers except timing (`to_ms_since_boot`, `time_us_64`). These can be replaced with `HAL_GetTick()` / `DWT->CYCCNT` equivalents.
- **Web configurator**: Fully portable — runs in-browser, communicates via USB CDC/HID or RNDIS. No MCU dependency.
- **Protobuf config**: Portable — nanopb is MCU-agnostic.

### What Would Require Rewriting

- **Pico SDK calls**: `gpio_init()`, `gpio_put()`, `pico/stdlib.h`, `hardware/gpio.h`, PIO programs, SPI/I2C via hardware peripherals. These require HAL-equivalent calls for the target MCU.
- **Flash storage**: RP uses XIP flash via the Pico SDK. STM32/iMX RT use their own flash abstraction (EEPROM emulation / external SPI flash).
- **Build system**: `pico_sdk_import.cmake` is RP-specific. CMake structure would remain but the toolchain file and SDK import change.
- **USB boot / UF2 flashing**: The RP bootloader's UF2 drag-and-drop would not be available. Replacement: STM32 DFU (built-in ROM bootloader, works with `dfu-util`) or iMX RT HID bootloader (Teensy-style).

### Estimated Port Effort

| Component | Effort |
|---|---|
| TinyUSB descriptor changes for HS | Low (hours) |
| Gamepad loop / report logic | Low (days) — mostly timing function substitution |
| GPIO abstraction layer | Medium (days) — create HAL wrapper, replace `hardware/gpio.h` calls |
| Flash / NVM storage | Medium (days) — re-implement with target HAL |
| Build system / CMake | Medium (days) — new toolchain file, SDK import |
| PIO-based features (WS2812 LEDs, JTAG, etc.) | High (weeks) — RP PIO programs have no direct equivalent; use DMA + timer on STM32/iMX |
| Web configurator | None — fully portable |

A full port would realistically take several weeks of engineering time and represent a significant architectural change. A more practical approach may be to create a **parallel build target** that shares the upper layers (TinyUSB HID + gamepad logic) while abstracting the lower layers behind a hardware HAL.

---

## Recommendation

### Option A: Switch MCU to iMX RT1062 (Cleanest Path to USB 2.0 HS)

**Recommended MCU: NXP i.MX RT1062** (or STM32H7 if external PHY is acceptable)

- Integrated USB 2.0 HS PHY — no extra chip
- TinyUSB first-class support; `tusb_config.h` already lists `OPT_MCU_MIMXRT10XX` in its HS block — enabling HS is automatic when targeting this MCU
- ARM Cortex-M7 @ 600 MHz (same ISA family as RP2350's M33)
- Proven in gamepad applications (Teensy 4.x)
- Achieves 125µs polling interval

This is a significant but clean platform migration. The upper layers (TinyUSB HID, gamepad logic, web configurator) are preserved. A hardware abstraction layer is required to isolate the RP-specific SDK calls.

### Option B: RP + FX2LP Dual-MCU (Keep RP, Add HS)

**For community boards that want USB 2.0 HS while keeping the RP ecosystem:**

Add an Infineon EZ-USB FX2LP (CY7C68013A, ~$3) to the PCB alongside the RP chip. The RP handles all gamepad logic and sends 12-byte HID reports over SPI. The FX2LP presents a USB 2.0 HS HID device to the host.

- RP2040/RP2350 firmware is largely unchanged
- FX2LP firmware is a small dedicated codebase (~200 lines for USB HID + SPI read)
- Achieves 125µs polling — identical to Option A from the host's perspective
- Added cost: ~$3–5 BOM, larger PCB, two firmware images to maintain
- All USB modes (XInput, HID, PS4) must be reimplemented in FX2LP firmware

This is architecturally messier than a clean MCU migration but preserves the RP ecosystem for GPIO, add-ons, and community tooling.

### Option C: Stay on RP, Accept Full-Speed

If staying on RP with a single chip is a hard requirement, the current 1ms / full-speed USB remains the ceiling. Optimise within the constraint:
- Zero debounce mode
- Minimise gamepad loop overhead
- XInput mode to minimise report parsing overhead on the host

### On "USB 3 Gamepad" as a Product Claim

A "USB 3 gamepad" achieves the same 125µs polling as USB 2.0 HS. The bandwidth difference (5 Gbps vs. 480 Mbps) is entirely irrelevant for 12-byte HID reports. The added complexity of USB 3 SS silicon (FX3 over FX2LP, for example) is not justified for this application.

**The correct framing is: "USB 2.0 High-Speed HID with 125µs polling"** — which is an honest, verifiable claim that directly addresses the community's latency concerns without overstating capabilities.

---

## Test Matrix Additions (If Implemented)

If USB 2.0 HS is implemented on a new platform, the latency test framework (see `latency-testing-framework.md`) would add:

| Variable | Additional Values |
|---|---|
| USB speed | Full-Speed (1ms), High-Speed (125µs) |
| MCU platform | RP2040, RP2350A, STM32H743, iMX RT1062 |

The logic analyzer methodology is unchanged — USB HS D+/D- signals are captured at the same probe points. The sniffer sample rate must be ≥ 5 MHz to resolve 125µs intervals (the recommended 24 MHz is sufficient).

---

## Open Questions

1. **HAL abstraction layer**: Is it worth investing in a hardware abstraction layer now that would enable multi-MCU targets? This is a significant architectural investment but would future-proof the project against RP silicon limitations.

2. **Dual-target build**: Could the project maintain both RP (community breadth, $4 cost, existing hardware) and iMX RT / STM32H7 (premium HS performance) as simultaneous build targets with a shared codebase? This is architecturally appealing but doubles maintenance burden.

3. **USB 2.0 HS compliance testing**: USB HS eye diagram compliance (per USB-IF spec) requires access to a USB compliance test fixture. Community builds would need to meet HS signal integrity requirements — achievable on a well-designed PCB but not trivial.

4. **Host compatibility**: USB 2.0 HS HID gamepad compatibility with all major hosts (Windows, Linux, Android, console adapters) should be verified. USB 3 ports are backward-compatible, but host USB hub topologies sometimes downgrade HS devices.

5. **Power**: USB 3 capable ports supply 900mA vs. 500mA for USB 2. For a controller that adds wireless radios or high-brightness LEDs, the higher current budget could be relevant.

---

## Acceptance Criteria (If Moving to Implementation)

- [ ] MCU selection finalized with USB HS confirmed functional
- [ ] TinyUSB HS HID profile verified on target MCU (125µs bInterval confirmed on host)
- [ ] Hardware abstraction layer defined and documented
- [ ] GPIO abstraction layer implemented for target MCU
- [ ] Flash/NVM abstraction implemented
- [ ] Gamepad loop boots and reports correctly on new platform
- [ ] Latency test rig results published: FS baseline vs. HS with new platform
- [ ] UF2 / DFU flashing workflow documented for new platform
- [ ] Web configurator verified functional over USB CDC/RNDIS on new platform
