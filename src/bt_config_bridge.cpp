/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifdef ENABLE_BLUETOOTH

#include "bt_config_bridge.h"
#include "storagemanager.h"

#include <cstring>

extern "C" {

void bt_config_save_bonded_addr(const uint8_t addr[6]) {
    BluetoothOptions& btOpts = Storage::getInstance().getAddonOptions().bluetoothOptions;
    memcpy(btOpts.bondedDeviceAddr.bytes, addr, 6);
    btOpts.bondedDeviceAddr.size = 6;
    btOpts.has_bondedDeviceAddr = true;
    Storage::getInstance().save();
}

bool bt_config_get_bonded_addr(uint8_t addr_out[6]) {
    BluetoothOptions& btOpts = Storage::getInstance().getAddonOptions().bluetoothOptions;
    if (!btOpts.has_bondedDeviceAddr || btOpts.bondedDeviceAddr.size != 6) {
        return false;
    }
    bool hasAddr = false;
    for (int i = 0; i < 6; i++) {
        if (btOpts.bondedDeviceAddr.bytes[i] != 0) {
            hasAddr = true;
            break;
        }
    }
    if (!hasAddr) {
        return false;
    }
    memcpy(addr_out, btOpts.bondedDeviceAddr.bytes, 6);
    return true;
}

} // extern "C"

#endif // ENABLE_BLUETOOTH
