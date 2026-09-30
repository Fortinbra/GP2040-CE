# FlashPROM Large-Flash Support

## Status

**Reopened for specification review, 2026-09-30.** The requested implementation
order is full-flash correctness first, followed by compile-time capacity flags for
2, 4, 8, and 16 MiB. This pass updates the existing feature document; it does not
enable larger flash or implement flags. Implementation begins in a subsequent
pass under the repository's feature-document gate.

The previous full-flash hardware tests failed at both candidate EEPROM locations.
The erase mechanism remains unconfirmed. Keep the Pimoroni 4 MiB safety policy
until the replacement layout and persistence behavior pass the gates below.

## Accepted Upgrade Policy

On 2026-09-30, the maintainer approved requiring a full flash nuke before installing
the new full-flash firmware on affected larger-flash boards. This pre-1.0 feature
may introduce an incompatible persistent layout; automatic migration and retention
of existing settings across that transition are not required.

BLE has not yet been released, as confirmed by the maintainer on 2026-09-30.
There is no released BLE bond format to preserve and no legacy-bond migration,
restoration, or backward-compatibility requirement for this feature. BLE checks
cover fresh pairing, reconnect, and persistence of bonds created by the new
firmware, not upgrades from pre-release BLE builds.

Release instructions must identify the affected board targets and require the
board-appropriate full flash erase before installation. Warn that the erase removes
settings, profiles, and other data stored in the erased flash. Users must configure
the controller after installation. Recommend exporting
supported configuration beforehand, but do not promise backup import compatibility
without validation. A raw old flash dump is not a supported
restore method into the new layout.

This policy does not require a nuke for unaffected targets or every subsequent
update. Updates within the same validated layout must retain saved state. A
downgrade to an incompatible layout requires another nuke and clean installation.
Interrupted erase/install recovery is a repeat of the board's documented recovery
and clean-install procedure, not an automatic migration rollback.

Approval of this release policy is not permission to erase a connected device
without explicit hardware-test consent. It also does not establish full-flash
correctness: the historical persistence failures reportedly followed full nukes.

## Scope and Units

- Sizes here are binary **MiB**, not megabits: 2, 4, 8, and 16 MiB correspond to 2,097,152, 4,194,304, 8,388,608, and 16,777,216 bytes.
- Preserve `GP2040_BOARD` as the only public board selector. Flash capacity and build flags derive from its resolved hardware definition and validated policy; they are not independent user-selectable size overrides.
- First establish safe access to the full physical flash, a usable application layout, and persistence. Then expose the capacity contract for future feature selection. Merely increasing `PICO_FLASH_SIZE_BYTES` is insufficient.
- Do not add or remove firmware features in this change. Future feature budgets must account for reserved storage, image overhead, RAM, and hardware capability, rather than assuming that a flash tier alone makes a feature usable.
- Preserve controller defaults, board identity, battery wiring, and BLE behavior. Post-build provisioning remains separate work.

## Current Implementation

The unified selector uses explicit target records and SDK-compatible hardware
headers. Pimoroni now resolves to `gp2040_pimoroni_lipo2xlw`, a project-owned header
inheriting `boards/pico2_w.h`. It corrects RP2350B package selection and guards the
effective flash declaration at 4 MiB. The former `PICO_NUM_GPIOS=48` setting did
not select RP2350B and must not be restored.

Pico and Pico W declare 2 MiB; Pico 2 and Pico 2 W declare 4 MiB. Some existing SDK
targets, including PimoroniPicoPlus2 and SparkFunProMicroRP2350, already declare
16 MiB. Those declarations and successful builds are not evidence of full-flash
runtime qualification.

There are two independent limits to address:

1. The hardware/effective capacity declaration, including Pimoroni's 4 MiB guard.
2. The current application/storage layout: `modules/board_storage.ld.in` rejects any image ending above `0x101F8000`, even on larger-flash targets.

`modules/BoardCapabilities.cpp.in` additionally requires the legacy 32 KiB EEPROM
reservation at `0x101F8000`. `FlashPROM::start()` copies that region to a static
cache; its delayed commit erases and programs the same region using an XIP-relative
offset. Neither operation chooses a different address based on declared capacity.
The configuration footer is located relative to the end of the EEPROM reservation.
Consequently, layout changes must update all owners coherently and explicitly
address existing stored data.

## Stage 1: Full-Flash Correctness

### Reproduce Before Choosing a Fix

