# Squad Decisions

## Active Decisions

### 2026-03-28T015800: Copilot instructions created
**By:** Roy Mustang  
**What:** Created `.github/copilot-instructions.md` covering code style (4-space indent, no tabs), Pico SDK 2.1.1, build config, project structure, and strict branching policy (no direct main commits, no upstream commits).  
**Why:** Requested by Fortinbra to guide Copilot contributions and establish consistent coding standards for AI-assisted development.

### 2026-03-28T015800: User directive
**By:** Fortinbra (via Copilot)  
**What:** All changes must be made on a branch. Never commit directly to main. Absolutely never commit to upstream.  
**Why:** User request — captured for team memory

### 2026-03-28T021300: Dependency management doc created
**By:** Maes Hughes
**What:** Created docs/development/dependency-updates.md covering all GP2040-CE dependencies (firmware + web configurator) with update procedures and compatibility notes.
**Why:** Feature doc requested by Fortinbra before PR

### 2026-03-28T024429: SDK version 2.2.0 confirmed as project-wide ground truth

**By:** Edward  
**What:** CMakeLists.txt line 7 sets `sdkVersion 2.2.0` and lines 62-63 enforce a FATAL_ERROR if SDK < 2.2.0. CI (`cmake.yml`) checks out pico-sdk at tag `2.2.0` explicitly. This is the project-wide minimum for ALL builds — RP2040 and RP2350 alike. All documentation must reference 2.2.0, not 2.1.1.  
**Impact:** `copilot-instructions.md`, `rp2350-support.md`, and `dependency-updates.md` all updated to 2.2.0. Picotool version confirmed as `2.2.0-a4` (exact CMakeLists.txt value).  
**Why:** Riza review rejection — SDK version conflict between docs and copilot-instructions.md required resolution. Ground truth is the build system and CI, not developer guidance docs.

### 2026-03-28T024429: All internal AI agent names removed from public-facing docs

**By:** Fortinbra  
**What:** Internal Squad agent names (e.g., "Roy Mustang", team member names) must not appear in any public-facing documentation. Maintainer fields should attribute "GP2040-CE core team" or similar project-level attribution.  
**Why:** Riza review round 4 & 5 — AI tooling references are inappropriate in public docs. Governance decision by Fortinbra.

### 2026-03-28T021300: RP2350 support doc created

