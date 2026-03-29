/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "OutputManager.h"
#include "drivermanager.h"
#include "tusb.h"

#ifdef ENABLE_BLUETOOTH
#include "BTHIDManager.h"
#include "drivers/hid/HIDDescriptors.h"
#endif

OutputManager& OutputManager::getInstance() {
    static OutputManager instance;
    return instance;
}

void OutputManager::init() {
    // BT init is deferred — CYW43 must not start before USB enumerates.
    // BTHIDManager::init() is called lazily in process() once tud_mounted() is true.
}

bool OutputManager::process(Gamepad* gamepad) {
    GPDriver* driver = DriverManager::getInstance().getDriver();
    bool processed = false;
    if (driver) {
        processed = driver->process(gamepad);
    }

#ifdef ENABLE_BLUETOOTH
    // Init BT once — BTHIDManager::_doInit() handles the USB/timeout gating internally.
    // Do NOT gate this on tud_mounted(): when on battery with no USB, tud_mounted() is
    // never true and the 3-second wireless-boot timeout would never start.
    if (!_btReady) {
        BTHIDManager::getInstance().init();
        _btReady = true;
    }

    if (_btReady) {
        HIDReport hidReport;
        
        switch (gamepad->state.dpad & GAMEPAD_MASK_DPAD) {
            case GAMEPAD_MASK_UP:                        hidReport.direction = HID_HAT_UP;        break;
            case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:   hidReport.direction = HID_HAT_UPRIGHT;   break;
            case GAMEPAD_MASK_RIGHT:                     hidReport.direction = HID_HAT_RIGHT;     break;
            case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT: hidReport.direction = HID_HAT_DOWNRIGHT; break;
            case GAMEPAD_MASK_DOWN:                      hidReport.direction = HID_HAT_DOWN;      break;
            case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:  hidReport.direction = HID_HAT_DOWNLEFT;  break;
            case GAMEPAD_MASK_LEFT:                      hidReport.direction = HID_HAT_LEFT;      break;
            case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:    hidReport.direction = HID_HAT_UPLEFT;    break;
            default:                                     hidReport.direction = HID_HAT_NOTHING;   break;
        }

        hidReport.l_x_axis = static_cast<uint8_t>(gamepad->state.lx >> 8);
        hidReport.l_y_axis = static_cast<uint8_t>(gamepad->state.ly >> 8);
        hidReport.r_x_axis = static_cast<uint8_t>(gamepad->state.rx >> 8);
        hidReport.r_y_axis = static_cast<uint8_t>(gamepad->state.ry >> 8);

        hidReport.buttons = 0
            | (gamepad->pressedB1()    ? GAMEPAD_MASK_B2     : 0)
            | (gamepad->pressedB2()    ? GAMEPAD_MASK_B3     : 0)
            | (gamepad->pressedB3()    ? GAMEPAD_MASK_B1     : 0)
            | (gamepad->pressedB4()    ? GAMEPAD_MASK_B4     : 0)
            | (gamepad->pressedL1()    ? GAMEPAD_MASK_L1     : 0)
            | (gamepad->pressedR1()    ? GAMEPAD_MASK_R1     : 0)
            | (gamepad->pressedL2()    ? GAMEPAD_MASK_L2     : 0)
            | (gamepad->pressedR2()    ? GAMEPAD_MASK_R2     : 0)
            | (gamepad->pressedS1()    ? GAMEPAD_MASK_S1     : 0)
            | (gamepad->pressedS2()    ? GAMEPAD_MASK_S2     : 0)
            | (gamepad->pressedL3()    ? GAMEPAD_MASK_L3     : 0)
            | (gamepad->pressedR3()    ? GAMEPAD_MASK_R3     : 0)
            | (gamepad->pressedA1()    ? GAMEPAD_MASK_A1     : 0)
            | (gamepad->pressedA2()    ? GAMEPAD_MASK_A2     : 0)
            | (gamepad->pressedA3()    ? GAMEPAD_MASK_A3     : 0)
            | (gamepad->pressedA4()    ? GAMEPAD_MASK_A4     : 0)
            | (gamepad->pressedUp()    ? GAMEPAD_MASK_DU     : 0)
            | (gamepad->pressedDown()  ? GAMEPAD_MASK_DD     : 0)
            | (gamepad->pressedLeft()  ? GAMEPAD_MASK_DL     : 0)
            | (gamepad->pressedRight() ? GAMEPAD_MASK_DR     : 0)
            | (gamepad->pressedE1()    ? GAMEPAD_MASK_E1     : 0)
            | (gamepad->pressedE2()    ? GAMEPAD_MASK_E2     : 0)
            | (gamepad->pressedE3()    ? GAMEPAD_MASK_E3     : 0)
            | (gamepad->pressedE4()    ? GAMEPAD_MASK_E4     : 0)
            | (gamepad->pressedE5()    ? GAMEPAD_MASK_E5     : 0)
            | (gamepad->pressedE6()    ? GAMEPAD_MASK_E6     : 0)
            | (gamepad->pressedE7()    ? GAMEPAD_MASK_E7     : 0)
            | (gamepad->pressedE8()    ? GAMEPAD_MASK_E8     : 0)
            | (gamepad->pressedE9()    ? GAMEPAD_MASK_E9     : 0)
            | (gamepad->pressedE10()   ? GAMEPAD_MASK_E10    : 0)
            | (gamepad->pressedE11()   ? GAMEPAD_MASK_E11    : 0)
            | (gamepad->pressedE12()   ? GAMEPAD_MASK_E12    : 0)
        ;

        if (gamepad->hasAnalogTriggers || gamepad->hasLeftAnalogStick) {
            if (gamepad->state.lt > 0)
                hidReport.buttons |= GAMEPAD_MASK_L2;
        }
        if (gamepad->hasAnalogTriggers || gamepad->hasRightAnalogStick) {
            if (gamepad->state.rt > 0)
                hidReport.buttons |= GAMEPAD_MASK_R2;
        }

        if (BTHIDManager::getInstance().isConnected()) {
            BTHIDManager::getInstance().sendReport(reinterpret_cast<const uint8_t*>(&hidReport), sizeof(HIDReport));
        }
    }

    if (_btReady) {
        BTHIDManager::getInstance().process();
    }
#endif

    return processed;
}

void OutputManager::processAux() {
    GPDriver* driver = DriverManager::getInstance().getDriver();
    if (driver) {
        driver->processAux();
    }
}