Use the corrected Pimoroni RP2350B hardware definition as the controlled baseline.
Compare the current 4 MiB policy with a diagnostic 16 MiB variant, holding GPIO,
ADC, wireless wiring, and controller defaults constant. Do not change production
defaults simply to create the diagnostic build.

Capture the complete layout and write ownership: linked image, image/partition
metadata, configuration storage, BLE bond storage, and any SDK/library flash banks.
Inspect ELF/map/UF2 metadata and physical flash identification. Record which
firmware operations erase or program each region.

Capture configuration bytes and validity/footer state after a completed save,
before reset, after reset, and after a true power cycle. Separate a write that
never completed from an actual erase, invalid data rejected during loading, or
an intentional reset to defaults. BLE bonds need equivalent persistence evidence.
Obtain a known-good backup first; never publish bond contents or other secrets in
test logs. Do not flash, erase, or overwrite hardware without explicit consent.

The falsifiable initial hypothesis is that a capacity-dependent storage owner or
load/reset path differs between the two builds. Compare snapshots and traced
write addresses to test that hypothesis. A bootrom metadata write is only one
candidate; introduce an explicit partition table only if evidence requires it.

### Layout and Clean-Install Contract

Define one validated layout for the full effective capacity, including application
space and every persistent reservation. Compiler constants, linker limits,
flash-writing code, and generated image metadata must agree. Keep alignment and
non-overlap assertions; replace the fixed-address assertion with stronger layout
checks rather than deleting safety checks.

The legacy reservation currently blocks a contiguous image above roughly 2 MiB.
The accepted clean-install policy permits relocating persistent reservations
without automatic migration or a segmented layout solely to preserve old storage.
Choose and document the new layout before implementation. Claim full application
capacity only when an image extending beyond the old boundary can boot without
consuming the new storage reservations. Every layout must fit 2 MiB targets as
well as the larger capacities it supports.

Treat installation over an incompatible old layout without the required nuke as
unsupported. Do not copy legacy flash contents into the new reservations or add
legacy migration machinery for this feature. Verify erased storage initializes
valid defaults and that subsequent saves, reboots, and same-layout updates retain
state. Document affected targets, erase/install steps, and downgrade/recovery
behavior before release; firmware must not silently perform a destructive nuke.

Keep the existing 32 KiB configuration format unless evidence demands a separately
justified schema change. Determine actual BLE bond ownership before deciding
whether bonds share the configuration reservation or require their own storage
policy. Freshly established bonds must persist under the new layout.

## Stage 2: Capacity Compile Flags

After Stage 1 qualifies the layout, expose a centrally derived capacity contract:

| Definition | Meaning |
| --- | --- |
| `GP2040_FLASH_SIZE_BYTES` | Effective enabled capacity, equal to the validated SDK/compiler flash size. |
| `GP2040_FLASH_SIZE_MIB` | Exact numeric capacity: 2, 4, 8, or 16. |
| `GP2040_FLASH_2MIB` | 1 only for the 2 MiB tier; otherwise 0. |
| `GP2040_FLASH_4MIB` | 1 only for the 4 MiB tier; otherwise 0. |
| `GP2040_FLASH_8MIB` | 1 only for the 8 MiB tier; otherwise 0. |
| `GP2040_FLASH_16MIB` | 1 only for the 16 MiB tier; otherwise 0. |

All four tier flags are numeric and exactly one is 1. Use `#if`, not `#ifdef`,
to test them. For minimum-capacity requirements, compare the numeric size:

```cpp
#if GP2040_FLASH_SIZE_MIB >= 8
// Future feature implementation requiring an 8 MiB or larger build.
#endif
```

Generate these values once in the owning build layer and make them consistent
across firmware and any compiled libraries that consume them. Do not duplicate
capacity tables across board records, source headers, and CI. This shared contract
is an intentional addition to the current SDK-macro-only pattern, justified by
future CMake source selection and C/C++ feature gating.

Validate exact supported sizes and reject conflicting overrides or unsupported
capacities explicitly. A physically 16 MiB board still limited to 4 MiB must report
the 4 MiB effective tier, never advertise unavailable capacity. Tier size is not
the application byte budget; reservations remain excluded from linker space.
Expose a clear configure summary of physical capacity when known, effective
capacity, application limit, storage reservations, and selected tier.

## Acceptance and Validation

