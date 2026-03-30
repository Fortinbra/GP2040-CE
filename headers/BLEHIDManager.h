/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#ifdef ENABLE_BLUETOOTH

#include <stdint.h>
#include <stdbool.h>

// Forward declaration for BTstack type (avoid including btstack.h in header to prevent TU/BTstack collision)
typedef uint16_t hci_con_handle_t;
#define HCI_CON_HANDLE_INVALID 0xFFFF

// Maximum BLE HID input report size (matches hid_descriptor_gamepad in BLEHIDManager.cpp)
#define BLE_HID_REPORT_SIZE 9

// Forward declaration for BTstack callback friend
static void ble_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size);

class BLEHIDManager {
public:
    static BLEHIDManager& getInstance();

    void init();
    void process();
    bool sendReport(const uint8_t* report, uint16_t len);
    void setPairingMode(bool enabled);
    bool isConnected() const;
    bool isEnabled() const;

private:
    BLEHIDManager() = default;

    void _doInit();
    void _startAdvertising();

    bool _initialized          = false;
    bool _connected            = false;
    bool _pairingMode          = false;
    bool _notificationsEnabled = false;

    hci_con_handle_t _conHandle  = HCI_CON_HANDLE_INVALID;
    uint32_t         _bootTimeMs = 0;
    uint32_t         _initDelayMs = 3000;  // initial 3-second boot delay; increases on retry

    // Pending report — written by sendReport(), sent on HIDS_SUBEVENT_CAN_SEND_NOW
    uint8_t  _pendingReport[BLE_HID_REPORT_SIZE];
    uint16_t _pendingReportLen = 0;
    bool     _reportPending = false;

    friend void ble_packet_handler(uint8_t, uint16_t, uint8_t*, uint16_t);
};

#endif // ENABLE_BLUETOOTH
