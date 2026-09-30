# Unified Board Selection

**Status:** Implementation authorized by the user's follow-up request on 2026-09-29.
Implemented on `feature/unified-board-selection`; all eight reference clean builds
passed, including the web build. Hardware qualification and persistence checks
remain pending.

**Date:** 2026-09-29

## Goal

Select one GP2040-CE build target and obtain the correct SDK hardware definition,
chip family, wireless capability, flash configuration, and controller defaults.
Support definitions supplied by the pinned Pico SDK and definitions maintained
outside that SDK. Update GitHub Actions and local build entry points together.

Public configure command:

```powershell
cmake -G Ninja -S . -B build --fresh -DGP2040_BOARD=PimoroniPicoLipo2XLW
```

The user must not also have to provide `PICO_BOARD`, `PICO_PLATFORM`, or
`GP2040_BOARDCONFIG`. SDK/toolchain location and build-mode options remain
independent of the board selector.

The follow-up goal is a small set of official hardware-compatible firmware builds
with controller presets applied after compilation. The one-selector migration
below is an intermediate step, not a commitment to retain one release binary per
controller configuration. That work is a separate dependent feature:
[Standard Builds and Post-Build Controller Provisioning](post-build-controller-provisioning.md).

## Feature Dependency

This feature has no dependency on post-build provisioning and can ship with the
existing per-controller release builds intact. It delivers the registered target
records, one-selector resolver, SDK/custom definition support, hardware validation,
and shared CI discovery that the provisioning feature consumes.

The dependent feature owns build-family consolidation, portable presets,
persistent factory defaults, release manifests, and the offline UF2 tool. Those
deliverables are not acceptance gates for this feature. Full-capacity flash
support remains separately gated for both features.

## Confirmed Scope Decisions

Decisions recorded during review on 2026-09-29:

- V1 accepts registered targets with explicit controller defaults. It does not
  accept arbitrary SDK board names with guessed or generic controller mappings.
- Custom target packages may live in-tree or in explicitly supplied external
  directories. Both use the same target-record format and resolver.
- Ship the selector migration with existing flash safety limits. Full-capacity
  larger-flash support remains a separately gated follow-up stage.
- The proposal-writing pass was planning only. The subsequent explicit request
  to branch and implement authorized this accepted scope.

## Build Usage

Use CMake 3.24 or newer, Ninja, Pico SDK 2.3.1, and the Arm toolchain. SDK and
toolchain locations remain environment prerequisites, not board selectors.
Wireless builds also require `pycryptodomex` in the Python interpreter discovered
by CMake; configuration fails without it instead of accepting random GATT hashes.

```powershell
$env:PICO_SDK_PATH = "$env:USERPROFILE/.pico-sdk/sdk/2.3.1"
$env:SKIP_WEBBUILD = 'TRUE'
cmake -G Ninja -S . -B build --fresh -DGP2040_BOARD=Pico2W -DCMAKE_BUILD_TYPE=Release
& "$env:USERPROFILE/.pico-sdk/ninja/v1.12.1/ninja.exe" -C build
```

For a web-inclusive build, set `SKIP_WEBBUILD=FALSE` before configuring. Node.js
and npm must be available; the configure runs `npm ci` and `npm run build`.
Do not set `PICO_BOARD`, `PICO_PLATFORM`, or `GP2040_BOARDCONFIG` for new builds.
Remove old terminal environment defaults before switching targets. `--fresh`
clears CMake's cache, not inherited environment variables.

List registered targets without configuring the SDK or accessing the network:

```powershell
cmake -G Ninja -P modules/ListBoards.cmake
cmake -G Ninja -DGP2040_BOARD=Pico2W -DGP2040_BOARD_OUTPUT=build/boards.json -P modules/ListBoards.cmake
```

The output is a JSON array of target IDs. An empty selection or `all` lists every
target; another value must match a registered target. Firmware configuration does
not accept `all`. The current in-tree registry contains 57 targets. Previously
implicit Pico mappings are now explicit, not guessed from marketing names; this
migration does not hardware-qualify those controller configurations.

The local VS Code tasks use the same command. `Configure CMake (Selected Board)`
and its `with Web` variant prompt for a registry target; the existing named tasks
remain available. Tasks and terminal settings in `.vscode/` are locally ignored,
so these changes do not ship in a checkout. The commands above are the portable
build entry point; the ignore policy is unchanged.

