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

    // Map GamepadState to the 9-byte BLE HID report body (no Report ID byte — that is
    // carried by the GATT Report Reference descriptor):
    //   bytes 0-3  : 32 buttons (little-endian bitmask)
    //   byte  4    : hat (4 bits, 0-7 or 0xF=null) | padding (4 bits = 0)
    //   bytes 5-8  : x, y, z, rz axes (uint8, unsigned 0..255, center = 0x80)
    //                matches HIDDescriptors.h HIDReport and USB HID driver exactly
    uint8_t report[9] = {};
    const GamepadState& state = gamepad->state;

    // Buttons: GP2040-CE uses a 32-bit bitmask directly
    report[0] = (uint8_t)(state.buttons & 0xFF);
    report[1] = (uint8_t)((state.buttons >> 8)  & 0xFF);
    report[2] = (uint8_t)((state.buttons >> 16) & 0xFF);
    report[3] = (uint8_t)((state.buttons >> 24) & 0xFF);

    // Hat switch: map dpad bitmask to 0-7 (N/NE/E/SE/S/SW/W/NW) or 8 (null)
    uint8_t hat;
    switch (state.dpad & GAMEPAD_MASK_DPAD) {
        case GAMEPAD_MASK_UP:                          hat = HID_HAT_UP;        break;
        case GAMEPAD_MASK_UP    | GAMEPAD_MASK_RIGHT:  hat = HID_HAT_UPRIGHT;   break;
        case GAMEPAD_MASK_RIGHT:                       hat = HID_HAT_RIGHT;     break;
        case GAMEPAD_MASK_DOWN  | GAMEPAD_MASK_RIGHT:  hat = HID_HAT_DOWNRIGHT; break;
        case GAMEPAD_MASK_DOWN:                        hat = HID_HAT_DOWN;      break;
        case GAMEPAD_MASK_DOWN  | GAMEPAD_MASK_LEFT:   hat = HID_HAT_DOWNLEFT;  break;
        case GAMEPAD_MASK_LEFT:                        hat = HID_HAT_LEFT;      break;
        case GAMEPAD_MASK_UP    | GAMEPAD_MASK_LEFT:   hat = HID_HAT_UPLEFT;    break;
        default:                                       hat = HID_HAT_NOTHING;   break;
    }
    // BLE descriptor Logical Maximum (7) with Null State — clamp 8 to 0xF (null indicator)
    if (hat > 7) hat = 0x0F;
    report[4] = hat & 0x0F;  // lower 4 bits = hat, upper 4 bits = padding (0)

    // Axes: uint16 [0, 65535] → uint8 [0, 255] unsigned; midpoint 0x8000 → 0x80.
    // Matches USB HID driver (HIDDriver.cpp) and descriptor unsigned 0..255 range.
    report[5] = (uint8_t)(state.lx >> 8);
    report[6] = (uint8_t)(state.ly >> 8);
    report[7] = (uint8_t)(state.rx >> 8);
    report[8] = (uint8_t)(state.ry >> 8);

    BLEHIDManager::getInstance().sendReport(report, sizeof(report));
#else
    (void)gamepad;
#endif
}
