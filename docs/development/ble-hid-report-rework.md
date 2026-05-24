# BLE HID Report Rework (Digital-Only v1)

**Last updated:** 2026-05-23
**Maintained by:** GP2040-CE core team
**Status:** Feature spec — implementation pending sign-off
**Related:** [bluetooth-support.md](./bluetooth-support.md)

---

## Purpose

Replace the current 13-byte XInput-style BLE HID Input Report and its
descriptor with a compact, **digital-only** gamepad report that:

1. Exposes every digital button GP2040-CE actually owns on the wire (not
   just the 11 that the v1 descriptor covers).
2. Stops sending analog stick and analog trigger fields whose contents
   are not backed by real analog hardware and which currently bleed
   spurious deflection to the host.
3. Keeps the BLE side honest about controller capability until a proper
   analog input pipeline lands as its own feature.

This is intentionally a **breaking** change. Hosts that have already
bonded with the controller cache the old HID report map and must unpair
and re-pair to pick up the new descriptor.

## Scope

In scope:

- Rewrite `hid_report_descriptor[]` in `src/BLEHIDManager.cpp`.
- Rewrite the report-packing block in `OutputManager::dispatch()`
  (`src/OutputManager.cpp`).
- Adjust `REPORT_SIZE_BYTES` / payload size in `src/BLEHIDManager.cpp`
  if a named constant exists; otherwise update the local
  `uint8_t report[N]` size and the `sendReport(report, sizeof(report))`
  call site.
- Document the unpair / re-pair requirement on the BLE feature page.

Out of scope (deferred):

- Real analog sticks. Tracked as a future feature once analog input
  plumbing exists for BLE.
- Real analog triggers (`Z`, `Rz`). Same as above.
- Console-specific HID layouts (XInput cluster, DS4 cluster, Switch
  cluster). v1 is **generic HID buttons**. Console mapping is a
  separate feature.
- Force feedback, rumble, LED reports, consumer-control reports.
- Re-keying which `GAMEPAD_MASK_*` bit drives which HID button index.
  v1 keeps a straight, ordered mapping (see below) and does not try to
  match any specific console silkscreen.

## Problems With the Current Report

Confirmed against `src/BLEHIDManager.cpp` and `src/OutputManager.cpp`:

1. **Buttons under-reported.** The button bitmap covers 11 inputs:
   B1, B2, B3, B4, L1, R1, S1, S2, A1, L3, R3. Notably absent:
   - **L2 / R2 digital.** They are *only* emitted as the analog
     trigger spike (0xFF). Hosts that ignore the trigger axes never
     see L2/R2 pressed.
   - **A2** (capture / touchpad-click).
   - **A3, A4** (board-defined extras).
2. **Spurious analog deflection.** Sticks are sent as
   `int16_t lx = (int16_t)state.lx + INT16_MIN` (with `~state.ly`
   inversion). On a build with no analog hardware,
   `state.lx/ly/rx/ry` are not guaranteed to be the XInput center
   `0x8000`. The `~` inversion on `ly` and `ry` also yields `-1` instead
   of `0` when the input is `0x8000`, producing a permanent
   one-bit deflection on both Y axes. End result: the host sees the
   controller pushing slightly down forever.
3. **Spurious trigger pressure.** When `gamepad->hasAnalogTriggers`
   is false the code currently sends `0` for both axes, which is
   correct in isolation but still consumes two report bytes and two
   HID Usage axes that the controller cannot honestly drive.
4. **Padding bits.** Byte 1 declares 3 button bits + 5 explicit pad
   bits via a separate constant. The new descriptor should use a
   single contiguous Buttons usage range with arithmetic padding.

## Target Report

### Button assignment (14 generic HID buttons)

The report exposes 14 buttons, numbered HID Button 1..14 in this
order:

| HID Button | Source                       |
|------------|------------------------------|
| 1          | `GAMEPAD_MASK_B1`            |
| 2          | `GAMEPAD_MASK_B2`            |
| 3          | `GAMEPAD_MASK_B3`            |
| 4          | `GAMEPAD_MASK_B4`            |
| 5          | `GAMEPAD_MASK_L1`            |
| 6          | `GAMEPAD_MASK_R1`            |
| 7          | `GAMEPAD_MASK_L2` (digital)  |
| 8          | `GAMEPAD_MASK_R2` (digital)  |
| 9          | `GAMEPAD_MASK_S1`            |
| 10         | `GAMEPAD_MASK_S2`            |
| 11         | `GAMEPAD_MASK_L3`            |
| 12         | `GAMEPAD_MASK_R3`            |
| 13         | `GAMEPAD_MASK_A1`            |
| 14         | `GAMEPAD_MASK_A2`            |

Rationale for 14: matches the XInput slot count one-for-one (4 face,
2 shoulder, 2 trigger, 2 menu, 2 stick-click, 2 system) without
reserving headroom that we have no plan to fill in v1. `A3`/`A4`/`E1+`
are deferred to a follow-up if a real consumer asks for them.

### Wire layout

