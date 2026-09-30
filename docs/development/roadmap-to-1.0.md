# Roadmap to 1.0: First-Party Controller Feature Parity

**Status:** Living roadmap; child specifications are drafts for review, not implementation authorization.

**Date:** 2026-09-30

**Baseline:** Local `feature/unified-board-selection` source tree, including the
integrated BLE work. Source presence is not hardware qualification.

## Goal and Release Contract

GP2040-CE 1.0.0 must meet or exceed the functionality of first-party primary
gamepad-style controllers across PlayStation, Xbox, and Nintendo while retaining its existing
multi-platform support, customization, and low-latency behavior.

Wireless console operation is the largest outstanding priority. Audio is a
secondary delivery priority, not an exemption from the final parity requirement.
Extra customization does not compensate for a missing first-party capability.

This is the umbrella roadmap and gap inventory, not a claim that every gap is
technically feasible on today's hardware. Each workstream needs an accepted
feature specification before implementation. Protocol, hardware, and qualification
gaps remain active work; licensing and console authentication are deferred from
this implementation-planning pass, not declared solved or unnecessary.

Preserve existing authentication integrations without redesigning them here.
Use existing authorized sessions or test fixtures to develop independent features;
mark console tests blocked when their prerequisites are unavailable. Deferral does
not turn simulated results into console qualification or authorize a final native
compatibility claim. Ordinary pairing, bonding, encryption, and private-state
handling remain technical requirements, separate from console authentication.

## Reference Controllers and Scope

The current reference scope is conventional gamepads. This is a living document:
record scope changes, retain requirement IDs, and update evidence as work proceeds.
New requirements need explicit review rather than automatically expanding 1.0.

| Console family | Primary reference | Additional coverage |
| --- | --- | --- |
| PlayStation 5 | Standard DualSense | Preserve existing PS3/PS4 and PS5 compatibility modes. DualSense Edge-specific extras are not required. |
| Xbox Series X\|S | Standard Xbox Wireless Controller | Preserve existing Xbox One, Xbox 360, and original Xbox modes with their documented dependencies. Elite-specific extras are not required. |
| Nintendo Switch 2 | Switch 2 Pro Controller | Preserve existing Switch/Switch OLED/Switch Lite compatibility. Qualify original Switch Pro Controller behavior separately. |

Joy-Con and Joy-Con 2 are explicitly outside 1.0 parity scope, including mouse
sensing, split-controller operation, rails/magnetic attachment, and attachment
charging. Existing support is still preserved. Nintendo parity means Pro
Controller-style gamepads, not every controller bundled with a console.

Parity means equivalent user-observable behavior on the actual console and in
games that exercise the feature, over each transport supported by the reference.
Generic PC HID recognition, a matching descriptor, an acknowledged command, or
an arcade-stick compatibility mode is not sufficient evidence.

The project need not clone an enclosure, proprietary updater, branding, or exact
component selection. It does need usable physical inputs and outputs, safe power
behavior, reliable updates/recovery, and documented hardware capable of delivering
the claimed experience. Console-side services remain console responsibilities.

## Status Vocabulary

- **Partial:** Relevant source exists, but coverage or end-to-end evidence is incomplete.
- **Open:** A complete implementation and qualification path has not been established.
- **Research gate:** Feasibility or requirements must be resolved before promising delivery.
- **Deferred:** Not active implementation scope; this does not establish completion.
- **Out of scope:** Deliberately excluded from the current parity contract.
- **Qualified:** Recorded hardware evidence satisfies the acceptance matrix. No new
  capability is assigned this status by this documentation pass.

## Current Foundations