### Target Record Format

Keep `<target>.cmake` and `BoardConfig.h` together in `configs/<target>/`, or in
`<external-root>/<target>/`. A stock SDK mapping needs only:

```cmake
set(PICO_BOARD pico2_w)
```

Optional record fields:

| Field | Meaning |
| --- | --- |
| `PICO_PLATFORM` | Expected SDK-resolved platform, not a global platform default. |
| `GP2040_BOARD_HEADER_DIR` | Package-relative or absolute directory containing `<PICO_BOARD>.h`. |
| `GP2040_BOARD_CMAKE_DIR` | Optional SDK extension directory containing `<PICO_BOARD>.cmake`; use only if hardware needs it. |
| `PICO_DEFAULT_UART`, `PICO_DEFAULT_UART_TX_PIN`, `PICO_DEFAULT_UART_RX_PIN` | Existing target-specific UART policy. |
| `GP2040_BOARD_LEGACY_PICO_BOARD` | Explicit compatibility name for a documented hardware-header correction. |

Records are trusted declarative build inputs: only set these fields. Do not create
targets, initialize the SDK, download files, or depend on caller working-directory
state. Hardware headers must remain independent of generated protobuf, TinyUSB,
and controller definitions. `BoardConfig.h` must supply the controller's required
defaults, including keyboard mappings. A general SDK board name alone is not a
complete controller target.

Example custom mapping:

```cmake
set(PICO_BOARD my_controller_hardware)
set(GP2040_BOARD_HEADER_DIR hardware)
```

Provide `hardware/my_controller_hardware.h` in that package, with a distinctive
SDK-compatible name. Select it with the package folder's target ID:

```powershell
cmake -G Ninja -S . -B build --fresh -DGP2040_BOARD=MyController "-DGP2040_BOARD_DIRS=C:/my boards;D:/other boards"
```

External roots are a CMake list. Relative root paths resolve against the source
directory; hardware paths in a record resolve against the package. Duplicate
target IDs, incomplete nonempty packages, missing headers, SDK-name collisions,
and unexpected SDK-selected header paths fail configuration. Empty directories
are not registered targets. SDK header search paths are preserved, including
inherited environment paths, and the actual selected header is checked.

### Compatibility

`GP2040_BOARDCONFIG` remains a deprecated selector alias for the first release.
Matching legacy hardware selections warn; conflicting selections fail. Explicit
CMake values take precedence over environment values of the same variable.
An invalid supplied selector never falls back to Pico. Only an absent selector
defaults to Pico, with a configure message. An explicitly empty firmware selector
is invalid; empty/all is supported only by the registry listing and CI dispatch.

The build cache records the target, package path, and record hash. Changing that
identity requires `--fresh`; normal same-target reconfiguration is supported.
Artifact suffixes and the runtime `GP2040_BOARDCONFIG` identity remain unchanged.

### Hardware Correction and Safety

The compiler checks exposed that the old Pimoroni `PICO_NUM_GPIOS=48` setting did
not change SDK 2.3.1's RP2350A package macros. It still compiled 30 GPIOs and ADC
base 26, incompatible with the board's GP43/ADC3 battery input.

The project-owned `gp2040_pimoroni_lipo2xlw.h` inherits `boards/pico2_w.h` and changes
only the package macro to RP2350B. It retains CYW43 wiring and the 4 MiB effective
flash limit, with a compile-time guard. The previous `PICO_BOARD=pico2_w` legacy
value is accepted for this target during the transition. The minimal BLE
`BatteryConfig.h` boundary and controller defaults are unchanged.

| Reference Target | GPIOs | ADC Channels / Base | CYW43 | Effective Flash |
| --- | --- | --- | --- | --- |
| Pico, CustomPico | 30 | 5 / 26 | No | 2 MiB |
| PicoW | 30 | 5 / 26 | Yes | 2 MiB |
| Pico2 | 30 | 5 / 26 | No | 4 MiB |
| Pico2W | 30 | 5 / 26 | Yes | 4 MiB |
| SparkFunProMicroRP2350 | 30 | 5 / 26 | No | 16 MiB |
| PimoroniPicoLipo2XLW | 48 | 9 / 40 | Yes | 4 MiB |
| PimoroniPicoPlus2 | 48 | 9 / 40 | No | 16 MiB |

