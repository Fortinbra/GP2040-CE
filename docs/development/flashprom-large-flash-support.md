# FlashPROM Large-Flash Support

## Status

Proposal — implementation not yet started. No migration path in v1; flashing this on an existing > 4 MB board requires a full flash erase (nuke).

## Problem

`lib/FlashPROM/src/FlashPROM.h` hardcodes the on-flash EEPROM region for every board:

```cpp
#define EEPROM_SIZE_BYTES    0x8000        // 32 KB
#define EEPROM_ADDRESS_START _u(0x101F8000) // XIP_BASE + 0x1F8000 (~2 MB - 32 KB)
```

This location was inherited from arduino-pico for cross-tooling compatibility. It works on every existing GP2040-CE board because they all target chips with **4 MB of declared flash** (`pico`, `pico_w`, `pico2`, `pico2_w`).

When a board declares larger flash — observed first on **PimoroniPicoLipo2XLW** with `PICO_FLASH_SIZE_BYTES = 16 * 1024 * 1024` and a custom `PICO_BOARD` header — the RP2350 boot/partition path writes metadata that erases the page at `0x101F8000` on reboot. Symptoms:

- Webconfig "Save" succeeds and the values write to flash.
- Power-cycle.
- All config values are back to defaults; the EEPROM page has been wiped.

Confirmed by reverting `PICO_BOARD` to stock `pico2_w` (4 MB declared). Persistence returns. See [build investigation in repo memory](../../.copilot/) and the prior session log.

## Goal

Allow boards with > 4 MB flash to declare their true flash size and use it, **without** breaking config persistence.

## Non-Goals (v1)

- **No automatic migration** of existing EEPROM contents from `0x101F8000` to a new location.
- **No reclamation** of the ~2 MB "hole" below `0x101F8000` on existing 4 MB boards. Their layout is unchanged.
- **No change** to FlashPROM's public API (`EEPROM.start()`, `EEPROM.commit()`, `EEPROM.reset()`, `EEPROM.writeCache[]`).

A migration path for shipped > 4 MB boards is **deferred to a follow-up before launch**.

## Decision

Make the EEPROM address a **per-board override**, with the legacy value as the default.

- FlashPROM keeps `EEPROM_ADDRESS_START = 0x101F8000` as the default, used by every board that does not override it.
- A board that needs a different location sets `EEPROM_ADDRESS_START` (and optionally `EEPROM_SIZE_BYTES`) as a **compile definition** in its `<Board>.cmake` (e.g. `add_compile_definitions(EEPROM_ADDRESS_START=0x10FF8000U)`). FlashPROM's header guards the macro so the board override wins in every translation unit, including `FlashPROM.cpp` which does not include `BoardConfig.h`.
- `EEPROM_ADDRESS_START` must be page-aligned (256 B) and `EEPROM_SIZE_BYTES` must be a multiple of the flash sector size (4 KB) — same constraints as today.

This is the smallest invasive change that unblocks the Pimoroni board and any future > 4 MB board.

### Default (unchanged)

```cpp
// lib/FlashPROM/src/FlashPROM.h
#ifndef EEPROM_SIZE_BYTES
#define EEPROM_SIZE_BYTES    0x8000
#endif

#ifndef EEPROM_ADDRESS_START
#define EEPROM_ADDRESS_START _u(0x101F8000)
#endif
```

### Board override (PimoroniPicoLipo2XLW)

Target location: **top of declared flash, minus the EEPROM region**. With 16 MB flash:

```
XIP_BASE        = 0x10000000
PICO_FLASH_SIZE = 0x01000000  (16 MB)
EEPROM_SIZE     = 0x00008000  (32 KB)
EEPROM_ADDRESS  = 0x10FF8000  (XIP_BASE + 16 MB - 32 KB)
```

Set in the board's CMake file as a compile definition (visible in all TUs, including `FlashPROM.cpp`):

```cmake
# configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake
add_compile_definitions(EEPROM_ADDRESS_START=0x10FF8000U)
```

Rationale for top-of-flash placement on this board:

- Above the RP2350 firmware image and any SDK-placed metadata.
- Below the partition-table region the RP2350 bootrom may write — placement here has been observed safe in other RP2350 projects; will be validated by save/reboot test on hardware.
- Leaves the maximum contiguous flash range available for future use (firmware growth, asset storage, etc.).

If hardware testing shows the top-of-flash page is also touched by the bootrom, the fallback is to move EEPROM down by a few additional 4 KB sectors and re-test. The doc will be updated with the verified address.

### Linker FLASH region

