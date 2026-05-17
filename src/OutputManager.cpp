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

    // Map GamepadState to the 13-byte XInput-style BLE HID report body (no Report ID byte — that
    // is carried by the GATT Report Reference descriptor):
    //   byte  0    : face + shoulder + menu buttons (A,B,X,Y,LB,RB,Back,Start)
    //   byte  1    : Guide, LS, RS (bits 3–7 = reserved/0)
    //   byte  2    : hat switch (lower 4 bits, 0–7 or 8=neutral) + padding (upper 4 bits = 0)
    //   bytes 3–4  : LT, RT (uint8, 0..255)
    //   bytes 5–12 : LX, LY, RX, RY (int16 LE, signed −32768..32767, Y axes inverted)
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
        default:                                       hat = 8; break;  // null state (>7)
    }
    report[2] = hat & 0x0F;

    // Bytes 3–4: triggers (digital falls back to 0xFF; analog uses state.lt/rt directly)
    if (gamepad->hasAnalogTriggers) {
        report[3] = gamepad->pressedL2() ? 0xFF : state.lt;
        report[4] = gamepad->pressedR2() ? 0xFF : state.rt;
    } else {
        report[3] = gamepad->pressedL2() ? 0xFF : 0;
        report[4] = gamepad->pressedR2() ? 0xFF : 0;
    }

    // Bytes 5–12: sticks as int16 LE, Y axes inverted to match XInput convention
    // (XInput: positive Y = up; GP2040-CE stores 0 = up, so invert via bitwise NOT)
    // Formula mirrors XInputDriver.cpp exactly.
    int16_t lx = static_cast<int16_t>(state.lx) + INT16_MIN;
    int16_t ly = static_cast<int16_t>(~state.ly) + INT16_MIN;
    int16_t rx = static_cast<int16_t>(state.rx) + INT16_MIN;
    int16_t ry = static_cast<int16_t>(~state.ry) + INT16_MIN;

    report[5]  = (uint8_t)(lx & 0xFF);          report[6]  = (uint8_t)((uint16_t)lx >> 8);
    report[7]  = (uint8_t)(ly & 0xFF);          report[8]  = (uint8_t)((uint16_t)ly >> 8);
    report[9]  = (uint8_t)(rx & 0xFF);          report[10] = (uint8_t)((uint16_t)rx >> 8);
    report[11] = (uint8_t)(ry & 0xFF);          report[12] = (uint8_t)((uint16_t)ry >> 8);

    BLEHIDManager::getInstance().sendReport(report, sizeof(report));
#else
    (void)gamepad;
#endif
}