| Area | Evidence and remaining boundary |
| --- | --- |
| Wired compatibility | Existing [driver selection](../../src/drivermanager.cpp) and [USB drivers](../../src/drivers/) provide multiple protocol families. Qualify each console, controller persona, authentication path, and game class independently. |
| PS5 paths | [PS4Driver](../../src/drivers/ps4/PS4Driver.cpp) has an arcade-stick compatibility path; [P5GeneralDriver](../../src/drivers/p5general/P5GeneralDriver.cpp) also exists with its own authentication integration. Neither source presence nor the older arcade path proves full DualSense parity. |
| Generic wireless | [Bluetooth support](bluetooth-support.md) documents BLE pairing, bonds, controls, and board-specific battery reporting. Its current digital-only gamepad payload lacks analog sticks/triggers and is not a native three-console wireless profile. |
| Sensors and feedback | PS3/PS4/P5General and other drivers contain sensor/report or feedback handling. Inventory actual sensor acquisition, calibration, actuator support, and per-transport behavior before labeling any of these complete or entirely absent. |
| Nintendo Pro protocol | [SwitchProDriver](../../src/drivers/switchpro/SwitchProDriver.cpp) provides wired controls and protocol handling; its input report initializes IMU data to zero. This is not evidence of complete motion, HD rumble, NFC, or Switch 2 support. |
| Xbox feedback | [XBOneDriver](../../src/drivers/xbone/XBOneDriver.cpp) handles rumble commands. Qualify body and trigger channels independently; command handling alone does not prove physical feedback parity. |
| Configuration and hardware | Embedded WebUI, persistent settings, addons, and [unified board selection](unified-board-selection.md) provide reuse points. Optional hardware must remain capability-gated. |

## Cross-Console Gap Inventory

Priorities express delivery order: **P0** establishes compatibility and wireless;
**P1** completes gameplay and controller lifecycle; **P2** completes audio.
All applicable rows remain 1.0 release requirements regardless of priority.

| ID | Priority / status | Remaining deliverable | Acceptance evidence |
| --- | --- | --- | --- |
| C01 | P0 / Partial + research gate | Full standard-gamepad protocol identity, reports, and session behavior for each console, distinct from restricted accessory personas. Authentication implementation is deferred. | Native games and system UI work in an available valid session, including reconnect and console updates; blocked console access is recorded separately. |
| C02 | P0 / Open | Native console wireless profiles and the necessary radio hardware. Generic BLE, Bluetooth Classic, and Xbox Wireless are not interchangeable. | Direct console pairing, bidirectional traffic, encrypted/bonded reconnect where applicable, and long-session stability for each reference profile. |
| C03 | P0 / Partial | Both analog sticks, independent axes/clicks, analog triggers where the reference has them, all face/shoulder/system buttons, simultaneous input, and calibration across wired/wireless profiles. | Sweep range, center, deadzone, resolution, rollover, and remapping tests against the reference. Digital stick emulation does not substitute for physical analog input. |
| C04 | P1 / Partial | Complete host-to-controller commands, feature requests, player assignment, status lighting, and persistent settings with bounded parsing and scheduling. | Console settings and output commands produce the intended physical behavior without dropped or delayed input. |
| C05 | P0 / Partial | Pairing controls, bond management, stable device identity, host selection, and reference-equivalent host switching. Preserve identities across ordinary updates. | Fresh pair, stale-bond recovery, repeated power cycles, multiple nearby controllers, and deliberate host changes without unwanted connections. |
| C06 | P1 / Partial | Controller idle/sleep, user power-off, console sleep/disconnect handling, wake from supported console sleep states, and deterministic wired/wireless handover. | Compare each state transition to the reference, including charging-only cables, USB hotplug, exhausted battery, and console resume. No duplicate controllers or stuck inputs. |
| C07 | P1 / Partial | Accurate battery/charging status, low-battery alerts, safe shutdown, charge-and-play, and suitable rechargeable or replaceable battery hardware. | Measured state reporting, runtime and standby comparisons, power-loss recovery, and electrical validation. Firmware must not imply charging support without a charger/protection circuit. |
| C08 | P1 / Open | Complete per-console physical feedback and sensor capabilities listed below. | Gameplay and hardware measurements, not just output logging or synthetic sensor fields. |
| C09 | P2 / Open | Controller audio paths where the reference supports them, including wireless delivery, microphone input, headset detection, and host controls. | Concurrent gameplay, feedback, and bidirectional audio without unacceptable latency, dropouts, or input regression. |
| C10 | P0 / Partial | Versioned capability reporting, configuration/migration, safe update/recovery, and clear unsupported-feature reporting. | Upgrade/rollback and recovery matrix; missing hardware cannot be enabled by configuration alone or falsely advertised to the host. |
| C11 | P0 / Open qualification | Measured input latency, jitter, reliability, RF coexistence, and multi-controller operation. | Reproducible first-party comparisons and preserved wired baselines under worst-case supported feature combinations. |

## PlayStation Deliverables