ADC channel counts include the internal temperature channel. SparkFun's pinned
SDK header declares RP2350A, not RP2350B. Existing 16 MiB SDK declarations on other
targets are preserved, not newly enabled for Pimoroni LiPo. Compiler assertions
check GPIO/ADC, wireless and flash agreement, battery availability, and the fixed
EEPROM bounds. Linker assertions keep the image before the EEPROM reservation at
`0x101F8000` with size `0x8000`, even on larger-flash boards. No storage migration,
partition redesign, or full-capacity flash enablement is included.

### Validation Commands

```powershell
cmake -G Ninja -P tests/board-selection.cmake
cmake -G Ninja -P tests/build-board-matrix.cmake
```

The first runs registry/compatibility failures and, when `PICO_SDK_PATH` is set,
real SDK resolution, header shadowing, platform conflict and reused-tree tests.
The second uses `build/` for each reference target, runs `--fresh`, cleans all
compiled outputs, and performs a full Ninja build with hardware expectations.
Pico includes the web build by default. The custom fixture is copied to a path
containing spaces and configured from a different working directory. Logs are
written under `build/board-validation/`.

For a focused rerun, pass `-DGP2040_BUILD_TARGETS=CustomPico` and optionally
`-DGP2040_BUILD_WITH_WEB=FALSE` before `-P`. CI builds every registered in-tree
target plus the isolated external fixture, retaining shared `fsData` and Pico-only
ELF publication. Its manual `board_config` string is validated before expansion.

Hardware qualification remains pending: input mappings, GP43 battery sensing,
BLE pairing/reconnect, saved configuration and bond persistence across power
cycles, and upgrade/recovery behavior cannot be established by compilation.

### Validation Record (2026-09-29)

- All eight reference targets passed fresh Release configure, output cleaning,
  full Ninja compilation/linking, hardware/storage assertions, and target-named
  UF2 generation with SDK 2.3.1 and Arm GCC 15.2.1 in `build/`.
- Pico's configure ran `npm ci` and the Vite production build. CustomPico compiled
  from an external package path containing spaces and a different working directory.
- The selector suite covers default/invalid selection, compatibility, cache and
  environment precedence, package completeness, duplicate IDs, SDK/header
  shadowing, platform conflicts, and reused-tree behavior.
- Picotool 2.3.1 reports the Pimoroni image as RP2350 ARM Secure, SDK 2.3.1, with
  binary range `0x10000000` through `0x1017e03c`, below the preserved EEPROM region.
  Compiler checks confirm its 4 MiB flash policy; no partition table is introduced.
- Workflow YAML, local task JSONC, edited-file diagnostics, documentation links,
  and tracked diff whitespace checks passed. GitHub-hosted execution of the full
  57-target production matrix has not run in this local implementation pass.
- Existing compiler/generator warnings remain. `npm ci` reported 33 dependency
  vulnerabilities (2 low, 8 moderate, 21 high, 2 critical); dependency migration is
  separate work and no package versions were changed here.
- No hardware was flashed. Persistence, battery sensing and BLE reconnect gates
  remain unverified, and full-capacity Pimoroni flash remains deferred.

## Pre-Implementation Baseline

The following findings describe the source before this migration:

- Root `CMakeLists.txt` defaults `PICO_BOARD` to `pico` and `PICO_PLATFORM` to
  `rp2040` before importing the SDK.
- `GP2040_BOARDCONFIG` independently selects `configs/<name>/BoardConfig.h` and
  an optional `configs/<name>/<name>.cmake` overlay. Some overlays already select
  the correct SDK board and platform; not every existing target defaults to Pico.
- The missing-overlay error is commented out. A controller configuration without
  an overlay inherits the global hardware defaults.
- SDK 2.3.1's `cmake/pico_pre_load_platform.cmake` treats `PICO_BOARD` as the root
  of hardware configuration. It loads board configuration before selecting the
  platform/toolchain and supports additional board-definition search paths.
- SDK hardware headers and GP2040 `BoardConfig.h` files have different roles.
  The latter contain controller button assignments, keyboard mappings, and addon
  defaults that cannot be inferred from a general-purpose SDK board header.
