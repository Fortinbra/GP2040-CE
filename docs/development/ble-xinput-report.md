# Feature: XInput-Style BLE HID Report

**Author:** GP2040-CE core team  
**Requested by:** thegu  
**Status:** Design / Not yet implemented  
**Last updated:** 2026-05-25  
**SDK version:** 2.2.0

---

## Problem

The current BLE HID report descriptor presents the controller as a **32-button generic gamepad**.
This causes two user-visible problems on Windows, Android, and macOS:

1. **Too many anonymous buttons.** Hosts display "Button 1" through "Button 32" with no
   semantic labels.  GP2040-CE's internal 32-bit `buttons` bitmask (which spans B1–B4, L1/R1,
   L2/R2, S1/S2, L3/R3, A1–A4, E1–E12) is written directly into the report, so hosts cannot
   map face buttons to named actions (A, B, X, Y, etc.).

2. **Axes are unsigned 8-bit.** The stick axes (X, Y, Z, Rz) are declared as `0..255` with a
   hardware midpoint of `0x80`.  This differs from the XInput convention of signed 16-bit axes
   (`−32768..32767`), causing poor dead-zone detection in some games and drivers.

The D-pad is already a hat switch in the current descriptor — that part is correct.

---

## Goal

Replace the 9-byte generic report with a **13-byte XInput-style BLE HID report** that:

- Exposes exactly 11 named buttons (A, B, X, Y, LB, RB, Back, Start, Guide, LS, RS).
- Keeps the D-pad as a proper 4-bit hat switch (8 directions + neutral).
- Adds separate 8-bit unsigned trigger axes (LT, RT) instead of burying them in the button mask.
- Upgrades stick axes to **signed 16-bit** to match XInput USB conventions.

> **Note:** This is *not* the proprietary Xbox XInput protocol.  True XInput requires Microsoft's
> vendor-specific USB class interface and cannot run over BLE.  The goal is a standard BLE HID
> descriptor whose data *layout* mirrors XInput so that Windows / Android / macOS recognise the
> device as a proper gamepad rather than a raw button board.

---

## Current Implementation

### GATT database (`src/ble_hid.gatt`)

The GATT file is unchanged by this feature.  It declares the service structure but carries no
HID descriptor bytes:

```gatt
PRIMARY_SERVICE, ORG_BLUETOOTH_SERVICE_HUMAN_INTERFACE_DEVICE
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_PROTOCOL_MODE, DYNAMIC | READ | WRITE_WITHOUT_RESPONSE,
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_REPORT, DYNAMIC | READ | NOTIFY,
REPORT_REFERENCE, READ, 1, 1
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_REPORT_MAP, DYNAMIC | READ,
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_HID_INFORMATION, READ, 01 01 00 02
CHARACTERISTIC, ORG_BLUETOOTH_CHARACTERISTIC_HID_CONTROL_POINT, DYNAMIC | WRITE_WITHOUT_RESPONSE,
```

The `REPORT_MAP` characteristic is marked `DYNAMIC | READ`; BTstack's HIDS service returns the
`hid_report_descriptor[]` array that is passed to `hids_device_init()` in `BLEHIDManager.cpp`.

### HID report descriptor (current — in `BLEHIDManager.cpp`)

| Field             | Type           | Size   |
|-------------------|---------------|--------|
| 32 buttons        | Bit fields     | 4 bytes|
| Hat switch        | 4-bit, 0–7 / null | 4 bits |
| Padding           | Constant       | 4 bits |
| X (LX)            | uint8, 0–255   | 1 byte |
| Y (LY)            | uint8, 0–255   | 1 byte |
| Z (RX)            | uint8, 0–255   | 1 byte |
| Rz (RY)           | uint8, 0–255   | 1 byte |
| **Total**         |               | **9 bytes** |

Report ID 1.  The Report ID byte is **not** included in the ATT notification payload — it is
communicated via the GATT Report Reference descriptor (`REPORT_REFERENCE, READ, 1, 1`).

### Report builder (`src/OutputManager.cpp`)