| ID | Gap / required outcome |
| --- | --- |
| PS01 | Qualify the existing general PS5 protocol path for ordinary PS5 titles, not only games accepting specialty controllers. Keep authentication implementation deferred and record session prerequisites separately. |
| PS02 | Implement and qualify DualSense-equivalent wireless pairing, reconnect, output traffic, console wake, battery status, and supported multi-device selection behavior. Wired compatibility is not evidence of wireless compatibility. |
| PS03 | Complete two-contact touchpad coordinates, tracking/contact lifecycle, click, gestures as interpreted by games, and real accelerometer/gyroscope data with calibration, orientation, timestamps, and required feature reports. |
| PS04 | Deliver both high-definition haptic channels with appropriate actuators and timing. Two basic rumble motors are a useful fallback, not full DualSense haptic parity. |
| PS05 | Deliver independent adaptive-trigger effects, force/tension changes, host enable/intensity controls, and a safe unpowered/disconnected state. Analog trigger position alone is not adaptive feedback. |
| PS06 | Provide Create/Options/PS/touchpad/mute controls, player indicators, host-controlled light behavior, and correct status reporting. Preserve existing mappings while adding missing semantics. |
| PS07 | Provide headset stereo output and microphone input, built-in speaker and microphone behavior, mute control/indicator, jack detection, and console audio routing/volume controls over the supported wired and wireless paths. Validate privacy and default mute behavior. |

Audio transport, haptic waveform delivery, and adaptive-trigger hardware are
independent research gates. Authentication is deferred. A passthrough device may be a documented
compatibility option, but a wired dongle or converter must not be reported as
direct native wireless parity.

## Xbox Deliverables

| ID | Gap / required outcome |
| --- | --- |
| XB01 | Qualify standard-controller wired protocol operation on Xbox One and Series X\|S. Preserve existing authentication options without expanding them in this workstream. |
| XB02 | Establish a technically viable Xbox Wireless transport or radio/module integration for direct console operation; licensing and authentication are deferred. The standard controller's Bluetooth PC/mobile path does not establish Xbox console wireless support; CYW43 Bluetooth support is not proof of Xbox Wireless capability. |
| XB03 | Complete body rumble plus independent left/right impulse-trigger feedback, including channel strength, duration, stop commands, and concurrent operation. Do not collapse four physical channels into two and claim parity. |
| XB04 | Provide Share, View, Menu, Xbox/system controls, player assignment, console wake, battery behavior, pairing, and reference-equivalent console/Bluetooth host switching. Preserve all ordinary stick and trigger semantics. |
| XB05 | Provide 3.5 mm headset output/input and the controller expansion/accessory interface behaviors needed by standard supported headsets/chat accessories, with routing, mute, volume, and chat/game controls where exposed. Qualify Xbox Wireless audio separately from USB and PC Bluetooth limitations. |

The standard Xbox Wireless Controller does not establish a requirement for gyro,
touchpad, adaptive resistance, a built-in microphone, or a built-in speaker.
GP2040-CE may exceed that baseline without making those features prerequisites
for its Xbox profile. Rechargeable-pack support must not attempt to charge
ordinary primary cells.

## Nintendo Deliverables

This inventory includes Switch 2-specific requirements, but those items remain
provisional until checked against accessible official specifications and reference
hardware. Existing Switch support must not be relabeled Switch 2 support.

| ID | Gap / required outcome |
| --- | --- |
| NS01 | Independently qualify original Switch Pro and Switch 2 Pro profiles: identification, handshakes, feature/subcommand behavior, calibration storage, input timing, USB behavior, and native wireless operation. An older compatible controller is not a Switch 2 Pro equivalent. |
| NS02 | Complete real accelerometer/gyroscope acquisition, calibrated motion reports, required sampling/timestamps, and console/game calibration flows. |
| NS03 | Deliver HD rumble for the original reference and HD rumble 2 for Switch 2 references, including appropriate actuators, decoding, timing, and safe stop behavior. Plain vibration is not equivalent. |
| NS04 | Provide NFC reader behavior for supported amiibo interactions, including physical reader hardware and console protocol handling. Reader support does not require distributing or manufacturing proprietary tag data. |
| NS05 | Provide Home/Capture/Plus/Minus, player/connection indicators, pairing, battery/charging, console sleep/wake, and controller-finding behavior where the reference supports it. Test each controller generation's actual wake limitations. |
| NS06 | Provide the Switch 2 Pro C button and GL/GR rear-button assignment semantics, including interaction with console-side configuration. GP2040 profiles alone do not prove equivalent console behavior. |
| NS07 | Provide Switch 2 Pro headset-jack playback/microphone paths and required audio controls. The C button opens console GameChat; the controller does not need to implement the GameChat service or a console-mounted microphone. Original Switch Pro has no headset jack and gains no invented audio requirement. |
| NS08 | Out of scope as of 2026-09-30: Joy-Con/Joy-Con 2 parity. ID retained for history; no implementation specification or 1.0 gate is assigned. |

