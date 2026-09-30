# Standard Builds and Post-Build Controller Provisioning

**Status:** Draft for review; implementation not authorized by this document.

**Date:** 2026-09-29

**Depends on:** [Unified Board Selection](unified-board-selection.md).

## Goal

Publish a small set of standard firmware builds. Users and controller makers
create and share pin mappings and other settings, and a tool applies those
settings without recompiling firmware. Official executable firmware remains
identifiable independently of community-provided configuration.

## Confirmed Scope Decisions

- Initial tooling generates a combined configured UF2 offline from an official
  firmware release, its matching manifest, and a controller preset.
- Factory reset restores the applied controller preset, not generic defaults.
- Provisioned defaults survive ordinary firmware upgrades and remain separate
  from mutable active settings and private device state.
- Device-connected application is a possible later extension, not initial scope.
- Full-capacity larger-flash support remains separately gated. Preserve existing
  safety limits; this feature does not depend on lifting them.
- This is planning only. No implementation begins during proposal drafting.

## Dependency and Delivery Contract

Unified Board Selection must deliver its accepted resolver, registered target
records, SDK/custom hardware-definition support, validation, and CI integration
before this feature replaces the release matrix or ships provisioning artifacts.
Planning and compatibility research may proceed before that prerequisite ships.

Consume these outputs from the prerequisite rather than creating another board
resolver or hardware registry:

- Stable registered target identities and the `GP2040_BOARD` build selector.
- Explicit SDK/custom hardware mappings, including external target packages.
- Validated platform, GPIO/ADC availability, wireless capabilities, and effective
  flash/storage constraints needed to assess compatibility.
- Shared registry discovery for CI and preserved legacy target behavior.

This feature owns grouping targets into build families, extracting portable
presets, adding persistent factory-defaults storage, releasing the offline tool,
and reducing the release build matrix. Its release manifests and provisioning
storage contract are new deliverables here, not prerequisites imposed on the
board-selection feature.

The dependency is one-way: Unified Board Selection can ship independently while
retaining per-controller builds. Do not remove those builds until this feature's
preset parity, upgrade, recovery, and hardware acceptance gates pass.

## Three Separate Identities

| Identity | Purpose |
| --- | --- |
| Firmware build family | Compiled architecture, drivers, boot/flash layout, hardware ABI |
| Hardware target | SDK/custom definition and electrical compatibility requirements |
| Controller preset | Portable pin assignments and supported runtime settings |

Many controller presets may use the same official build. Two boards may share
one build only after their compile-time hardware assumptions are proven
compatible. Do not promise exactly four builds based only on Pico/Pico W/Pico 2/
Pico 2 W names: RP2350A/B GPIO availability, flash boot support, memory layout,
CYW43 wiring, and compiled drivers can require additional families.

Start with a compatibility inventory, grouping targets by identical required
compile-time behavior. Keep exceptional families explicit. A board requiring new
radio wiring or a missing driver cannot be made compatible by changing a preset's
declared family or platform field.

Keep `GP2040_BOARD` as the public build selector during migration. When standard
families are introduced, it selects a registered build-family target rather than
a controller preset. CI must not compile once per preset. Deprecate old target
aliases explicitly and do not add a second required build selector.

## Runtime Settings Boundary

Move controller-specific defaults into versioned preset data where the runtime
already supports them: button mappings, profiles, keyboard mappings, display,
LED settings, and supported addon/peripheral configuration. Inventory remaining
compile-time-only settings before claiming preset parity with existing targets.

For example, BLE battery sense currently uses compile-time `BatteryConfig.h`
macros. Supporting arbitrary battery wiring through presets needs an explicit
runtime configuration and validation change, or that wiring remains a build-family
constraint. Do not silently drop battery reporting during consolidation.

Hardware-reserved pins and electrical limits are immutable capabilities, not
user-overridable preset settings. SDK UART/SPI defaults alone do not fully describe
safe user-assignable pins. Device-side validation must reject unavailable pins,
radio/flash conflicts, incompatible peripheral use, and settings requiring absent
drivers. ADC inputs and output-driving functions require particular care.

Standard builds need safe first-boot defaults and a reliable configuration entry
path independent of a user pin map. Do not assign arbitrary outputs before a
compatible preset is validated. Define USB configuration/provisioning mode and
recovery behavior before removing per-controller compiled defaults.