```cpp
uint8_t report[9] = {};
report[0..3] = state.buttons (raw 32-bit bitmask, little-endian);
report[4]    = hat (4 bits, lower nibble) | padding (4 bits, upper nibble = 0);
report[5]    = (uint8_t)(state.lx >> 8);   // uint16 [0,65535] → uint8 [0,255]
report[6]    = (uint8_t)(state.ly >> 8);
report[7]    = (uint8_t)(state.rx >> 8);
report[8]    = (uint8_t)(state.ry >> 8);
```

### Buffer sizes (`headers/BLEHIDManager.h`)

```cpp
uint8_t _pendingReport[9]   = {};
uint8_t _lastSentReport[9]  = {};
```

`sendReport()` clamps to `len = 9` if the caller passes a longer buffer.

---

## Proposed Solution

### Report layout (13 bytes, no Report ID in payload)

```
Byte  0:   Button byte 0  — bit 0=A, 1=B, 2=X, 3=Y, 4=LB, 5=RB, 6=Back, 7=Start
Byte  1:   Button byte 1  — bit 0=Guide, 1=LS, 2=RS, bits 3–7=reserved (0)
Byte  2:   Hat (lower 4 bits: 0=N 1=NE 2=E 3=SE 4=S 5=SW 6=W 7=NW 8=neutral)
            + 4-bit padding (upper nibble = 0)
Byte  3:   LT  — uint8, 0..255
Byte  4:   RT  — uint8, 0..255
Bytes 5–6:  LX  — int16 LE, −32768..32767
Bytes 7–8:  LY  — int16 LE, −32768..32767  (positive = up, matches XInput)
Bytes 9–10: RX  — int16 LE, −32768..32767
Bytes 11–12: RY  — int16 LE, −32768..32767 (positive = up)
```

> **Total ATT notification payload: 13 bytes.**

---

## HID Descriptor (C array for `BLEHIDManager.cpp`)

The GATT file is **not** changed.  Replace `hid_report_descriptor[]` in `BLEHIDManager.cpp`:

```cpp
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,              // USAGE (Game Pad)
    0xA1, 0x01,              // COLLECTION (Application)

    0x85, 0x01,              // Report ID (1)

    // ── 11 digital buttons (2 bytes total) ─────────────────────────────────
    // Buttons 1–8: A, B, X, Y, LB, RB, Back, Start
    0x05, 0x09,              //   USAGE_PAGE (Button)
    0x19, 0x01,              //   USAGE_MINIMUM (Button 1)
    0x29, 0x08,              //   USAGE_MAXIMUM (Button 8)
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x25, 0x01,              //   LOGICAL_MAXIMUM (1)
    0x95, 0x08,              //   REPORT_COUNT (8)
    0x75, 0x01,              //   REPORT_SIZE (1)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    // Buttons 9–11: Guide, LS, RS
    0x19, 0x09,              //   USAGE_MINIMUM (Button 9)
    0x29, 0x0B,              //   USAGE_MAXIMUM (Button 11)
    0x95, 0x03,              //   REPORT_COUNT (3)
    0x75, 0x01,              //   REPORT_SIZE (1)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    // 5 padding bits to complete byte 1
    0x95, 0x05,              //   REPORT_COUNT (5)
    0x75, 0x01,              //   REPORT_SIZE (1)
    0x81, 0x03,              //   INPUT (Cnst,Var,Abs)

    // ── D-pad as hat switch (1 byte total) ─────────────────────────────────
    0x05, 0x01,              //   USAGE_PAGE (Generic Desktop)
    0x09, 0x39,              //   USAGE (Hat switch)
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x25, 0x07,              //   LOGICAL_MAXIMUM (7)
    0x35, 0x00,              //   PHYSICAL_MINIMUM (0)
    0x46, 0x3B, 0x01,        //   PHYSICAL_MAXIMUM (315 = 7×45 degrees)
    0x65, 0x14,              //   UNIT (Eng Rot: Angular Position)
    0x75, 0x04,              //   REPORT_SIZE (4)
    0x95, 0x01,              //   REPORT_COUNT (1)
    0x81, 0x42,              //   INPUT (Data,Var,Abs,Null)

    // 4 padding bits to complete the hat byte
    0x65, 0x00,              //   UNIT (None)
    0x75, 0x04,              //   REPORT_SIZE (4)
    0x95, 0x01,              //   REPORT_COUNT (1)
    0x81, 0x03,              //   INPUT (Cnst,Var,Abs)

    // ── Analog triggers: LT, RT (2 bytes total) ────────────────────────────
    0x05, 0x02,              //   USAGE_PAGE (Simulation Controls)
    0x09, 0xC5,              //   USAGE (Brake)       = LT
    0x09, 0xC4,              //   USAGE (Accelerator) = RT
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,        //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,              //   REPORT_SIZE (8)
    0x95, 0x02,              //   REPORT_COUNT (2)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    // ── Analog sticks: LX, LY, RX, RY (8 bytes total) ─────────────────────
    0x05, 0x01,              //   USAGE_PAGE (Generic Desktop)
    0x09, 0x30,              //   USAGE (X)  = LX
    0x09, 0x31,              //   USAGE (Y)  = LY
    0x09, 0x32,              //   USAGE (Z)  = RX
    0x09, 0x35,              //   USAGE (Rz) = RY
    0x16, 0x00, 0x80,        //   LOGICAL_MINIMUM (-32768)
    0x26, 0xFF, 0x7F,        //   LOGICAL_MAXIMUM (32767)
    0x75, 0x10,              //   REPORT_SIZE (16)
    0x95, 0x04,              //   REPORT_COUNT (4)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    0xC0,                    // END_COLLECTION
};
```