Original Joy-Con IR-camera, rail/accessory behavior, Ring-Con, and other specialty
controllers are also excluded. Removing them from parity scope does not permit
regressions in existing firmware features or input addons.

## Preserve and Extend the Existing Feature Set

Before refactoring shared paths, freeze an executable regression inventory from
the actual release source and configuration schema, not just the abbreviated
root README. Preserve every currently supported feature and its documented
hardware/host constraints, including:

- Existing PC, console, mini/retro, generic HID, keyboard, and BLE modes, their
  descriptors, boot shortcuts, authentication options, and host compatibility.
- SOCD modes, dual-direction inputs, D-pad/stick emulation, analog inputs,
  remapping, profiles/hotkeys, turbo/macros where supported, and input addons.
- USB-host controller/keyboard/mouse inputs and authentication passthrough,
  including resource conflicts when these share ports or peripherals.
- Displays, splash screens, RGB/player/reactive LEDs, buzzer, feedback addons,
  peripheral expansion, and all existing board-specific wiring options.
- Embedded WebUI, localization, backup/import, saved settings and migrations,
  factory reset, boot recovery, BLE bonds, and board-specific battery reporting.
- Non-wireless RP2040 builds, existing RP2350 targets, storage safety limits,
  and current low-latency wired operation without requiring new peripherals.

Use the existing per-driver protocol model, shared gamepad/auxiliary state,
addon model, and board capability selection. Extend the
[Bluetooth architecture](bluetooth-controller-architecture.md) rather than
creating a competing transport/profile abstraction. Any intentional restructuring
needs justification in its workstream feature document.

Existing physical controllers need not acquire impossible hardware capabilities.
Instead, retain their current behavior and distinguish a **compatible build**
from a **parity-qualified hardware configuration**. Publish at least one reproducible
reference configuration per console covering all its mandatory capabilities.
Different consoles may require different radio, actuator, sensor, and audio
hardware; do not promise one universal Pico-only circuit before feasibility tests.

New features must not silently remove existing ones to fit memory. Establish CPU,
RAM/stack, flash, bus, USB endpoint, DMA, pin, and power budgets for supported
combinations. Use explicit capability/resource validation and documented optional
builds where necessary, keeping existing configurations available. Preserve the
Pimoroni 4 MiB safety limit until the separate storage qualification lifts it.

## Delivery Milestones

| Milestone | Deliverables and exit gate |
| --- | --- |
| M0: Baseline and measure | Record gamepad reference models, console versions, per-feature acceptance tests, current compatibility/regression baselines, and hardware inventory. Resolve technical feasibility experiments for Xbox Wireless, PS5 protocol identity, Switch 2 Pro protocols, and audio bandwidth. Track deferred authentication/licensing separately. |
| M1: Wireless foundation | Preserve generic BLE; implement accepted transport/profile separation, full analog state plumbing, bond/identity management, bidirectional channels, and safe capability selection. Existing USB/BLE regressions pass. |
| M2: Native console wireless | Deliver PlayStation, Nintendo Pro, and Xbox wireless paths with pairing, reconnect, wake/power behavior, and initial latency/reliability evidence. Qualify on consoles where valid sessions are available; mark inaccessible tests blocked. One console's success does not close the other two. |
| M3: Complete gameplay parity | Finish sensors, touchpad, NFC, system controls, haptics, adaptive/impulse triggers, and remaining gamepad lifecycle/hardware requirements. Verify actual games and physical effects. |
| M4: Audio parity | Finish each applicable headset, microphone, speaker, mute/routing, and accessory path over its supported transports. Pass simultaneous audio/input/feedback stress tests. Research and hardware selection begin in M0 even though delivery is secondary. |
| M5: 1.0 release qualification | Close every mandatory capability row with evidence, preserve the existing feature inventory, publish hardware/compatibility limits, and pass release builds, migrations, endurance, safety, and performance gates. |

These are dependency gates, not promised dates or prescribed intermediate version
numbers. Independent console research can proceed in parallel. A blocked native
wireless or audio path cannot be waived merely by completing the other milestones;
changing the 1.0 promise requires an explicit scope decision.