## Existing Reuse Points and Gaps

- `www/src/Pages/BackupPage.jsx` reads/writes JSON `.gp2040` files via individual
  settings endpoints. This is an existing configuration vocabulary, not yet a
  versioned portable-preset contract or transactional application mechanism.
- `/api/getConfig` and `/api/setConfig` expose full configuration serialization.
  `setConfig()` starts with a default object and replaces the live configuration;
  omitted properties do not mean preserve the corresponding device values.
- `ConfigUtils::fromJSON()` checks supported types, enums, and sizes, then fills
  defaults and runs migrations. Hardware-family compatibility and preset policy
  require additional explicit validation.
- Stored configuration uses protobuf plus a size/CRC/magic footer in the reserved
  FlashPROM area. This format is not equivalent to the WebUI backup JSON.
- Existing save/import behavior needs failure analysis before claiming atomicity
  or power-loss safety. An application tool must not advertise success just
  because an HTTP request was accepted.

## Preset Contract

Use a data-only, versioned package with a stable preset ID/version, display name,
author/provenance, schema version, supported firmware/configuration versions,
compatible build-family IDs, hardware requirements, and allowed settings payload.
Reuse supported configuration schemas rather than duplicating a parallel pin
model. Exact format and migration rules must be settled before implementation.

Distinguish reusable presets from private device backups. Public presets must
exclude BLE bonds, authentication material, network credentials, unique device
identity, private calibration, migration bookkeeping, and storage addresses.
An export path must whitelist public fields, not assume every backup is shareable.

Default application preserves private device state and settings outside the
preset's documented ownership. Explicitly define omitted fields, array replacement,
clearing a setting, and selecting which sections to apply. Never implement this as
an unrestricted deep merge or post a partial preset to the full-replacement API.
Reject unknown or unsupported required fields instead of silently dropping them.

## Application Tool

The confirmed initial tool is an offline command-line packager. Its inputs are an
official firmware artifact, the matching release manifest, and a controller
preset. It validates schema and compatibility, supports validation/dry-run, then
produces a derived configured UF2 and a provenance/validation report. It does not
require a connected controller, a compiler, or private device backups. A future
GUI can use the same validation and packaging logic.

Device-connected application through USB web-configuration mode is a possible
later extension, not the initial delivery scope. That path would require staged
validation, backup, explicit device selection, controlled activation, read-back
verification, and reviewed interruption/storage-failure behavior. Neither path
should transmit backups or secrets to a hosted service.

The initial output is a combined UF2, not a configuration-only image requiring a
separate manual flash step. Evaluate existing GP2040 configuration/UF2
tooling before implementing a new encoder. Offline generation requires exact
release schema, migration/default behavior, flash layout, UF2 family ID, bounds,
and CRC/footer handling; it must not guess offsets from a board name.

Combined artifacts are derived images, not unchanged official downloads. Verify
the input release digest and prove executable regions are unchanged; record source
firmware and preset digests. Offline provisioning cannot preserve private state
already on a device without reading it, so keep first-install and update workflows
distinct and require explicit consent before overwriting stored settings/bonds.

The tool must refuse older firmware artifacts that lack the provisioned-defaults
contract. It cannot retrofit reliable preset-reset behavior solely by patching
the current active configuration. Report successful packaging separately from
successful flashing or hardware validation, which offline execution cannot prove.

## Defaults, Reset, and Upgrades

Confirmed behavior: the applied preset establishes controller-specific factory
defaults. Persist its validated payload and identity separately from mutable
active settings. Normal user changes must not modify the provisioned defaults.
Factory reset discards active settings and initializes them from that preset,
while generic official firmware without a preset retains safe generic defaults.

Define a dedicated, versioned defaults storage contract with integrity checks,
capacity limits, and release-manifest offsets. Reserve that region in the linker
and image layout, separately from active configuration and private device state.
Do not select its address or expand flash until overlap and existing storage
behavior have been verified. Default-preset validation must also run in firmware:
the host packager is not a trusted substitute for bounds and pin checks at boot.

On first provisioning, activate the preset only after complete validation. Define
a recoverable activation marker/version scheme so interrupted flashing or stale
active settings cannot silently select a different controller setup. Invalid or
incomplete defaults must enter a safe recovery/configuration path rather than
drive arbitrary outputs. Repeated boot must not reset later user modifications.

