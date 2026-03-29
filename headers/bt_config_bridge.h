/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Save a bonded Bluetooth device address to persistent config.
void bt_config_save_bonded_addr(const uint8_t addr[6]);

// Read the bonded Bluetooth device address from config.
// Returns true and fills addr_out[6] if a non-zero address is stored, false otherwise.
bool bt_config_get_bonded_addr(uint8_t addr_out[6]);

#ifdef __cplusplus
}
#endif
