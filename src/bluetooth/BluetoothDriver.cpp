/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "bluetooth/BluetoothDriver.h"
#include "storagemanager.h"

BluetoothDriver* BluetoothDriver::instance = nullptr;

void BluetoothDriver::initialize() {
    instance = this;
    
    // Initialize BTstack
    l2cap_init();
    sdp_init();
    hid_device_init(false, 0, 0, 0, 0, 0, 0, 0, 0);
    
    // Setup HID service specific to the driver
    setup_hid_service();
    
    // Turn on Bluetooth
    hci_power_control(HCI_POWER_ON);
}

void BluetoothDriver::on_bluetooth_connected(hci_con_handle_t handle) {
    connected = true;
    connection_handle = handle;
}

void BluetoothDriver::on_bluetooth_disconnected() {
    connected = false;
    connection_handle = HCI_CON_HANDLE_INVALID;
}

void BluetoothDriver::packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    if (!instance) return;
    
    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case HCI_EVENT_CONNECTION_COMPLETE:
                    if (hci_event_connection_complete_get_status(packet) == 0) {
                        instance->on_bluetooth_connected(hci_event_connection_complete_get_connection_handle(packet));
                    }
                    break;
                    
                case HCI_EVENT_DISCONNECTION_COMPLETE:
                    instance->on_bluetooth_disconnected();
                    break;
                    
                default:
                    break;
            }
            break;
            
        default:
            break;
    }
}
