/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once
#ifdef ENABLE_BLUETOOTH

#include <stdint.h>
#include <stdbool.h>

// Forward declaration for friend function
static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

class BTHIDManager {
public:
    static BTHIDManager& getInstance();

    void init();
    void process();
    bool sendReport(const uint8_t* report, uint16_t len);
    void setPairingMode(bool enabled);
    bool isConnected() const;
    bool isEnabled() const;

private:
    BTHIDManager() = default;
    bool _initialized = false;
    bool _pendingInit = false;
    bool _initFailed = false;
    bool _connected = false;
    bool _pairingMode = false;
    bool _reconnectNeeded = false;
    uint16_t _hid_cid = 0;
    uint32_t _bootTimeMs = 0;
    uint32_t _reconnectAfterMs = 0;

    void _doInit();

    friend void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);
};

#endif // ENABLE_BLUETOOTH