## Related Feature Documents

| Document | Roadmap relationship |
| --- | --- |
| [Warning-free builds](warning-free-builds.md) | Measured firmware/WebUI diagnostic backlog and strict clean-build gates supporting release qualification; separate from the 16 parity feature specifications. |
| [Bluetooth support](bluetooth-support.md) | Preserve and qualify the implemented BLE baseline. |
| [Bluetooth controller architecture](bluetooth-controller-architecture.md) | Foundation for transport/profile separation and output channels. Its logging/no-op output stage is an intermediate milestone, not physical feedback/audio parity. |
| [Latency testing framework](latency-testing-framework.md) | Measurement methodology and regression evidence for wired and wireless paths. |
| [Unified board selection](unified-board-selection.md) | Shared registered hardware identities, capability constraints, and build validation. |
| [Post-build controller provisioning](post-build-controller-provisioning.md) | Optional distribution/configuration improvement; preserve presets, private state, and hardware validation if adopted. Not a substitute for parity. |
| [FlashPROM large-flash support](flashprom-large-flash-support.md) | Separate persistence/safety gate if a qualified build requires more usable flash. Not permission to lift current limits. |
| [RM2 module support](rm2-module-support.md) | Possible Bluetooth-capable hardware expansion, not proof of proprietary console-radio compatibility. |
| [I2C peripheral expansion](i2c-peripheral-expansion.md) | Possible hardware expansion path; assess timing/bandwidth before assigning sensors or feedback to it. |
| [WiFi web configuration](wifi-web-config.md) | Optional configuration convenience, not controller wireless parity; any concurrent radio use needs coexistence testing. |

GPIO retro output, HID over I2C, additional Bluetooth keyboard/mouse personas,
provisioning, and dependency migrations remain valuable independent work. They
are not automatically 1.0 parity blockers unless a mandatory capability depends
on them or they are needed to preserve existing behavior.

## Individual Feature Specifications

These 16 living drafts own the active implementation gaps. Shared requirements
are split by responsibility rather than copied into separate per-console physical
drivers. C08 is an umbrella for sensors and feedback; its owners are listed below.
All drafts inherit the gamepad-only scope, preservation requirements, deferred
licensing/authentication policy, and qualification rules in this roadmap.

| Feature specification | Owned requirements / responsibility |
| --- | --- |
| [PlayStation gamepad profiles](playstation-gamepad-parity.md) | C01/C02 PlayStation protocol and transport; PS01, PS02. |
| [Xbox gamepad profiles](xbox-gamepad-parity.md) | C01/C02 Xbox protocol and radio; XB01, XB02. |
| [Nintendo Pro gamepad profiles](nintendo-pro-gamepad-parity.md) | C01/C02 Nintendo protocol and transport; NS01. |
| [Complete analog inputs](gamepad-analog-input-parity.md) | C03 acquisition, calibration, and complete input state. |
| [Pairing and host management](wireless-pairing-and-host-management.md) | C05 identity, bonds, reconnect, and host selection; shared parts of PS02, XB04, NS05. |
| [Power lifecycle](controller-power-lifecycle.md) | C06 sleep/wake and transport handover; shared parts of PS02, XB04, NS05. |
| [Battery and charging](controller-battery-and-charging.md) | C07 power measurement and charging hardware/status; shared parts of PS02, XB04, NS05. |
| [System controls and output routing](controller-output-and-system-controls.md) | C04, PS06, XB04, NS05, NS06 controls/indicators and command dispatch. |
| [Motion sensors](controller-motion-sensors.md) | C08 motion; PS03 motion portion; NS02. |
| [PlayStation touchpad](playstation-touchpad.md) | C08 touch; PS03 contact/click portion. |
| [Haptics and impulse triggers](controller-haptics.md) | C08 feedback; PS04, XB03, NS03. |
| [Adaptive triggers](playstation-adaptive-triggers.md) | C08 force feedback; PS05. |
| [Nintendo NFC reader](nintendo-nfc-reader.md) | NS04. |
| [Controller audio](controller-audio.md) | C09, PS07, XB05, NS07. |
| [Capabilities and configuration](controller-capabilities-and-configuration.md) | C10 validation, settings, migration, and recovery. |
| [Parity qualification](controller-parity-qualification.md) | C11 measurement plus acceptance/regression evidence for all active IDs. |

