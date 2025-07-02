/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _BLUETOOTH_PS4_DRIVER_H_
#define _BLUETOOTH_PS4_DRIVER_H_

#include "bluetooth/BluetoothDriver.h"
#include "drivers/ps4/PS4Descriptors.h"

class BluetoothPS4Driver : public BluetoothDriver {
public:
    BluetoothPS4Driver();
    virtual ~BluetoothPS4Driver() {}

    virtual bool process(Gamepad * gamepad) override;
    
protected:
    virtual void setup_hid_service() override;
    virtual void send_report(const uint8_t* report, uint16_t len) override;

private:
    PS4Report ps4Report;
    uint8_t last_report[sizeof(PS4Report)];
    
    // Bluetooth HID service
    static const uint8_t hid_descriptor[];
    static const uint8_t* hid_descriptor_ptr;
    static uint16_t hid_descriptor_len;
};

#endif