```
Report ID 1
Byte 0   : Buttons  1..8   (B1, B2, B3, B4, L1, R1, L2, R2)
Byte 1   : Buttons  9..14 + 2 pad bits
            bit 0 = S1
            bit 1 = S2
            bit 2 = L3
            bit 3 = R3
            bit 4 = A1
            bit 5 = A2
            bits 6..7 = constant 0 (padding)
Byte 2   : Hat switch  (lower nibble 0..7 = direction, 8 = neutral;
                        upper nibble = constant 0 padding)
```

Total payload after the Report ID: **3 bytes**. `sendReport()` and
the local `uint8_t report[N]` in `OutputManager::dispatch()` size to
3.

### HID Report Descriptor outline

Plain-language structure for the new descriptor:

- `Usage Page (Generic Desktop)`
- `Usage (Gamepad)`
- `Collection (Application)`
  - `Report ID (1)`
  - **Buttons block**
    - `Usage Page (Button)`
    - `Usage Minimum (Button 1)`
    - `Usage Maximum (Button 14)`
    - `Logical Minimum (0)`
    - `Logical Maximum (1)`
    - `Report Size (1)`
    - `Report Count (14)`
    - `Input (Data, Var, Abs)`
    - `Report Size (1)`
    - `Report Count (2)`
    - `Input (Const, Var, Abs)`       // 2-bit padding to byte boundary
  - **Hat block**
    - `Usage Page (Generic Desktop)`
    - `Usage (Hat switch)`
    - `Logical Minimum (0)`
    - `Logical Maximum (7)`
    - `Physical Minimum (0)`
    - `Physical Maximum (315)`
    - `Unit (Degrees, English Rotation)`
    - `Report Size (4)`
    - `Report Count (1)`
    - `Input (Data, Var, Abs, Null)`
    - `Report Size (4)`
    - `Report Count (1)`
    - `Input (Const, Var, Abs)`       // 4-bit padding to byte boundary
- `End Collection`

No `Usage (X/Y/Z/Rz)`. No analog trigger axes. No `Brake` /
`Accelerator` usages.

Appearance in advertising data stays `0x03C4` (Gamepad).

## Implementation Plan

1. **Descriptor.** Replace `hid_report_descriptor[]` in
   `src/BLEHIDManager.cpp` with the layout above. Keep the existing
   `static_assert(sizeof(hid_report_descriptor) > 0, ...)`.
2. **Report packing.** In `src/OutputManager.cpp`,
   `OutputManager::dispatch()`:
   - Replace the existing 13-byte body with a 3-byte body.
   - Pack buttons as listed in the table above.
   - Reuse the existing 8-direction `dpad` → hat-value switch (with
     `8` as the neutral sentinel) for byte 2.
   - Delete the stick and trigger packing entirely (`lx/ly/rx/ry/lt/rt`
     are no longer referenced from this path).
3. **Size sync.** `sendReport(report, sizeof(report))` continues to use
   `sizeof(report)`, so once the array shrinks to 3 the GATT side is
   consistent. Verify there is no hardcoded `13` elsewhere in
   `BLEHIDManager.cpp` for the input report; if so, update or remove.
4. **GATT / Report Reference.** No changes expected — the BLE HID
   service advertises the Report Map blob as-is; hosts re-read it on
   re-pair.
5. **Re-pair guidance.** Add a short note to
   [bluetooth-support.md](./bluetooth-support.md) under "Host
   Compatibility" or the operational checklist:
   "After upgrading firmware across this change, remove the existing
   pairing on every host before connecting; cached HID Report Map data
   from older firmware will misrepresent the new layout."

## Verification Plan

Per `clean-build-required.instructions.md`, end with a clean `--fresh`
configure plus full Ninja build for `PimoroniPicoLipo2XLW`
(`PICO_BOARD=pico2_w`, `GP2040_BOARDCONFIG=PimoroniPicoLipo2XLW`,
`SKIP_WEBBUILD=TRUE`).

On hardware, after unpair / re-pair on each host under test:

- Each of the 14 buttons in the table produces exactly one HID button
  press observable in the OS gamepad tester. No additional ghost
  buttons.
- D-pad reports the 8 cardinals and diagonals via the hat switch;
  releasing all directions reports neutral (no last-direction sticking
  at center).
- Sticks and triggers no longer appear in the host's gamepad axes
  list at all (or, if axes are still shown by the OS for a 0-axis
  Gamepad collection, all read static 0).
- Battery and BLE-status reporting remain unchanged (covered by the
  prior feature; not regressed here).

## Risk and Rollback

- **Risk:** existing bonds on user devices break silently; gameplay
  bindings configured against the old report will need to be redone.
  Mitigated by the re-pair note and by treating this as a deliberate
  v1 of a new report shape.
- **Rollback:** revert the two source-file changes
  (`src/BLEHIDManager.cpp`, `src/OutputManager.cpp`) and ask hosts to
  re-pair again. No persistent storage or schema is involved.

## Open Questions

None blocking. Future follow-ups (not part of this feature):

- Promotion to `XInput`/`DS4`/`Switch`-shaped BLE reports.
- Real analog input on BLE.
- Exposing `A3`, `A4`, and the `E1..En` extra set when a board needs
  them.