**By:** Maes Hughes (from Edward's analysis)

**What:** Created `docs/development/rp2350-support.md` covering:
- Hardware comparison table (RP2350A vs RP2350B)
- Supported boards and their configurations
- Minimum requirements and Pico SDK version
- Build instructions for pre-built releases and from source
- Pin mapping and GPIO validation
- Complete guide for creating custom RP2350 board configurations
- Migration path for users upgrading from RP2040
- Known limitations and testing notes

**Finding:** RP2350 support is already partially implemented in the codebase:
- Three board configs exist and are CI-tested: `Pico2`, `FlatboxRev8`, `SparkFunProMicroRP2350`
- Firmware source code is platform-agnostic (zero `#ifdef` guards for chip-specific logic)
- GPIO validation uses `NUM_BANK0_GPIOS` SDK macro—automatically adapts at compile time
- Web configurator auto-detects GPIO count from running firmware

Documented gaps:
1. **Pico2W config missing** — Raspberry Pi Pico 2 W (RP2350A + WiFi) not yet supported
2. **No RP2350B GPIO 30–47 configs** — RP2350B capabilities documented but no board configs currently leverage extra pins
3. **PIO USB timing** — Tested in CI but hardware pass-through testing recommended at 150 MHz clock

**Why:** Feature doc requested by Fortinbra to explain RP2350 support to end users and guide board creators on custom configurations.

**Commit:** `bf3d2f4d` (docs/copilot-instructions branch)

### 2026-03-28T024429: Bluetooth and Multi-Output Architecture — Constraints from Edward's Survey

**By:** Edward  
**What:** Deep codebase survey identified 8 architectural constraints and findings for multi-output support:
1. GPDriver is USB-only; output transport abstraction needed above it
2. No runtime output switching exists; mode selection is boot-time only
3. USB HID and CYW43 Bluetooth CAN coexist on PicoW (separate hardware)
4. GPIO retro console OUTPUT does not exist (only INPUT adapters present)
5. Zero Bluetooth code in firmware; feature is entirely new
6. Pico2W (RP2350 + CYW43) missing pending CYW43 wireless stack porting to RP2350
7. Output transport abstraction layer required before BT or GPIO can be peer output modes
8. InputMode enum is source of truth for output modes; new mode requires protobuf change + web configurator cascade

**Why:** Requested by Fortinbra as technical foundation for Bluetooth feature planning documentation. Findings form the basis for Hughes's feature doc and Edward's final approval by Riza.

### 2026-03-28T024429: GPIO Output Architecture Constraints

**By:** Edward (via survey for Hughes's GPIO output feature doc)  
**What:** Deep technical survey of GPIO retro console OUTPUT mode established the following architectural constraints and decisions:

1. **GPIOOutputAddon is a `GPAddon` subclass, not a `GPDriver`** — enables simultaneous USB HID + GPIO retro output. GPIO output runs in `addon->process()` after USB report is sent per main loop iteration.

2. **No new `InputMode` enum value required** — GPIO output protocol is selected within the addon config, not at the DriverManager/InputMode level. Avoids cascade changes to protobuf, DriverManager, and web configurator mode selector.

3. **Level shifting is mandatory for NES, SNES, Genesis/MD, TurboGrafx-16/PC Engine** — these consoles use 5V TTL. RP2040/RP2350 GPIO inputs are NOT 5V tolerant. Hardware requirement: 74AHCT125 (output) + 74LVC245 or resistor divider (input). This is a hardware design constraint to document.

4. **N64 and Dreamcast are 3.3V native** — no level shifting needed. RP2040/RP2350 GPIO compatible out of the box.

5. **N64 output requires PIO — non-negotiable** — 1 MHz NRZ serial, 500 ns/bit tolerance. Bit-bang and interrupt approaches fail at this tolerance. One PIO state machine needed.

6. **Dreamcast MAPLE bus requires PIO — non-negotiable** — ~2 Mbps, bidirectional, CRC-checked frames, <200 µs response window. Two PIO SMs needed (TX + RX). Most complex retro console target. No Dreamcast code exists in the project today.

7. **SNES/NES output: interrupt-driven is viable, PIO is optional** — timing window is 6–12 µs per clock edge. GPIO IRQ response latency on RP2040 is typically <1 µs. Both approaches achievable; PIO provides jitter-free guarantee.

8. **Genesis/MD and TurboGrafx-16: bit-bang fully sufficient** — parallel protocols with slow console polling rates (60 Hz). No PIO needed.

9. **PIO state machines are not a bottleneck** — RP2040 has 8 total. Worst case (PIO-USB passthrough + WS2812 + N64 + Dreamcast) = 8 SMs fully consumed. More typical retro-output board (no USB passthrough) uses at most 4. RP2350B has 12 SMs.

10. **Pico W loses GPIO 23–25 to CYW43** — pins are used for CYW43 SPI/SDIO bus. Only GPIO 22, 26, 27, 28 are free candidates (4 pins). Sufficient for NES/SNES/N64/TG16/Dreamcast; tight for Genesis 6-button (needs 9 signal pins).

11. **SNES protocol timing ground truth** — from `lib/SNESpad/SNESpad.cpp`: Latch HIGH = 12 µs, setup delay = 6 µs, clock LOW = 6 µs, clock HIGH = 6 µs. Frame = 16 bits (SNES) / 8 bits (NES). Data active-low.

12. **New protobuf fields needed** — `GPIOOutputOptions` message in `proto/config.proto` with fields: enabled, protocol enum, per-protocol pin assignments. Wire into `AddonOptions` alongside existing `SNESOptions`, `TG16Options`.

**Why:** Requested by Fortinbra as technical foundation for Hughes's GPIO output feature documentation. Full analysis in `.squad/agents/edward/gpio-analysis.md`.

### 2026-03-28T04:14: RP2350 + BTStack confirmed working

**By:** Fortinbra  
**What:** RP2350 with CYW43 wireless fully supports BTStack — confirmed by Fortinbra who has built a working BT controller project on the Pimoroni Pico Lipo 2 XL W (RP2350 + CYW43). The previous note that RP2350 CYW43 BT was blocked/unverified is incorrect and must be removed from docs.  
**Also:** TinyUSB and BTStack have conflicting namespaces — this must be documented as a known integration concern in bluetooth-support.md.  
**Why:** User-confirmed hardware fact — supersedes prior analysis uncertainty.

### 2026-03-28T04:20: Pimoroni Pico Lipo 2 XL W as BT reference board

**By:** Fortinbra  
**What:** The Pimoroni Pico Lipo 2 XL W (RP2350 + CYW43 + onboard LiPo charger) is the designated reference board for initial Bluetooth development and testing.  
**Why:** User-confirmed working BT target with battery hardware present.

### 2026-03-28T04:20: BT must include battery level reporting

**By:** Fortinbra  
**What:** Bluetooth feature must include battery level reporting (BT HID Battery Service, UUID 0x180F). Battery percentage must be reported to the connected host over BT.  
**Why:** Wireless devices must report battery state; this is expected by all modern OSes.

### 2026-03-28T04:20: Power management required for BT/battery builds

**By:** Fortinbra  
**What:** GP2040-CE must implement power management for wireless (battery-powered) builds. USB builds always have VBUS so power management was never needed. BT+battery builds require: sleep/dormant modes when idle, CYW43 radio power saving, VBUS detection to switch power profiles between USB and battery operation.  
**Why:** First time the firmware must manage its own power budget — foundational for any battery-powered use.

### 2026-03-28T17:05: User directive — RM2 module support for custom boards (future feature)
**By:** Fortinbra (via Copilot)
**What:** Future feature — custom RP2040/RP2350 boards should be able to use the Pimoroni RM2 module (CYW43439 standalone wireless module) for BT/WiFi, provided they wire the RM2 to the same GPIO pins that Pico W / Pico 2 W use for their onboard CYW43. This enables BT on custom boards without requiring a Pico W form factor.
**Reference:** https://shop.pimoroni.com/products/rm2-breakout?variant=53492995719547
**Why:** Expands wireless capability to the broader ecosystem of custom GP2040-CE boards.

### 2026-03-28T234414: Deferred Dependency Upgrade Planning Documentation

**By:** Maes Hughes (authored), Riza (approved)  
**What:** Four feature planning documents created to guide future contributors through deferred dependency upgrades that require significant manual effort:

1. **tinyusb-upstream-port.md** — Plan to port TinyUSB fork (0.17.0 + 18 commits) to upstream 0.20.0
2. **pico-pio-usb-upstream-port.md** — Plan to port pico-pio-usb fork (0.5.3 + 10 commits) to upstream 0.7.2
3. **nanopb-stable-migration.md** — Plan to migrate vendored nanopb from 0.4.8-dev to 0.4.8 stable
4. **npm-major-upgrades.md** — Five-phase roadmap for npm major version upgrades in web configurator

**Technical Facts Verified:**
- SDK 2.2.0 referenced throughout (matches squad decision 2026-03-28T024429)
- TinyUSB: 0.17.0 base + 18 custom commits verified; upstream 0.20.0 correct
- pico-pio-usb: 0.5.3 base + 10 custom commits verified; upstream 0.7.2 correct
- nanopb: 0.4.8-dev confirmed as vendored snapshot (not submodule)
- npm packages: All version pairs match confirmed facts (React 18→19, Vite 4→8, react-router-dom 6→7, ESLint 8→9, TypeScript 5→6, Zustand 4→5, protobufjs-cli 1→2)

**Standards Compliance:**
- No internal Squad agent names in any document (uses "GP2040-CE core team" only)
- Documentation complete with problem statements, inventories, goals, phased approaches, risk analysis, and testing requirements
- Ready for merge to main

**Why:** Automated dependency audit identified deferred items that could not be automatically updated due to manual effort requirements. Without clear documentation, these items risk being forgotten, tackled haphazardly, or duplicated. These docs provide clear roadmaps, timelines, and success criteria.

**Commit:** `49bd6797` (feature/dependency-updates branch)

## Governance

- All meaningful changes require team consensus
- Document architectural decisions here
- Keep history focused on work, decisions focused on direction
