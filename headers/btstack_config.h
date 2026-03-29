/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef BTSTACK_CONFIG_H
#define BTSTACK_CONFIG_H

// Port features
#define HAVE_EMBEDDED_TIME_MS

// BTstack buffer configuration for HID gamepad profile
#define HCI_ACL_PAYLOAD_SIZE 200
#define HCI_OUTGOING_PRE_BUFFER_SIZE 4
#define HCI_ACL_CHUNK_SIZE_ALIGNMENT 4
#define MAX_NR_HIDS_CONNECTIONS         2
#define MAX_NR_L2CAP_SERVICES           4
#define MAX_NR_L2CAP_CHANNELS           8
#define MAX_NR_RFCOMM_MULTIPLEXERS      0
#define MAX_NR_RFCOMM_SERVICES          0
#define MAX_NR_RFCOMM_CHANNELS          0
#define MAX_NR_BTSTACK_LINK_KEY_DB_MEMORY_ENTRIES  4
#define MAX_NR_HCI_CONNECTIONS          2
#define NVM_NUM_LINK_KEYS               4

// Enable BT Classic HID Device support
#define ENABLE_CLASSIC
#define ENABLE_HID_DEVICE
#define ENABLE_L2CAP_ENHANCED_RETRANSMISSION_MODE

// SDP support for service discovery
#define ENABLE_SDP_DES_DUMP

// Required for HCI transport
#define ENABLE_PRINTF_HEXDUMP

// Disable logging in production builds
#undef ENABLE_LOG_INFO
#undef ENABLE_LOG_DEBUG

#endif // BTSTACK_CONFIG_H
