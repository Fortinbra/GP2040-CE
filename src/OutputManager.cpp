#include "OutputManager.h"

#include "enums.pb.h"
#include "storagemanager.h"
#include "gamepad.h"
#include "drivers/hid/HIDDescriptors.h"

#ifdef ENABLE_BLUETOOTH
#include "BLEHIDManager.h"
#endif

void OutputManager::dispatch(Gamepad* gamepad) {
#ifdef ENABLE_BLUETOOTH
    const GamepadOptions& opts = Storage::getInstance().getGamepadOptions();
    if (opts.inputMode != INPUT_MODE_BLE) return;

    // Map GamepadState to the 3-byte digital-only BLE HID report body (no Report ID byte —
    // that is carried by the GATT Report Reference descriptor). See
    // docs/development/ble-hid-report-rework.md.
    //   byte 0 : Buttons 1..8   (B1, B2, B3, B4, L1, R1, L2, R2)
    //   byte 1 : Buttons 9..14  (S1, S2, L3, R3, A1, A2) + 2 padding bits
    //   byte 2 : Hat switch (lower nibble 0..7 = direction, 8 = neutral) + 4 padding bits
    uint8_t report[3] = {};
    const GamepadState& state = gamepad->state;

    // Byte 0: Buttons 1..8
    report[0] =
        (gamepad->pressedB1() ? (1u << 0) : 0) |  // Button 1
        (gamepad->pressedB2() ? (1u << 1) : 0) |  // Button 2
        (gamepad->pressedB3() ? (1u << 2) : 0) |  // Button 3
        (gamepad->pressedB4() ? (1u << 3) : 0) |  // Button 4
        (gamepad->pressedL1() ? (1u << 4) : 0) |  // Button 5
        (gamepad->pressedR1() ? (1u << 5) : 0) |  // Button 6
        (gamepad->pressedL2() ? (1u << 6) : 0) |  // Button 7
        (gamepad->pressedR2() ? (1u << 7) : 0);   // Button 8

    // Byte 1: Buttons 9..14 (bits 6..7 = padding = 0)
    report[1] =
        (gamepad->pressedS1() ? (1u << 0) : 0) |  // Button 9
        (gamepad->pressedS2() ? (1u << 1) : 0) |  // Button 10
        (gamepad->pressedL3() ? (1u << 2) : 0) |  // Button 11
        (gamepad->pressedR3() ? (1u << 3) : 0) |  // Button 12
        (gamepad->pressedA1() ? (1u << 4) : 0) |  // Button 13
        (gamepad->pressedA2() ? (1u << 5) : 0);   // Button 14

    // Byte 2: hat switch (lower 4 bits) + padding (upper 4 bits = 0)
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
        default:                                       hat = 8; break;  // null state (>7)
    }
    report[2] = hat & 0x0F;

    BLEHIDManager::getInstance().sendReport(report, sizeof(report));
#else
    (void)gamepad;
#endif
}