- `.github/workflows/cmake.yml` discovers config directories for its matrix but
  also maintains a separate manual-dispatch board dropdown. It passes the old
  selector and does not explicitly select Ninja in its configure command.
- The firmware workflow runs for `main` pushes/PRs, while this fork's integration
  policy uses `develop`; the new feature needs PR coverage before integration.
- `.github/workflows/node.js.yml` produces shared `fsData`; firmware jobs download
  that artifact and skip rebuilding the web assets themselves.
- The current Pimoroni target deliberately uses a 4 MiB declared flash limit.
  Previous 16 MiB tests lost configuration after reboot. The documented bootrom
  explanation is a hypothesis, not a confirmed root cause.
- SDK 2.3.1 declares 2 MiB for `pico` and 4 MiB for `pico2_w`. The default EEPROM
  region remains at `0x101F8000`, with size `0x8000`, in `FlashPROM.h`. Historical
  claims that all Pico variants have the same flash capacity are not a design
  assumption for this feature.

## Accepted Architecture

### One Public Selector

Introduce `GP2040_BOARD`, with existing project config names as stable target IDs
such as `Pico`, `PicoW`, `Pico2`, `Pico2W`, and `PimoroniPicoLipo2XLW`.

Each supported target maps to:

| Item | Owner |
| --- | --- |
| Stable target ID, label, controller defaults | GP2040 target configuration |
| SDK board name and board-header source | Target's explicit hardware mapping |
| Chip family, wireless wiring, physical hardware properties | SDK-compatible board definition |
| Reserved pins, battery integration, addon defaults | Explicit firmware configuration for that hardware |
| Effective flash/EEPROM layout and temporary safety limits | Validated firmware storage policy |

This removes the need to select two boards on the command line, not the useful
separation between a hardware definition and controller wiring defaults.

Keep the existing `configs/<target>/` layout and use `<target>.cmake` as the
declarative target record next to `BoardConfig.h`. Add an explicit record where
one is currently absent. All existing targets need an explicit hardware mapping;
do not replace the implicit global Pico fallback with a similarly implicit
fallback inside the new resolver.

The record declares an SDK board name, optional custom hardware-header directory,
and only necessary target-specific capability expectations and storage exceptions.
The folder name is the stable target ID; the resolver derives the existing
`GP2040_BOARDCONFIG` internally so compile definitions and runtime identity do not
need to be renamed. Capability expectations validate the SDK result; they are not
a second source of platform or wireless configuration.

The record format must be usable before `project()` and in CMake script mode:
no target creation, SDK initialization, network access, or build side effects.
Translate any existing imperative overlays into declarative values, then apply
them in the owning build layer. This is an intentional tightening of the existing
overlay contract so configuration and CI can share one registry reader.

A shared CMake registry module loads and validates these records. A small script
entry point lists in-tree targets as JSON for CI using CMake's structured JSON
support, without configuring a firmware project. Do not parse CMake source with
shell regular expressions or maintain a duplicate YAML board list.

Prefer a mapping to an existing SDK header over copying it. Add a project-owned
SDK-compatible header only when the SDK lacks that hardware or a documented
hardware correction is necessary. Keep the custom hardware header independent
of TinyUSB, generated protobuf headers, and controller button definitions.

### Resolution Contract

1. Resolve the requested public target before SDK import and `project()`.
2. Load its hardware mapping and firmware-defaults location.
3. Register its additional SDK board search directories, preserving required
   existing paths instead of replacing them later in configuration.
4. Set the resolved `PICO_BOARD`; let the SDK derive its supported platform and
   toolchain. Do not pre-fill every build with `PICO_PLATFORM=rp2040`.
5. Apply only documented, target-specific hardware/storage exceptions.
6. Initialize the SDK and verify that the resolved hardware matches the target.
7. Retain the existing firmware identity and artifact naming for existing targets.
8. Print a summary of target ID, firmware config, hardware source/header, resolved
   SDK board/platform, wireless support, and effective flash/storage policy.

Invalid target IDs, missing required hardware headers, duplicate target IDs, and
incompatible explicit board/platform values must fail with actionable messages.
No silent fallback to Pico is allowed for a supplied but invalid target.

