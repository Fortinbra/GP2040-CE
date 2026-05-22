# FlashPROM Large-Flash Support

## Status

**Deferred (v1).** Investigation completed, hardware test failed for both candidate EEPROM locations. The PimoroniPicoLipo2XLW board ships in v1 with flash declared as 4 MB (`PICO_BOARD=pico2_w`) to keep config persistence working. The infrastructure for per-board EEPROM relocation is in place; a follow-up will revisit declaring the full 16 MB once the bootrom-erase root cause is understood.

## Problem

`lib/FlashPROM/src/FlashPROM.h` hardcoded the on-flash EEPROM region for every board:

```cpp
#define EEPROM_SIZE_BYTES    0x8000        // 32 KB
#define EEPROM_ADDRESS_START _u(0x101F8000) // XIP_BASE + 0x1F8000 (~2 MB - 32 KB)
```

This location was inherited from arduino-pico for cross-tooling compatibility. It works on every existing GP2040-CE board because they all target chips with **4 MB of declared flash** (`pico`, `pico_w`, `pico2`, `pico2_w`).

When a board declares larger flash — observed first on **PimoroniPicoLipo2XLW** with `PICO_FLASH_SIZE_BYTES = 16 * 1024 * 1024` via a custom `PICO_BOARD` header — the RP2350 boot/partition path writes metadata that erases the EEPROM page on reboot. Symptoms:

- Webconfig "Save" succeeds and the values write to flash.
- Power-cycle.
- All config values are back to defaults; the EEPROM page has been wiped.

## Goal (future work)

Allow boards with > 4 MB flash to declare their true flash size and use it, **without** breaking config persistence.

## v1 Decision

Declare the Pimoroni board as 4 MB (`PICO_BOARD=pico2_w`) and keep the legacy EEPROM location (`0x101F8000`). The upper 12 MB of physical flash is unused in v1.

The generic infrastructure for per-board EEPROM relocation has been added and is kept as harmless plumbing — it changes no behavior when a board doesn't override, and it will be needed when the full-flash effort resumes.

### Per-board override mechanism (in place, currently unused)

```cpp
// lib/FlashPROM/src/FlashPROM.h
#ifndef EEPROM_SIZE_BYTES
#define EEPROM_SIZE_BYTES    0x8000
#endif

#ifndef EEPROM_ADDRESS_START
#define EEPROM_ADDRESS_START _u(0x101F8000)
#endif
```

A board that needs a different location can set `EEPROM_ADDRESS_START` (and optionally `EEPROM_SIZE_BYTES`) as a **compile definition** in its `<Board>.cmake`:

```cmake
add_compile_definitions(EEPROM_ADDRESS_START=0x10FF8000U)
```

A compile definition (not a `BoardConfig.h` `#define`) is required because `FlashPROM.cpp` does not include `BoardConfig.h`. The macro must be page-aligned (256 B) and `EEPROM_SIZE_BYTES` must be a multiple of the flash sector size (4 KB).

## Hardware Test Results

Two EEPROM locations were tested on PimoroniPicoLipo2XLW with `PICO_BOARD=pimoroni_pico_lipo2xl_w` (custom header declaring `PICO_FLASH_SIZE_BYTES = 16 MB`). Both failed identically: first boot OK, save succeeds, power-cycle reverts to defaults.

| Attempt | EEPROM address | Build | Save+reboot persistence |
|---|---|---|---|
| Legacy default | `0x101F8000` (~2 MB − 32 KB) | ok | **fails** |
| Top-of-flash | `0x10FF8000` (16 MB − 32 KB) | ok | **fails** |

Both tests were preceded by a full flash nuke. The failure mode is symmetric: whichever page FlashPROM occupies gets clobbered between the save and the next boot.

## Findings

- The erase is **not address-specific**. Moving EEPROM did not help.
- The trigger correlates with **declaring `PICO_FLASH_SIZE_BYTES = 16 MB`** at the C level via the custom board header. Reverting `PICO_BOARD` to stock `pico2_w` (4 MB declared) restores persistence at the legacy `0x101F8000` location.
- Linker FLASH region length is independent of this: even with the linker FLASH region at 4 MB (the stock `pico_cmake_set_default`), declaring 16 MB at the C level is enough to provoke the erase.
- Most likely suspect: the RP2350 bootrom partition/metadata path writing somewhere based on the declared flash size. Confirmation would require dumping flash before and after a reboot.

## Path Forward (not in v1)

When the full-flash effort resumes, candidate next steps:

1. Add a one-shot UART/USB CDC dump of flash regions on boot (post-save) to identify exactly what the bootrom writes and where, on a 16 MB-declared image.
2. Investigate whether an **explicit RP2350 partition table** declared in the binary suppresses the implicit bootrom metadata write, or moves it to a known location FlashPROM can avoid.
3. Re-evaluate whether the upper 12 MB is worth the complexity for the Pimoroni board's use case, or whether keeping 4 MB declared (as in v1) is sufficient indefinitely.

## v1 Board Configuration

```cmake
# configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_DEFAULT_UART 0)
set(PICO_DEFAULT_UART_TX_PIN 0)
set(PICO_DEFAULT_UART_RX_PIN 1)
set(PICO_NUM_GPIOS 48)
```

The `PICO_NUM_GPIOS=48` override is needed because the stock `pico2_w` board header targets RP2350A (30 GPIOs); the Pimoroni board uses RP2350B (48 GPIOs), and GP43 is used by the battery ADC.

EEPROM remains at the default `0x101F8000`. No `add_compile_definitions(EEPROM_ADDRESS_START=...)` is set.

## What Stays in the Tree

- `lib/FlashPROM/src/FlashPROM.h` — `#ifndef` guards around `EEPROM_SIZE_BYTES` / `EEPROM_ADDRESS_START`. No default behavior change for any board.
- This document — captures the investigation, the failure mode, and the rationale for deferring.

## What Was Removed for v1

- `configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h` — the custom 16 MB board header. Preserved in git history (added in commit `de31cb33`, removed alongside this deferral) for the future full-flash investigation.

## References

- [lib/FlashPROM/src/FlashPROM.h](../../lib/FlashPROM/src/FlashPROM.h)
- [lib/FlashPROM/src/FlashPROM.cpp](../../lib/FlashPROM/src/FlashPROM.cpp)
- [configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake](../../configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake)
- [docs/development/rp2350-support.md](rp2350-support.md)