The custom board header declares the chip as 16 MB at the C level (`PICO_FLASH_SIZE_BYTES = 16 * 1024 * 1024`) but also carries `pico_cmake_set_default PICO_FLASH_SIZE_BYTES = (4 * 1024 * 1024)`, so the SDK CMake variable remains 4 MB unless explicitly overridden. As a result:

- Generated `pico_flash_region.ld` keeps `LENGTH = 4 MB`. The firmware image partition is bounded to the lower 4 MB.
- FlashPROM operates outside the linker FLASH region via the bootrom flash APIs, so the linker length does not affect EEPROM read/write at `0x10FF8000`.
- This is the conservative starting point. If a future need to grow the firmware image past 4 MB arises, `PICO_FLASH_SIZE_BYTES` can be set in the board CMake to expand the linker region; that change must be re-tested against the EEPROM location.

## Board Targeting

- `PICO_BOARD = pimoroni_pico_lipo2xl_w` (custom board header at `configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h`, declaring `PICO_FLASH_SIZE_BYTES = 16 * 1024 * 1024`).
- `PICO_PLATFORM = rp2350-arm-s`.
- `PICO_NUM_GPIOS = 48` (RP2350B).
- `EEPROM_ADDRESS_START = 0x10FF8000` (in `BoardConfig.h`).

The temporary diagnostic revert to `PICO_BOARD = pico2_w` will be undone as part of implementation.

## Upgrade Requirement (v1)

For any board with > 4 MB flash being upgraded to a firmware that includes this feature:

- **Flash must be fully erased before loading the new UF2.** Existing saved config at `0x101F8000` will not be migrated.
- Recommended method: hold BOOTSEL, drag `flash_nuke.uf2`, then load the GP2040-CE UF2.
- This requirement does **not** apply to existing 4 MB boards (Pico, Pico W, Pico 2, Pico 2 W default configurations) — their EEPROM location is unchanged.

This will be called out in release notes and in the PimoroniPicoLipo2XLW board documentation.

## Out-of-Scope / Follow-Up

- **EEPROM migration**: a future change will detect a populated legacy region at `0x101F8000`, copy it to the new per-board location, and clear the old page atomically on first boot. Required before public release for any > 4 MB board that ever shipped with the legacy layout.
- **Reclaim the 2 MB hole on 4 MB boards**: separate proposal. Would require the same migration mechanism plus a FlashPROM API revision.
- **Explicit RP2350 partition table**: out of scope here. If we later need to reserve flash regions explicitly for the bootrom, that becomes its own proposal.

## Implementation Plan

1. Make `EEPROM_SIZE_BYTES` and `EEPROM_ADDRESS_START` overridable in `lib/FlashPROM/src/FlashPROM.h` via `#ifndef` guards. Default values unchanged.
2. Restore `PICO_BOARD = pimoroni_pico_lipo2xl_w` and `PICO_FLASH_SIZE_BYTES = 16 MB` in the PimoroniPicoLipo2XLW configuration. Remove the temporary `pico2_w` diagnostic.
3. Add `add_compile_definitions(EEPROM_ADDRESS_START=0x10FF8000U)` in `configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake`.
4. Clean-slate Ninja configure + full build for PimoroniPicoLipo2XLW.
5. Clean-slate Ninja configure + full build for at least one stock board (`Pico` default) to confirm no regression in the default path.
6. Hardware validation on PimoroniPicoLipo2XLW: full flash erase, load new UF2, save a config change, power-cycle, verify persistence.
7. Update [docs/development/rp2350-support.md](rp2350-support.md) and the PimoroniPicoLipo2XLW notes with the v1 upgrade requirement.

## Validation Checklist

- [ ] Default FlashPROM build (Pico) produces identical EEPROM address (`0x101F8000`) — verify in `.elf.map`.
- [ ] PimoroniPicoLipo2XLW build resolves EEPROM to `0x10FF8000` — verify in `.elf.map` and `storagemanager` references.
- [ ] Save → power-cycle → values persist on PimoroniPicoLipo2XLW hardware.
- [ ] Stock Pico build still loads existing saved configs from `0x101F8000`.
- [ ] Clean-slate build succeeds for both targets.

## References

- [lib/FlashPROM/src/FlashPROM.h](../../lib/FlashPROM/src/FlashPROM.h)
- [configs/PimoroniPicoLipo2XLW/BoardConfig.h](../../configs/PimoroniPicoLipo2XLW/BoardConfig.h)
- [configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake](../../configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake)
- [configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h](../../configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h)
- [docs/development/rp2350-support.md](rp2350-support.md)
