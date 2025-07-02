/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "bluetooth/BluetoothPS4Driver.h"
#include "drivers/shared/driverhelper.h"
#include "storagemanager.h"

// Use the same HID descriptor as the USB PS4 driver
const uint8_t BluetoothPS4Driver::hid_descriptor[] = {
    // Same as PS4 USB HID descriptor - copy from PS4Descriptors.h
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x05,        // Usage (Game Pad)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x01,        //   Report ID (1)
    0x09, 0x30,        //   Usage (X)
    0x09, 0x31,        //   Usage (Y)
    0x09, 0x32,        //   Usage (Z)
    0x09, 0x35,        //   Usage (Rz)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x04,        //   Report Count (4)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    // ... rest of PS4 HID descriptor would go here
    0xC0,              // End Collection
};

const uint8_t* BluetoothPS4Driver::hid_descriptor_ptr = BluetoothPS4Driver::hid_descriptor;
uint16_t BluetoothPS4Driver::hid_descriptor_len = sizeof(BluetoothPS4Driver::hid_descriptor);

BluetoothPS4Driver::BluetoothPS4Driver() {
    memset(&ps4Report, 0, sizeof(PS4Report));
    memset(last_report, 0, sizeof(last_report));
}

void BluetoothPS4Driver::setup_hid_service() {
    // Setup Bluetooth HID service with PS4 descriptors
    hid_device_init_with_descriptor(
        false,  // boot device
        hid_descriptor_len,
        hid_descriptor_ptr,
        0,      // name (will be set later)
        0,      // service name
        0,      // service description
        0,      // provider name
        0,      // provider description
        0       // reconnect policy
    );
    
    // Register packet handler
    hci_event_callback_registration.callback = &BluetoothDriver::packet_handler;
    hci_add_event_handler(&hci_event_callback_registration);
}

bool BluetoothPS4Driver::process(Gamepad * gamepad) {
    if (!is_connected()) {
        return false;
    }

    const GamepadOptions & options = gamepad->getOptions();
    
    // Convert gamepad state to PS4 report format (same logic as USB PS4Driver)
    switch (gamepad->state.dpad & GAMEPAD_MASK_DPAD) {
        case GAMEPAD_MASK_UP:                        ps4Report.dpad = PS4_HAT_UP;        break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_RIGHT:   ps4Report.dpad = PS4_HAT_UPRIGHT;   break;
        case GAMEPAD_MASK_RIGHT:                     ps4Report.dpad = PS4_HAT_RIGHT;     break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_RIGHT: ps4Report.dpad = PS4_HAT_DOWNRIGHT; break;
        case GAMEPAD_MASK_DOWN:                      ps4Report.dpad = PS4_HAT_DOWN;      break;
        case GAMEPAD_MASK_DOWN | GAMEPAD_MASK_LEFT:  ps4Report.dpad = PS4_HAT_DOWNLEFT;  break;
        case GAMEPAD_MASK_LEFT:                      ps4Report.dpad = PS4_HAT_LEFT;      break;
        case GAMEPAD_MASK_UP | GAMEPAD_MASK_LEFT:    ps4Report.dpad = PS4_HAT_UPLEFT;    break;
        default:                                     ps4Report.dpad = PS4_HAT_NOTHING;   break;
    }

    bool anyA2A3A4 = gamepad->pressedA2() || gamepad->pressedA3() || gamepad->pressedA4();

    ps4Report.button_south    = gamepad->pressedB1();
    ps4Report.button_east     = gamepad->pressedB2();
    ps4Report.button_west     = gamepad->pressedB3();
    ps4Report.button_north    = gamepad->pressedB4();
    ps4Report.button_l1       = gamepad->pressedL1();
    ps4Report.button_r1       = gamepad->pressedR1();
    ps4Report.button_l2       = gamepad->pressedL2();
    ps4Report.button_r2       = gamepad->pressedR2();
    ps4Report.button_select   = options.switchTpShareForDs4 ? anyA2A3A4 : gamepad->pressedS1();
    ps4Report.button_start    = gamepad->pressedS2();
    ps4Report.button_l3       = gamepad->pressedL3();
    ps4Report.button_r3       = gamepad->pressedR3();
    ps4Report.button_home     = gamepad->pressedA1();
    ps4Report.button_touchpad = options.switchTpShareForDs4 ? gamepad->pressedS1() : anyA2A3A4;

    ps4Report.left_stick_x = static_cast<uint8_t>(gamepad->state.lx >> 8);
    ps4Report.left_stick_y = static_cast<uint8_t>(gamepad->state.ly >> 8);
    ps4Report.right_stick_x = static_cast<uint8_t>(gamepad->state.rx >> 8);
    ps4Report.right_stick_y = static_cast<uint8_t>(gamepad->state.ry >> 8);

    if (gamepad->hasAnalogTriggers) {
        ps4Report.left_trigger = gamepad->state.lt;
        ps4Report.right_trigger = gamepad->state.rt;
    } else {
        ps4Report.left_trigger = gamepad->pressedL2() ? 0xFF : 0;
        ps4Report.right_trigger = gamepad->pressedR2() ? 0xFF : 0;
    }

    // Send report if changed
    bool reportSent = false;
    if (memcmp(last_report, &ps4Report, sizeof(ps4Report)) != 0) {
        send_report((uint8_t*)&ps4Report, sizeof(ps4Report));
        memcpy(last_report, &ps4Report, sizeof(ps4Report));
        reportSent = true;
    }

    return reportSent;
}

void BluetoothPS4Driver::send_report(const uint8_t* report, uint16_t len) {
    if (is_connected()) {
        hid_device_send_interrupt_message(connection_handle, report, len);
    }
}
