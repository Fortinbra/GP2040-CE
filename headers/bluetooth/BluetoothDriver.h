/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _BLUETOOTH_DRIVER_H_
#define _BLUETOOTH_DRIVER_H_

#include "gpdriver.h"
#include "gamepad.h"
#include "btstack.h"

// Base class for Bluetooth gamepad drivers
class BluetoothDriver : public GPDriver {
public:
    BluetoothDriver() : connected(false), connection_handle(HCI_CON_HANDLE_INVALID) {}
    virtual ~BluetoothDriver() {}

    // Common Bluetooth functionality
    virtual void initialize() override;
    virtual void initializeAux() override {}
    virtual bool process(Gamepad * gamepad) override = 0;
    virtual void processAux() override {}
    
    // USB-specific methods (not used for Bluetooth)
    virtual uint16_t get_report(uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen) override { return 0; }
    virtual void set_report(uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize) override {}
    virtual bool vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const *request) override { return false; }
    virtual const uint16_t * get_descriptor_string_cb(uint8_t index, uint16_t langid) override { return nullptr; }
    virtual const uint8_t * get_descriptor_device_cb() override { return nullptr; }
    virtual const uint8_t * get_hid_descriptor_report_cb(uint8_t itf) override { return nullptr; }
    virtual const uint8_t * get_descriptor_configuration_cb(uint8_t index) override { return nullptr; }
    virtual const uint8_t * get_descriptor_device_qualifier_cb() override { return nullptr; }
    virtual uint16_t GetJoystickMidValue() override { return GAMEPAD_JOYSTICK_MID; }
    virtual USBListener * get_usb_auth_listener() override { return nullptr; }

    // Bluetooth-specific methods
    virtual void on_bluetooth_connected(hci_con_handle_t handle);
    virtual void on_bluetooth_disconnected();
    virtual bool is_connected() const { return connected; }

protected:
    bool connected;
    hci_con_handle_t connection_handle;
    
    // Bluetooth HID service setup
    virtual void setup_hid_service() = 0;
    virtual void send_report(const uint8_t* report, uint16_t len) = 0;
    
    // Common packet handling
    static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
    static BluetoothDriver* instance; // For static callback
};

#endif
