# Development Document Audit

**Audit date:** 2026-09-29
**Baseline:** Local `feature/bluetooth-next` source tree.

Reviewed the 20 top-level development documents against their scope and current
source/configuration entry points. Three implemented documents moved to the
existing `archive/` directory. Seventeen remain here: incomplete proposals,
ongoing work, and maintained reference material are not completed features.
The two existing untracked drafts were retained without changing their scope.

The existing [ignore rule](../../.gitignore) deliberately makes `archive/`
local-only. This policy was explicitly retained during the audit: archived files
remain in this workspace but are excluded from commits. Links into `archive/`
therefore require those local files and will not resolve in a fresh checkout.

This is a source/documentation audit, not a fresh hardware qualification or
release certification. Archived specs preserve historical rationale; their
audit notes distinguish the current implementation from old plans and claims.

## Archived Implementations

| Document | Implementation evidence and caveats |
| --- | --- |
| [BLE device identity](archive/ble-device-identity.md) | [DIS initialization](../../src/BLEHIDManager.cpp) and [identity defaults](../../headers/ble_identity.h) implement metadata, PnP ID, serial number, and board overrides. Fixed product-version and placeholder-PID caveats are recorded in the archive note. |
| [BLE digital report rework](archive/ble-hid-report-rework.md) | [Descriptor](../../src/BLEHIDManager.cpp) and [report packing](../../src/OutputManager.cpp) implement 14 buttons plus hat in three bytes, without analog fields. Host re-pair guidance exists; hardware regression checks were not rerun. |
| [RP2350 support](archive/rp2350-support.md) | SDK overlays and `NUM_BANK0_GPIOS`-based firmware/Web API handling are present. The archive note corrects historical SDK/GPIO assumptions and leaves separate board-selection/large-flash proposals open. |

## Retained Documents

| Document | Audit disposition |
| --- | --- |
| [Roadmap to 1.0](roadmap-to-1.0.md) | Added 2026-09-30. Living gamepad-only parity roadmap for PlayStation, Xbox, and Nintendo Pro controllers. Wireless is first and audio later; Joy-Con parity is excluded and licensing/authentication work is deferred. The 16 feature drafts below cover the active gaps while preserving existing features. |
| [Warning-free firmware and WebUI builds](warning-free-builds.md) | Added 2026-09-30. Measured clean-build/quality baseline and remediation plan: successful but warning-bearing Pico/Pico2W Release and Pico Debug builds, failing lint/types, dependency advisories, and missing strict CI gates. Planning only; diagnostics are not yet fixed. |
| [Bluetooth support](bluetooth-support.md) | Implemented BLE baseline plus active host regression, UI/power refinement, and experimental variant work. Retain the operational reference; narrower completed specs are archived above. |
| [Bluetooth controller architecture](bluetooth-controller-architecture.md) | Pending profile abstraction, keyboard/mouse, Classic/LE selection, and output channels. Current [dispatch](../../src/OutputManager.cpp) still packs a single BLE gamepad profile. |
| [Dependency management](dependency-updates.md) | Ongoing maintenance guide, not a finite feature to archive. Version tables are historical: current [CMake](../../CMakeLists.txt) pins SDK 2.3.1 and ArduinoJson v6.21.5. |
| [Developer documentation system](developer-documentation-system.md) | Docs artifact generation, release-sync PR automation, and drift enforcement are still pending. This audit index does not implement that CI system. |
| [FlashPROM large-flash support](flashprom-large-flash-support.md) | Reopened for specification review on 2026-09-30: full-flash persistence/layout qualification first, then derived 2/4/8/16 MiB compile flags. No new firmware implementation in the specification pass; the Pimoroni 4 MiB safety limit remains. |
| [GPIO retro output](gpio-retro-output.md) | Planned output adapters and configuration are absent; existing retro input adapters do not fulfill this scope. |
| [HID over I2C](hid-over-i2c.md) | Planned HID target/slave transport and configuration are absent. Existing I2C peripheral input support is not this feature. |
| [I2C peripheral expansion](i2c-peripheral-expansion.md) | Planned satellite-output addon and packet/configuration contract are absent. Historical BLE/Classic status in its context table is superseded by the active Bluetooth reference. |
| [Latency testing framework](latency-testing-framework.md) | Planned capture fixture, analysis tooling, and published regression measurements are not present as an integrated framework. |
| [nanopb stable migration](nanopb-stable-migration.md) | [pb.h](../../lib/nanopb/pb.h) still declares `nanopb-0.4.8-dev`. |
| [npm major upgrades](npm-major-upgrades.md) | [package.json](../../www/package.json) retains React 18, Vite 4, ESLint 8, Zustand 4, TypeScript 5, and protobufjs-cli 1 ranges. The staged migration is not complete. |
| [PIO USB upstream port](pico-pio-usb-upstream-port.md) | [.gitmodules](../../.gitmodules) still selects the OpenStickCommunity fork and `dev` branch; no completed upstream migration evidence. |
| [Post-build controller provisioning](post-build-controller-provisioning.md) | Draft. No offline provisioning tool, release-manifest contract, or separate persistent factory-preset implementation found; existing config import does not fulfill this scope. |
| [RM2 module support](rm2-module-support.md) | Planned custom-board integration and validation. Shared CYW43 support exists, but no RM2 reference configuration or completed validation evidence was found. |
| [TinyUSB upstream port](tinyusb-upstream-port.md) | [tusb_option.h](../../lib/tinyusb/src/tusb_option.h) still declares 0.17.0, and [.gitmodules](../../.gitmodules) retains the project fork. |
| [Unified board selection](unified-board-selection.md) | Authorized follow-up implementation on `feature/unified-board-selection`: shared registry, `GP2040_BOARD` resolver, CI/local task migration, and hardware/storage assertions are present. Eight reference clean builds passed, including the web build; hardware persistence/reconnect qualification remains pending. |
| [WiFi web configuration](wifi-web-config.md) | Planned STA connection/configuration path is absent; CYW43 BLE support alone does not provide WiFi web configuration. |