1. Reproduce and explain the earlier 16 MiB failure, or document controlled evidence showing why it no longer reproduces. A successful compile cannot close it.
2. With explicit hardware consent, verify read/erase/program at bounded scratch sectors in upper flash, including near the declared end, without touching images, metadata, configuration, or bonds. Confirm address operations do not wrap or alias lower flash. Restore scratch contents after testing.
3. Boot a diagnostic image exercising application placement beyond the old EEPROM boundary, with linker/image checks proving that persistent regions are intact.
4. Verify save completion, soft reboot, repeated power cycles, configuration reload, BLE pairing/reconnect/bond retention, and GP43 battery sensing on Pimoroni.
5. Starting with existing firmware and saved settings on an affected larger-flash board, perform the documented nuke and clean installation. Verify old state is cleared, valid defaults load, and newly saved settings persist. Validate fresh BLE pairing and bond persistence on the new firmware separately; no pre-release BLE upgrade test is required. Test same-layout updates without a nuke and the documented interrupted-install recovery/incompatible-layout downgrade procedures. Preserve the Pimoroni 4 MiB policy until this qualification succeeds.
6. Test all four capacity tiers in CMake and compiler assertions: byte/MiB equality, exactly-one tier, threshold behavior, consistent library definitions, unsupported sizes, conflicting overrides, reservation alignment, and image/storage bounds. Use a clearly test-only 8 MiB fixture if no suitable registered board exists; fixture success is not physical 8 MiB qualification.
7. Run fresh configure and full clean Ninja builds using SDK 2.3.1 in `build/`, covering RP2040/RP2350, wireless/non-wireless targets and all capacity tiers, including at least one web-inclusive build. Validate produced UF2/ELF metadata.
8. Keep capacity-dependent web assets out of this change. If future feature gating changes embedded web content, revise the shared CI `fsData` contract explicitly.

No new feature is enabled solely to demonstrate the flags. Record build-only
coverage separately from hardware qualification. Existing state may be discarded
only through the explicitly documented clean-install transition; runtime
persistence under the new layout remains mandatory.

## Historical Investigation

The following records preserve the original failure reports. They are not proof
of a bootrom erase or a current implementation recipe.

## Problem

`lib/FlashPROM/src/FlashPROM.h` hardcoded the on-flash EEPROM region for every board:

```cpp
#define EEPROM_SIZE_BYTES    0x8000        // 32 KB
#define EEPROM_ADDRESS_START _u(0x101F8000) // XIP_BASE + 0x1F8000 (~2 MB - 32 KB)
```

This location was inherited from arduino-pico for cross-tooling compatibility.
It fits the 2 MiB declarations used by `pico`/`pico_w` and the 4 MiB declarations
used by `pico2`/`pico2_w`; the earlier claim that all four declare 4 MB was incorrect.

With a historical custom 16 MiB header on **PimoroniPicoLipo2XLW**, configuration
was reported lost on reboot. A boot/partition metadata erase was suspected, not
confirmed by before/after flash captures. Reported symptoms:

- Webconfig "Save" succeeds and the values write to flash.
- Power-cycle.
- All config values are back to defaults; the EEPROM page has been wiped.

## Original Goal

Allow boards with > 4 MB flash to declare their true flash size and use it, **without** breaking config persistence.

## v1 Decision

Declare the Pimoroni board as 4 MB (`PICO_BOARD=pico2_w`) and keep the legacy EEPROM location (`0x101F8000`). The upper 12 MB of physical flash is unused in v1.

The generic infrastructure for per-board EEPROM relocation has been added and is kept as harmless plumbing — it changes no behavior when a board doesn't override, and it will be needed when the full-flash effort resumes.

### Historical Override Mechanism

```cpp
// lib/FlashPROM/src/FlashPROM.h
#ifndef EEPROM_SIZE_BYTES
#define EEPROM_SIZE_BYTES    0x8000
#endif

#ifndef EEPROM_ADDRESS_START
#define EEPROM_ADDRESS_START _u(0x101F8000)
#endif
```

A historical overlay could set `EEPROM_ADDRESS_START` (and optionally
`EEPROM_SIZE_BYTES`) as a compile definition:

```cmake
add_compile_definitions(EEPROM_ADDRESS_START=0x10FF8000U)
```

A compile definition (not a `BoardConfig.h` definition) was required because
`FlashPROM.cpp` does not include `BoardConfig.h`. Erase addresses and lengths must
be sector-aligned (4 KiB), which also satisfies program-page alignment (256 bytes).
Current target records are declarative, and the unified build explicitly rejects
EEPROM relocation. This snippet is historical, not a supported current record.