NS08 remains out of scope and has no child specification. Licensing and console
authentication have no new implementation document in this pass. Existing
Bluetooth architecture and latency framework documents retain ownership of their
shared foundations; these drafts reference them instead of replacing them.

Update each draft's status, decisions, dependencies, and evidence when it changes.
Split a feature further only when separate hardware/protocol work warrants its
own owner, retaining links and requirement IDs. Review and approve a specification
before implementing it; document creation itself authorizes no firmware changes.

## Acceptance and Release Evidence

Maintain one qualification record for each combination of console model/system
version, reference controller/firmware, GP2040 commit/build target, physical
peripherals, transport, authentication dependency, and exercised feature ID.
Record pass/fail/blocker, reproducible steps, traces or measurements, and test date.
Separate direct native support from converter-assisted compatibility.

1. Compare system menus and representative games requiring ordinary controls,
  motion/touchpad, feedback/trigger effects, NFC, and audio as applicable.
   Include multiplayer assignment and simultaneous input combinations.
2. Exercise repeated pairing, reconnect, console/controller sleep, wake, USB
   hotplug, host switching, full discharge, charging, and interrupted settings
   writes. Verify bonds/calibration survive normal upgrades and power cycles.
3. Compare latency distributions, jitter, missed reports, and reconnect time to
   the first-party reference in the same setup. Preserve existing wired targets;
   do not impose an unsupported universal sub-millisecond wireless promise.
4. Measure RF interference/range, multi-controller load, battery runtime/standby,
   sensor drift, audio quality/latency, and simultaneous audio/haptic throughput.
   Set numerical thresholds and endurance durations before implementation signoff,
   not after seeing results; parity claims remain blocked until these are agreed.
5. Test malformed/truncated output traffic, unplugged peripherals, brownouts, and
   stalled streams. Stop actuators safely, bound queues, prevent loud audio
   transients, and keep input processing responsive. Protect bond/authentication
   material and microphone privacy; never distribute private credentials.
6. Run the preserved feature regression matrix and clean-slate Ninja configure/full
   builds for affected registered targets, including Standard Pico and a web-inclusive
   path when required. Record physical qualification separately from compile success.
7. Publish reproducible hardware BOMs/wiring, setup, update/recovery procedures,
   console/version support, transport-specific limits, and unresolved defects.
   A mandatory gap means the full-parity 1.0 gate is still open.

## Research Risks and Decisions

- Licensing and console authentication are deferred from the active feature backlog.
  Preserve integrations, record unavailable test prerequisites, and revisit these
  dependencies before claiming native compatibility; no bypass is proposed here.
- Current Pico/CYW43 resources may be insufficient for some concurrent radio,
  haptic, and audio workloads. Hardware extensions or additional supported targets
  need their own accepted specification; existing low-cost targets stay supported.
- Gamepad-only scope is confirmed for this roadmap revision. Legacy compatibility
  preservation is not a claim to reproduce every historical first-party accessory.
- Reference-controller firmware and console updates can change compatibility.
  Requalify against recorded versions and explicitly revise the baseline when needed.
- Official Nintendo specifications and detailed Xbox accessory behavior need a
  follow-up source/hardware check before this draft becomes an accepted contract.

## Reference Sources

- [Sony DualSense overview](https://www.playstation.com/en-us/accessories/dualsense-wireless-controller/):
  consulted on 2026-09-30 for the standard controller, haptics, adaptive triggers,
  and USB/Bluetooth distinction. Detailed transport behavior still needs hardware tests.
- [Xbox Wireless Controller overview](https://www.xbox.com/en-US/accessories/controllers/xbox-wireless-controller):
  official reference entry point; retrieval redirected to sign-in during this pass.
  Validate the detailed wireless/audio/accessory inventory against official support
  material and a reference controller before approving the specification.
- [Nintendo Switch 2 accessories](https://www.nintendo.com/us/gaming-systems/switch-2/accessories/):
  official reference entry point; retrieval was blocked during this pass. Nintendo
  rows are a requirements/research checklist, not verified protocol documentation.

This document changes planning only. No firmware, WebUI, hardware qualification,
or release certification is delivered by creating the roadmap.

## Scope History

| Date | Decision |
| --- | --- |
| 2026-09-30 | Narrowed parity to conventional gamepads; excluded Joy-Con families and retained NS08 as out of scope. Deferred licensing and console-authentication work. Adopted a living roadmap with individually owned feature specifications. |