**Descriptor byte budget:**

| Section            | Report bits | Bytes in payload |
|--------------------|-------------|-----------------|
| Buttons 1–8        | 8           | 1               |
| Buttons 9–11 + pad | 3 + 5 = 8   | 1               |
| Hat + pad          | 4 + 4 = 8   | 1               |
| LT + RT            | 8 + 8 = 16  | 2               |
| LX, LY, RX, RY     | 16 × 4 = 64 | 8               |
| **Total**          | **104 bits**| **13 bytes**    |

> **HID signed-value encoding note:** `LOGICAL_MINIMUM(-32768)` is encoded as the 2-byte item
> `0x16, 0x00, 0x80` (tag 0x16 = Global LOGICAL_MINIMUM 2-byte; value `0x8000` in
> little-endian = −32768).  `LOGICAL_MAXIMUM(32767)` is `0x26, 0xFF, 0x7F`.

---

## Gamepad State Mapping

GP2040-CE canonical button names → XInput-style BLE report bytes:

| GP2040 button | XInput name | Report byte | Bit |
|---------------|-------------|-------------|-----|
| B1            | A           | 0           | 0   |
| B2            | B           | 0           | 1   |
| B3            | X           | 0           | 2   |
| B4            | Y           | 0           | 3   |
| L1            | LB          | 0           | 4   |
| R1            | RB          | 0           | 5   |
| S1            | Back        | 0           | 6   |
| S2            | Start       | 0           | 7   |
| A1            | Guide       | 1           | 0   |
| L3            | LS          | 1           | 1   |
| R3            | RS          | 1           | 2   |
| *(reserved)*  | —           | 1           | 3–7 |

D-pad hat encoding (byte 2, lower nibble):

| GP2040 dpad state          | Hat value |
|----------------------------|-----------|
| `UP`                       | 0 (N)     |
| `UP + RIGHT`               | 1 (NE)    |
| `RIGHT`                    | 2 (E)     |
| `DOWN + RIGHT`             | 3 (SE)    |
| `DOWN`                     | 4 (S)     |
| `DOWN + LEFT`              | 5 (SW)    |
| `LEFT`                     | 6 (W)     |
| `UP + LEFT`                | 7 (NW)    |
| *(any other / neutral)*    | 8 (null)  |

Hat null value: HID declares `LOGICAL_MAXIMUM(7)` with the `Null State` flag (`0x81, 0x42`).
Any value > 7 in the 4-bit field (i.e., `0x8`–`0xF`) is treated as "no direction" by all
conforming hosts.

Trigger mapping (bytes 3–4):

| GP2040 action                         | Report byte | Value  |
|---------------------------------------|-------------|--------|
| L2 digital pressed (no analog)        | 3 (LT)      | 0xFF   |
| L2 analog (`state.lt`)                | 3 (LT)      | `state.lt` |
| R2 digital pressed (no analog)        | 4 (RT)      | 0xFF   |
| R2 analog (`state.rt`)                | 4 (RT)      | `state.rt` |

Stick axis conversion (bytes 5–12):