## Hardware Test Results

Two EEPROM locations were tested on PimoroniPicoLipo2XLW with `PICO_BOARD=pimoroni_pico_lipo2xl_w` (custom header declaring `PICO_FLASH_SIZE_BYTES = 16 MB`). Both failed identically: first boot OK, save succeeds, power-cycle reverts to defaults.

| Attempt | EEPROM address | Build | Save+reboot persistence |
| --- | --- | --- | --- |
| Legacy default | `0x101F8000` (~2 MB − 32 KB) | ok | **fails** |
| Top-of-flash | `0x10FF8000` (16 MB − 32 KB) | ok | **fails** |

Both tests were reportedly preceded by a full flash nuke. Both locations lost
persistence; byte-level evidence is still needed to establish whether the region
was erased, never durably written, or rejected during loading.

## Findings

- The reported persistence failure was not resolved by moving EEPROM.
- The trigger correlates with **declaring `PICO_FLASH_SIZE_BYTES = 16 MB`** at the C level via the custom board header. Reverting `PICO_BOARD` to stock `pico2_w` (4 MB declared) restores persistence at the legacy `0x101F8000` location.
- Linker FLASH region length is independent of this: even with the linker FLASH region at 4 MB (the stock `pico_cmake_set_default`), declaring 16 MB at the C level is enough to provoke the erase.
- Most likely suspect: the RP2350 bootrom partition/metadata path writing somewhere based on the declared flash size. Confirmation would require dumping flash before and after a reboot.

## Original Investigation Candidates

When the full-flash effort resumes, candidate next steps:

1. Add a one-shot UART/USB CDC dump of flash regions on boot (post-save) to identify exactly what the bootrom writes and where, on a 16 MB-declared image.
2. Investigate whether an **explicit RP2350 partition table** declared in the binary suppresses the implicit bootrom metadata write, or moves it to a known location FlashPROM can avoid.
3. Re-evaluate whether the upper 12 MB is worth the complexity for the Pimoroni board's use case, or whether keeping 4 MB declared (as in v1) is sufficient indefinitely.

## Historical v1 Board Configuration

```cmake
# configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake
set(PICO_BOARD pico2_w)
set(PICO_PLATFORM rp2350-arm-s)
set(PICO_DEFAULT_UART 0)
set(PICO_DEFAULT_UART_TX_PIN 0)
set(PICO_DEFAULT_UART_RX_PIN 1)
set(PICO_NUM_GPIOS 48)
```

The intent of `PICO_NUM_GPIOS=48` was to expose RP2350B's GP43 battery ADC.
Unified-board compiler tests subsequently proved it ineffective. The current
project hardware header corrects the SDK package macro instead; do not copy this
historical overlay into a new target.

EEPROM remains at the default `0x101F8000`. No `add_compile_definitions(EEPROM_ADDRESS_START=...)` is set.

## What Stays in the Tree

- `lib/FlashPROM/src/FlashPROM.h` — `#ifndef` guards around `EEPROM_SIZE_BYTES` / `EEPROM_ADDRESS_START`. No default behavior change for any board.
- This document — captures the investigation, the failure mode, and the rationale for deferring.

## What Was Removed for v1

- `configs/PimoroniPicoLipo2XLW/pimoroni_pico_lipo2xl_w.h` — the custom 16 MB board header. Preserved in git history (added in commit `de31cb33`, removed alongside this deferral) for the future full-flash investigation.

## References

- [Unified board selection](unified-board-selection.md)
- [Build hardware/storage validation](../../modules/ValidateBoard.cmake)
- [Compiler capability assertions](../../modules/BoardCapabilities.cpp.in)
- [Current linker storage guard](../../modules/board_storage.ld.in)
- [Current Pimoroni hardware header](../../configs/PimoroniPicoLipo2XLW/hardware/gp2040_pimoroni_lipo2xlw.h)
- [lib/FlashPROM/src/FlashPROM.h](../../lib/FlashPROM/src/FlashPROM.h)
- [lib/FlashPROM/src/FlashPROM.cpp](../../lib/FlashPROM/src/FlashPROM.cpp)
- [configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake](../../configs/PimoroniPicoLipo2XLW/PimoroniPicoLipo2XLW.cmake)
- [docs/development/archive/rp2350-support.md](archive/rp2350-support.md)
