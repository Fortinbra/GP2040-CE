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

## Governance

- All meaningful changes require team consensus
- Document architectural decisions here
- Keep history focused on work, decisions focused on direction