GP2040-CE stores axes as `uint16_t` in `[0, 65535]` with midpoint `GAMEPAD_JOYSTICK_MID = 0x7FFF`.
XInput convention uses signed `int16_t` in `[−32768, 32767]` with center ≈ 0.
The Y axes are **inverted**: GP2040 Y=0 means "up" but XInput positive-Y means "up".

```cpp
// Matches XInputDriver.cpp axis conversion exactly
int16_t lx = static_cast<int16_t>(state.lx) + INT16_MIN;
int16_t ly = static_cast<int16_t>(~state.ly) + INT16_MIN;  // invert Y
int16_t rx = static_cast<int16_t>(state.rx) + INT16_MIN;
int16_t ry = static_cast<int16_t>(~state.ry) + INT16_MIN;  // invert Y
```

Little-endian store (matching the `int16_t LE` declaration in the HID descriptor):

```cpp
report[5]  = (uint8_t)(lx & 0xFF);       report[6]  = (uint8_t)(lx >> 8);
report[7]  = (uint8_t)(ly & 0xFF);       report[8]  = (uint8_t)(ly >> 8);
report[9]  = (uint8_t)(rx & 0xFF);       report[10] = (uint8_t)(rx >> 8);
report[11] = (uint8_t)(ry & 0xFF);       report[12] = (uint8_t)(ry >> 8);
```

---

## Proposed `OutputManager.cpp` Implementation

Replace the current `dispatch()` body (inside `#ifdef ENABLE_BLUETOOTH`):

```cpp
void OutputManager::dispatch(Gamepad* gamepad) {
#ifdef ENABLE_BLUETOOTH
    const GamepadOptions& opts = Storage::getInstance().getGamepadOptions();
    if (opts.inputMode != INPUT_MODE_BLE) return;

    uint8_t report[13] = {};
    const GamepadState& state = gamepad->state;

    // Byte 0: face + shoulder + menu buttons
    report[0] =
        (gamepad->pressedB1() ? (1u << 0) : 0) |  // A
        (gamepad->pressedB2() ? (1u << 1) : 0) |  // B
        (gamepad->pressedB3() ? (1u << 2) : 0) |  // X
        (gamepad->pressedB4() ? (1u << 3) : 0) |  // Y
        (gamepad->pressedL1() ? (1u << 4) : 0) |  // LB
        (gamepad->pressedR1() ? (1u << 5) : 0) |  // RB
        (gamepad->pressedS1() ? (1u << 6) : 0) |  // Back
        (gamepad->pressedS2() ? (1u << 7) : 0);   // Start

    // Byte 1: Guide + stick clicks (bits 3–7 = reserved/0)
    report[1] =
        (gamepad->pressedA1() ? (1u << 0) : 0) |  // Guide
        (gamepad->pressedL3() ? (1u << 1) : 0) |  // LS
        (gamepad->pressedR3() ? (1u << 2) : 0);   // RS

    // Byte 2: hat (lower 4 bits) + padding (upper 4 bits = 0)
    uint8_t hat;
    switch (state.dpad & GAMEPAD_MASK_DPAD) {
        case GAMEPAD_MASK_UP:                          hat = 0; break;
        case GAMEPAD_MASK_UP    | GAMEPAD_MASK_RIGHT:  hat = 1; break;
        case GAMEPAD_MASK_RIGHT:                       hat = 2; break;
        case GAMEPAD_MASK_DOWN  | GAMEPAD_MASK_RIGHT:  hat = 3; break;
        case GAMEPAD_MASK_DOWN:                        hat = 4; break;
        case GAMEPAD_MASK_DOWN  | GAMEPAD_MASK_LEFT:   hat = 5; break;
        case GAMEPAD_MASK_LEFT:                        hat = 6; break;
        case GAMEPAD_MASK_UP    | GAMEPAD_MASK_LEFT:   hat = 7; break;
        default:                                       hat = 8; break;
    }
    report[2] = hat & 0x0F;

    // Bytes 3–4: triggers (digital falls back to 0xFF; analog uses state.lt/rt)
    if (gamepad->hasAnalogTriggers) {
        report[3] = gamepad->pressedL2() ? 0xFF : state.lt;
        report[4] = gamepad->pressedR2() ? 0xFF : state.rt;
    } else {
        report[3] = gamepad->pressedL2() ? 0xFF : 0;
        report[4] = gamepad->pressedR2() ? 0xFF : 0;
    }

    // Bytes 5–12: sticks as int16 LE, Y axes inverted to match XInput convention
    int16_t lx = static_cast<int16_t>(state.lx) + INT16_MIN;
    int16_t ly = static_cast<int16_t>(~state.ly) + INT16_MIN;
    int16_t rx = static_cast<int16_t>(state.rx) + INT16_MIN;
    int16_t ry = static_cast<int16_t>(~state.ry) + INT16_MIN;

    report[5]  = (uint8_t)(lx & 0xFF);  report[6]  = (uint8_t)((uint16_t)lx >> 8);
    report[7]  = (uint8_t)(ly & 0xFF);  report[8]  = (uint8_t)((uint16_t)ly >> 8);
    report[9]  = (uint8_t)(rx & 0xFF);  report[10] = (uint8_t)((uint16_t)rx >> 8);
    report[11] = (uint8_t)(ry & 0xFF);  report[12] = (uint8_t)((uint16_t)ry >> 8);

    BLEHIDManager::getInstance().sendReport(report, sizeof(report));
#else
    (void)gamepad;
#endif
}
```