Ordinary official upgrade artifacts must leave provisioned defaults and active
state untouched. Preset-bearing images explicitly replace factory defaults and
require a documented policy for existing active settings and private state.
Initial packaging targets fresh-device provisioning; replacing a preset on a
configured device requires an explicit reprovisioning flow. An offline packager
cannot inspect the connected device's state or promise to back it up.

Normal factory reset restores the provisioned preset, not generic defaults. A
distinct explicit erase/recovery operation may remove provisioning. Document
whether reset clears bonds/credentials separately from controller-default
restoration; public presets never contain those secrets.

Firmware updates should preserve compatible active settings and private state.
Unsupported downgrades and cross-family changes must be rejected or handled by an
explicit recovery flow. Do not infer compatibility from a semver string alone.
Keep the existing large-flash safety gate: post-build configuration does not fix
the Pimoroni persistence issue or authorize moving EEPROM.

## CI and Release Transition

1. Verify the Unified Board Selection dependency has met its acceptance criteria.
   Inventory controller defaults and split hardware-family requirements from
   portable preset settings. Record unsupported compile-time-only settings.
2. Add preset schema/compatibility tests, persistent factory-defaults storage, and
   safe first-boot/reset/recovery behavior to the standard firmware builds.
3. Build and validate the offline UF2 packager against matching official release
   manifests and representative hardware before switching the release model.
4. Change the release matrix from controller targets to approved build families.
   Validate every preset as data; test representative applications across families
   without recompiling every controller variant.
5. Publish firmware plus a machine-readable capability/layout/schema manifest,
   digests, preset packages, and the application tool. Community presets should
   work without a fork or a bespoke firmware build when hardware is compatible.
6. Retain legacy controller builds until equivalent presets, upgrade paths, and
   hardware validation pass. Only then retire their redundant release artifacts.

CI must reject invalid presets, reserved-pin conflicts, incompatible firmware
requirements, accidental secrets, malformed/oversized payloads, and offline
artifacts overlapping executable or forbidden storage regions. Include tests for
round-trip export/application, interruption/retry, preservation of private state,
reset semantics, upgrades, and unchanged executable hashes in offline mode.

## Acceptance Criteria

- The prerequisite's registry, resolver, and hardware validation are reused;
  this feature does not introduce a separate hardware selection system.
- Two distinct controller presets run on the same byte-identical official
  executable firmware without recompilation.
- A community author can validate and apply a new compatible setup using the
  released tool and schema without adding a firmware CI matrix entry.
- Fresh devices can be provisioned and recovered without an existing button map.
- One combined configured UF2 establishes both the controller setup and durable
  factory defaults. Changed active settings survive normal reboot; factory reset
  restores the provisioned preset, including after an ordinary firmware upgrade.
- A normal official upgrade does not overwrite provisioning storage. Explicit
  reprovisioning and full erase follow documented, independently tested policies.
- Corrupted/truncated defaults, interrupted provisioning, schema mismatches, and
  firmware without defaults support are rejected or recover safely without
  enabling unsafe outputs. Packaging success is not reported as hardware success.
- Device state is preserved according to explicit policy; failed application is
  detected and recoverable, with no false success or silent partial application.
- The official build-family count follows proven hardware requirements, not the
  number of available controller presets. No fixed count is promised before audit.
- Hardware tests cover input mappings, BLE/battery where supported, save/reboot,
  reset, firmware upgrade, and incompatible-preset rejection.
- Fresh configure and full Ninja builds pass for the affected families, including
  a path with the web build, before implementation is considered complete.

## References

- [Unified Board Selection prerequisite](unified-board-selection.md)
- [Backup UI](../../www/src/Pages/BackupPage.jsx)
- [Configuration API](../../src/webconfig.cpp)
- [Configuration serialization and defaults](../../src/config_utils.cpp)
- [FlashPROM storage](../../lib/FlashPROM/src/FlashPROM.h)
- [Firmware CI workflow](../../.github/workflows/cmake.yml)
- [Reusable web CI workflow](../../.github/workflows/node.js.yml)
- [FlashPROM large-flash investigation](flashprom-large-flash-support.md)
- [Bluetooth baseline](bluetooth-support.md)