Use the SDK's `PICO_BOARD_HEADER_DIRS` for custom headers and its existing
`PICO_BOARD_CMAKE_DIRS` extension only if a hardware definition actually needs it.
SDK 2.3.1's `cmake/generic_board.cmake` already interprets
`pico_board_cmake_set(...)`, `pico_board_cmake_set_default(...)`, and supported
board-header includes. GP2040 should not implement a competing hardware-header
parser. Validate the resolved SDK header path after SDK selection, including
whether inherited SDK search-path environment values changed the chosen source.

Preserve `Pico` as the default only when no selector is supplied. Make that default
visible in configure output.

### Custom Definitions

Support in-tree hardware definitions through the SDK's extension points, not by
modifying the installed SDK. A target record declares the SDK board name and the
directory containing its custom definition.

Use `GP2040_BOARD_DIRS` as an additional search path for out-of-tree packages.
Each root contains `<target>/<target>.cmake`, `BoardConfig.h` within that target
directory, and any declared custom SDK headers or minimal battery definitions.
Resolve relative paths against the target package, not the invoking shell's
working directory. Report missing files with the package path in the error.

This is a search path, not a second board selection. Example of the proposed
interface for a registered external target:

```powershell
cmake -G Ninja -S . -B build --fresh -DGP2040_BOARD=MyController -DGP2040_BOARD_DIRS=C:/my-boards
```

Resolution must be deterministic. Do not silently shadow an SDK header with a
same-named custom header or accept two packages declaring the same target ID.
Project headers should use distinctive names. External definitions are trusted
build inputs; CI must not download arbitrary headers or execute unreviewed custom
CMake from user-provided URLs.

An arbitrary SDK header does not establish a valid controller configuration.
V1 requires a registered complete target, even when its hardware header comes
directly from the SDK. Never silently reuse Pico button mappings for an unrelated
board. Future generic SDK-only targets are explicitly outside the initial scope.

### Compatibility and Reconfiguration

- Preserve existing controller mappings, stored board identities, artifact
  suffixes, and configuration behavior while changing the public build interface.
- Proposed transition: accept `GP2040_BOARDCONFIG` as a deprecated alias when the
  new selector is absent. Retain this alias for the first release of the feature;
  removal requires a later announced breaking-change decision.
- Command-line cache values should take precedence over environment defaults.
  Detect conflicting legacy selectors rather than silently building the wrong
  chip. Matching old values can warn during the compatibility period.
- Remove hardcoded board/platform values from VS Code task and terminal defaults
  when migrating to the new selector; inherited environment values matter even
  for `cmake --fresh`.
- Record the resolved target identity in the build tree. Changing targets in a
  reused tree must fail with instructions to clean-configure, rather than retain
  stale toolchain, header, or wireless-library settings.
- Continue using Ninja and the repository's `build/` directory convention.

## Flash and Wireless Safety

SDK definitions describe hardware capabilities, not proof that all GP2040 features
are implemented or safe on it. CYW43 support should continue to gate BLE build
integration; this feature does not add a new wireless protocol or Wi-Fi UI.

Use the real hardware definition where possible, but distinguish physical flash
capacity from the effective firmware layout. Validate that compiler macros,
linker limits, image metadata, and EEPROM reservation agree. Do not assume a
larger SDK flash value automatically makes more application space safe to use.

For Pimoroni Pico LiPo 2 XL W, retain the documented 4 MiB safety policy until a
follow-up investigation verifies configuration and BLE-bond persistence with the
full 16 MiB. Do not remove that limit merely to enable a native header.

The larger-flash goal remains part of the requested outcome. Decide whether its
hardware policy can be represented without altering the known-safe boot image
before switching the Pimoroni target away from its current definition. The
confirmed rollout separates full-capacity support from the initial selector
release. Preserve existing saved configurations and bonds; changing EEPROM
placement requires an explicit migration/recovery design.

Validate RP2350A versus RP2350B GPIO/ADC availability, not just RP2040 versus
RP2350. Preserve wireless-reserved pins and the Pimoroni GP43 battery path.
Keep BLE's minimal `BatteryConfig.h` inclusion boundary intact.

## GitHub Actions Plan

Update the workflows in the same implementation series as CMake:

1. Generate the firmware matrix from the same supported target records consumed
  by the resolver, using its script-mode JSON entry point. Report
  invalid/incomplete records instead of skipping them. Production matrices use
  in-tree registered targets; an isolated test fixture exercises external roots.