---

## Files to Change

| File | Change |
|------|--------|
| `src/BLEHIDManager.cpp` | Replace `hid_report_descriptor[]` with the 13-byte XInput-style array above |
| `src/BLEHIDManager.cpp` | In `sendReport()`: change `if (len > 9) len = 9;` → `if (len > 13) len = 13;` |
| `headers/BLEHIDManager.h` | Resize `_pendingReport[9]` and `_lastSentReport[9]` to `[13]`; update doc comment on `sendReport()` |
| `src/OutputManager.cpp` | Replace 9-byte report builder with the 13-byte XInput-style builder above |

**The `src/ble_hid.gatt` file does NOT change.** It defines the GATT service structure, not the
HID report descriptor contents.  The descriptor is passed dynamically via `hids_device_init()`.

---

## Compatibility

### Windows 10/11

Windows recognises the device as a **generic HID gamepad** (not an XInput controller).
True XInput recognition requires Microsoft's proprietary vendor-specific USB class and the
`XUSB.sys` driver — neither of which is available over BLE.

What users get on Windows:
- Device appears in **Settings → Bluetooth → "GP2040-CE Gamepad"**
- In DirectInput / SDL2 / XInput-wrapper apps: 11 buttons with semantic names, hat switch, and
  proper analog sticks are exposed.
- Windows 10+ **Bluetooth HID gamepad** profile handles it natively; no extra driver needed.
- Games using **SDL2** (`SDL_GameControllerDB`) will auto-map face buttons via the DB once
  a mapping entry is added for the device GUID.

### Android

Android's BLE HID stack handles this descriptor correctly.  The device appears as a standard
gamepad in `InputDevice.SOURCE_GAMEPAD`.  Button labels depend on the launcher / game.

### macOS (12+)

Recognised as a generic HID gamepad.  Analog sticks and hat switch work.  Button labels vary
by application.

### iOS

BLE HID gamepad support on iOS is limited and version-dependent.  Simple button and stick input
works in apps that explicitly support MFi or third-party BLE controllers, but system-level
gamepad support is incomplete.  iOS is **not a primary target** for this feature.

### Nintendo Switch

Switch requires **BT Classic HID** and does not support BLE HID gamepads at all.
This descriptor has no effect on Switch compatibility.

---

## Implementation Notes

### Re-pairing required

After changing the HID report descriptor, the **host must delete the existing pairing and
re-pair**.  The GATT Database Hash (characteristic 0x2B2A) changes when `REPORT_MAP` content
changes, and the host's cached descriptor is invalid.  The new 13-byte report will be silently
ignored or mis-parsed until re-pairing occurs.

Inform users: *"Delete 'GP2040-CE Gamepad' from your Bluetooth settings and re-pair."*

### Hat switch null-state encoding

The HID descriptor declares `LOGICAL_MAXIMUM(7)` with the `Null State` flag (`0x42` in the
INPUT item).  Any 4-bit value > 7 signals "no direction pressed".  We use `8` (0x8) for neutral.
Values 9–15 (0x9–0xF) are also accepted as null by conforming hosts.