## 1.0 Feature Specifications

Added 2026-09-30 after the historical audit above. These are living planning
documents, not completed implementations. The roadmap owns the requirement-ID
mapping, shared scope decisions, and release contract.

| Document | Feature scope |
| --- | --- |
| [PlayStation gamepad profiles](playstation-gamepad-parity.md) | Standard PS5 protocol and native wireless. |
| [Xbox gamepad profiles](xbox-gamepad-parity.md) | Standard Xbox protocol and native radio integration. |
| [Nintendo Pro gamepad profiles](nintendo-pro-gamepad-parity.md) | Original Switch Pro and Switch 2 Pro protocols/wireless. |
| [Complete analog inputs](gamepad-analog-input-parity.md) | Sticks, triggers, complete input state, and calibration. |
| [Pairing and host management](wireless-pairing-and-host-management.md) | Identities, bonds, reconnect, and host selection. |
| [Power lifecycle](controller-power-lifecycle.md) | Sleep, wake, shutdown, and transport handover. |
| [Battery and charging](controller-battery-and-charging.md) | Measurement, host status, and safe power hardware. |
| [System controls and output routing](controller-output-and-system-controls.md) | Console buttons, indicators, and command dispatch. |
| [Motion sensors](controller-motion-sensors.md) | IMU acquisition, timing, and calibration. |
| [PlayStation touchpad](playstation-touchpad.md) | Physical two-contact tracking and click. |
| [Haptics and impulse triggers](controller-haptics.md) | High-definition feedback and independent Xbox channels. |
| [Adaptive triggers](playstation-adaptive-triggers.md) | PlayStation trigger resistance and safe actuation. |
| [Nintendo NFC reader](nintendo-nfc-reader.md) | Physical reader and console tag interactions. |
| [Controller audio](controller-audio.md) | Headsets, microphones, speaker, and accessory streams. |
| [Capabilities and configuration](controller-capabilities-and-configuration.md) | Hardware validation, settings, migrations, and recovery. |
| [Parity qualification](controller-parity-qualification.md) | Performance, hardware evidence, and existing-feature regression gates. |

## Existing Archive

These documents were already archived before this audit and were not
reclassified. Archive location alone is not proof of implementation:

- [Pimoroni Pico Lipo 2 XL W support](archive/pimoroni-pico-lipo-2xl-w-support.md)
- [Optional Pimoroni SDK board definition](archive/pimoroni-sdk-board-definition-option.md), still a proposal
- [USB 3 gamepad research](archive/usb3-gamepad-research.md), research rather than an implemented feature

## Remaining Documentation Debt

The dependency guide has four pre-existing unresolved relative links: a docs
README, `protobuf-config.md`, `web-configurator.md`, and a hardware directory.
They are unrelated to these archive moves and remain for a documentation
maintenance pass. Existing Markdown style warnings were not broadly reformatted.