2. Replace the manually duplicated dispatch dropdown with a validated string
   target input plus an empty/all option. Validate against the registry before
   matrix expansion and pass values through environment variables, not unquoted
   expression interpolation into shell commands.
3. Configure every target using only `-DGP2040_BOARD=<target>` as its board
   selector, with explicit `-G Ninja`, SDK 2.3.1, and fresh build state.
4. Install Ninja and the GATT generator's crypto dependency in relevant jobs.
   Do not accept the random GATT database-hash fallback for BLE artifacts.
5. Keep the reusable web build and shared `fsData` artifact. Avoid introducing
   board-dependent web assets without also changing that artifact contract.
6. Retain stable target-based UF2/ELF artifact names and fail when expected outputs
   are absent. Keep the existing baseline ELF publication behavior.
7. Cover PRs targeting `develop` as well as the existing `main` workflow. Retain
   manual dispatch. Define feature-branch coverage during implementation so this
   change does not wait until a main-branch push to receive CI validation.
8. Test registry validation and negative selection cases, not just successful
   compilation. Include at least one repository-supplied hardware header.

## Implementation Stages

1. Review this design and inventory existing targets and their correct hardware
  mappings. Flag uncertain mappings for maintainer verification rather than
  guessing from names. Confirm each target can retain its firmware defaults
  while using the chosen SDK or custom hardware definition.
2. Add the resolver and explicit mappings while preserving existing behavior.
   Reuse SDK definitions and add only necessary project-owned hardware headers.
3. Migrate CMake consumers, VS Code settings/tasks, documented commands, and both
   workflow entry points together. Preserve runtime identities and artifact names.
4. Add configuration checks and run the cross-board clean-build matrix.
5. Validate representative boards on hardware, including wireless and persistence.
   Lift any larger-flash safety limit only after its separate acceptance gates pass.

Implementation began only after the proposal-writing pass and explicit user
authorization. Hardware qualification remains a separate validation stage.

## Acceptance Criteria

- A clean build works with one selector for RP2040, wireless RP2040, RP2350A,
  wireless RP2350A, and RP2350B configurations.
- Minimum reference matrix: `Pico`, `PicoW`, `Pico2`, `Pico2W`,
  `SparkFunProMicroRP2350`, and `PimoroniPicoLipo2XLW`, plus a representative
  larger-flash SDK definition and a custom hardware definition.
- Tests assert actual resolved platform, GPIO/ADC capacity, CYW43 support, flash
  limits, and firmware configuration; successful compilation alone is insufficient.
- Unknown names, absent custom headers, duplicate targets, conflicting legacy
  inputs, and stale target changes fail predictably. Matching compatibility inputs
  work with documented warnings. Environment and cache precedence are tested.
- External target packages work from paths containing spaces and from a different
  invoking directory. A package colliding with an in-tree target or silently
  shadowing a selected SDK definition is rejected.
- Existing controller defaults, board identity, stored configuration and BLE bonds
  survive the migration. Battery sensing and BLE pairing/reconnect remain intact.
- CI validates all registered project targets, uses the same one-selector command,
  and publishes correctly named artifacts. Unregistered SDK boards fail with
  guidance to add a target record; a compiled target is not automatically a
  hardware-qualified release target.
- Fresh configure and full Ninja builds pass, including a path with the web build.
- Large-flash enablement requires power-cycle persistence tests for configuration
  and BLE bonds, flash/layout inspection, and upgrade/recovery validation.

## References

- [Dependent provisioning feature](post-build-controller-provisioning.md)
- [Root CMake configuration](../../CMakeLists.txt)
- [Firmware CI workflow](../../.github/workflows/cmake.yml)
- [Reusable web CI workflow](../../.github/workflows/node.js.yml)
- [RP2350 support](archive/rp2350-support.md)
- [FlashPROM large-flash investigation](flashprom-large-flash-support.md)
- [Bluetooth baseline](bluetooth-support.md)
- [SDK 2.3.1 board/platform selection](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/cmake/pico_pre_load_platform.cmake)
- [SDK 2.3.1 custom-header lookup](https://github.com/raspberrypi/pico-sdk/blob/2.3.1/cmake/generic_board.cmake)

The archived Pimoroni-only proposal is historical context, not authorization for
this design. Its extra source selectors and automatic fallback are deliberately
not carried forward: they conflict with the one-selector requirement.