### Report ID: not in ATT payload

The Report ID (`0x01`) is declared in the HID descriptor and referenced by the GATT Report
Reference descriptor (`REPORT_REFERENCE, READ, 1, 1`).  It is **not** prepended to the
13-byte ATT notification payload.  `hids_device_send_input_report()` handles the GATT framing.

### Y-axis inversion

GP2040-CE stores Y axes with `0 = up` (minimum = full up press).  XInput convention uses
`positive = up`.  The `~state.ly` bitwise inversion combined with `+ INT16_MIN` is the same
formula used by `XInputDriver.cpp` for the USB XInput output.  Any existing calibration or
SOCD-cleaning logic in the gamepad pipeline applies before `OutputManager::dispatch()`, so no
additional adjustment is needed here.

### `hasAnalogTriggers` flag

The trigger bytes are populated using the same `hasAnalogTriggers` guard that `XInputDriver.cpp`
uses: when the flag is false (digital-only board), L2/R2 are mapped to 0x00 or 0xFF; when true,
`state.lt` / `state.rt` (0..255) are used directly.

### BTstack buffer size

`BLEHIDManager` holds the pending report in a fixed-size array.  After this change the array
must be at least 13 bytes:

```cpp
// headers/BLEHIDManager.h
uint8_t _pendingReport[13]    = {};
uint8_t _lastSentReport[13]   = {};
```

The `sendReport()` guard must also be updated:

```cpp
// src/BLEHIDManager.cpp
if (len > 13) len = 13;
```

### Connection interval

For low-latency gaming the BLE connection interval should be requested at 7.5 ms (6 × 1.25 ms).
This is independent of this descriptor change.  See the existing commented-out
`gap_request_connection_parameter_update()` call in `BLEHIDManager.cpp`.

---

## Buttons Intentionally Omitted

GP2040-CE has 16 standard buttons (B1–B4, L1/R1, L2/R2, S1/S2, L3/R3, A1–A4) plus 12 extra
(E1–E12).  This descriptor maps 11 of those to named XInput-equivalent buttons.  The unmapped
buttons are:

| GP2040 button | Reason omitted |
|---------------|----------------|
| L2, R2        | Moved to dedicated trigger bytes |
| A2            | No XInput equivalent (Capture); can be added as Button 12 if needed |
| A3, A4        | No XInput equivalent |
| E1–E12        | Extra buttons with no XInput equivalent |

If future requirements call for exposing A2 or the E-buttons, extend the button count and adjust
the padding field count in the HID descriptor accordingly.

---

## Implementation

**Status:** Implemented 2026-05-25  
**Build:** Clean (uild_ble3, PimoroniPicoLipo2XLW target)

### Files Changed

| File | Change |
|------|--------|
| src/BLEHIDManager.cpp | Replaced hid_report_descriptor[] with 13-byte XInput-style descriptor; updated sendReport() length clamp from 9 to 13 |
| headers/BLEHIDManager.h | Resized _pendingReport[9] and _lastSentReport[9] to [13]; updated sendReport() doc comment |
| src/OutputManager.cpp | Replaced 9-byte generic report builder with 13-byte XInput-style builder |

### Actual Report Size

**13 bytes** — no deviations from the design doc layout:
- Byte 0: buttons 1–8 (A, B, X, Y, LB, RB, Back, Start)
- Byte 1: buttons 9–11 (Guide, LS, RS) + 5 reserved bits
- Byte 2: hat switch (lower 4 bits, null=8) + 4-bit padding
- Bytes 3–4: LT, RT (uint8, 0..255)
- Bytes 5–12: LX, LY, RX, RY (int16 LE, signed)

### Axis Formula

Identical to XInputDriver.cpp:
- lx = static_cast<int16_t>(state.lx) + INT16_MIN
- ly = static_cast<int16_t>(~state.ly) + INT16_MIN  (Y inverted)
- x = static_cast<int16_t>(state.rx) + INT16_MIN
- y = static_cast<int16_t>(~state.ry) + INT16_MIN  (Y inverted)

### Re-pairing Required

After flashing this firmware, hosts must delete the existing BLE pairing and re-pair.
The GATT Database Hash changes when REPORT_MAP content changes; the host's cached
descriptor will mis-parse the new 13-byte reports until re-pairing occurs.
