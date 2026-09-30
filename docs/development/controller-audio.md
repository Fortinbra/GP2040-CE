# Gamepad Audio, Microphones, and Headset Accessories

**Status:** Living draft; implementation requires review and approval.

**Date:** 2026-09-30

**Roadmap:** [Roadmap to 1.0](roadmap-to-1.0.md)

**Requirements:** C09, PS07, XB05, NS07. **Priority:** P2. **Milestones:** M0 research, M4 delivery.

## Goal and Current Foundation

Provide the reference gamepad's audio experience over each applicable transport.
Audio follows wireless in delivery priority but remains required for full parity.
[Bluetooth architecture](bluetooth-controller-architecture.md) reserves output
channel concepts; logging/discarding output and the existing buzzer addon are not
streamed headset, microphone, or speaker support.

## Per-Console Scope

| Reference | Required audio surface |
| --- | --- |
| DualSense | Stereo headset output, headset mic input, built-in speaker/mic, detection/routing, mute state/indicator, host volume controls, and supported USB/wireless paths. |
| Xbox Wireless Controller | Headset jack input/output and standard supported expansion/chat accessory behavior, with applicable volume/mute/chat controls on supported USB/Xbox Wireless paths. Do not assume PC Bluetooth carries headset audio. |
| Switch 2 Pro | Verify and implement headset-jack playback/microphone and supported controls/transports. Console GameChat and the console's built-in microphone remain console responsibilities. |
| Original Switch Pro | No headset jack; no invented controller-audio requirement. |

## Required Behavior

- Record native stream/control protocols, sample formats/rates, channel counts,
  codecs if required, packet cadence, and bandwidth independently per transport.
  Generic Bluetooth audio or USB Audio Class is not assumed console-compatible.
- Specify DAC/ADC/codec, amplifiers, mic bias, jack detection, analog routing,
  electrical protection, and reference headset/accessory wiring.
- Support full-duplex streams and host mute/volume/routing state with explicit
  precedence over local controls. Never transmit microphone samples while muted;
  initialize privacy state safely and keep the indicator truthful.
- Bound buffer depth, latency, clock drift correction, underrun/overrun, and stream
  restart. Silence invalid output and prevent loud startup/hotplug transients.
- Preserve gameplay under audio load, including haptics that share native stream
  resources. Handle headset hotplug, console suspend, handover, and disconnect.
- Keep audio optional on existing boards without removing established features.
  Do not add a new dependency or codec until protocol need, footprint, and reuse
  options are reviewed. Licensing and console authentication remain deferred.

## Integration and Dependencies

Console profiles own transport framing; dedicated audio hardware/stream handling
owns clocking, buffers, and physical I/O. Coordinate with
[haptics](controller-haptics.md), [system controls](controller-output-and-system-controls.md),
and [power lifecycle](controller-power-lifecycle.md). Budget USB endpoints, DMA,
CPU, memory, radio airtime, and battery current during M0, before hardware selection.

## Acceptance

1. Test playback and microphone concurrently with games/chat on supported paths,
   including reference accessories, mute/volume, detection, and routing changes.
2. Measure round-trip/audio latency, noise, level/clipping, channel separation,
   clock drift, and dropouts against the reference under sustained operation.
3. Test USB/radio loss, hotplug, sleep/wake, buffer starvation, malformed controls,
   and mute transitions; verify safe output and microphone privacy.
4. Stress simultaneous input, sensors, feedback, and audio; preserve wired latency
   and existing non-audio builds. Complete clean builds and physical tests.

## Open Decisions

Confirm native protocols and Switch 2 jack capabilities from reference hardware;
choose formats/codecs, analog hardware, queue/clock strategy, quality/latency
thresholds, and separate accessory coverage. Successful generic audio playback is
an intermediate experiment, not console parity.
