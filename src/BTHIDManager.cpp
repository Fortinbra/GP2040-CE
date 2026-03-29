/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifdef ENABLE_BLUETOOTH

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_run_loop_async_context.h"
#include "classic/hid_device.h"
#include "bluetooth.h"
#include "pico/time.h"
#include "btstack_run_loop.h"

#include "BTHIDManager.h"
#include "bt_config_bridge.h"

#include <cstring>

extern "C" {
    bool tud_mounted(void);
}

static const uint8_t hid_descriptor_gamepad[] = {
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,        // USAGE (Gamepad)
    0xa1, 0x01,        // COLLECTION (Application)
    0x05, 0x09,        //   USAGE_PAGE (Button)
    0x19, 0x01,        //   USAGE_MINIMUM (Button 1)
    0x29, 0x20,        //   USAGE_MAXIMUM (Button 32)
    0x15, 0x00,        //   LOGICAL_MINIMUM (0)
    0x25, 0x01,        //   LOGICAL_MAXIMUM (1)
    0x95, 0x20,        //   REPORT_COUNT (32)
    0x75, 0x01,        //   REPORT_SIZE (1)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)
    0x05, 0x01,        //   USAGE_PAGE (Generic Desktop)
    0x09, 0x39,        //   USAGE (Hat switch)
    0x25, 0x07,        //   LOGICAL_MAXIMUM (7)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x75, 0x04,        //   REPORT_SIZE (4)
    0x81, 0x42,        //   INPUT (Data,Var,Abs,Null)
    0x95, 0x01,        //   REPORT_COUNT (1)
    0x75, 0x04,        //   REPORT_SIZE (4)
    0x81, 0x01,        //   INPUT (Cnst,Ary,Abs)
    0x05, 0x01,        //   USAGE_PAGE (Generic Desktop)
    0x26, 0xff, 0x00,  //   LOGICAL_MAXIMUM (255)
    0x46, 0xff, 0x00,  //   PHYSICAL_MAXIMUM (255)
    0x09, 0x30,        //   USAGE (X)
    0x09, 0x31,        //   USAGE (Y)
    0x09, 0x32,        //   USAGE (Z)
    0x09, 0x35,        //   USAGE (Rz)
    0x75, 0x08,        //   REPORT_SIZE (8)
    0x95, 0x04,        //   REPORT_COUNT (4)
    0x81, 0x02,        //   INPUT (Data,Var,Abs)
    0xc0               // END_COLLECTION
};

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size);

BTHIDManager& BTHIDManager::getInstance() {
    static BTHIDManager instance;
    return instance;
}

void BTHIDManager::init() {
    _bootTimeMs = to_ms_since_boot(get_absolute_time());
    _pendingInit = true;
}

void BTHIDManager::_doInit() {
    if (_initialized || _initFailed) {
        return;
    }

    if (cyw43_arch_init() != 0) {
        _initFailed = true;
        return;
    }

    // Hook BTstack into CYW43's poll async context so its timers fire
    btstack_run_loop_init(btstack_run_loop_async_context_get_instance(cyw43_arch_async_context()));

    l2cap_init();
    sdp_init();
    gap_set_local_name("GP2040-CE Controller");
    gap_discoverable_control(1);
    gap_set_class_of_device(0x002508);
    gap_ssp_set_io_capability(SSP_IO_CAPABILITY_NO_INPUT_NO_OUTPUT);

    hid_device_init(false, sizeof(hid_descriptor_gamepad), hid_descriptor_gamepad);
    hid_device_register_packet_handler(packet_handler);

    hid_sdp_record_t hid_params = {
        .hid_device_subclass = 0x2508,
        .hid_country_code = 0x00,
        .hid_virtual_cable = 0,
        .hid_remote_wake = 1,
        .hid_reconnect_initiate = 1,
        .hid_normally_connectable = 1,
        .hid_boot_device = 0,
        .hid_ssr_host_max_latency = 1600,
        .hid_ssr_host_min_timeout = 3200,
        .hid_supervision_timeout = 0x0c80,
        .hid_descriptor = hid_descriptor_gamepad,
        .hid_descriptor_size = sizeof(hid_descriptor_gamepad),
        .device_name = "GP2040-CE Controller"
    };

    // static: sdp_register_service() stores a pointer — must outlive init()
    static uint8_t hid_service_buffer[300];
    hid_create_sdp_record(hid_service_buffer, 0x10001, &hid_params);
    sdp_register_service(hid_service_buffer);

    hci_power_control(HCI_POWER_ON);

    // Reconnect to previously bonded device if one is stored;
    // otherwise stay discoverable so a new host can pair.
    bd_addr_t bondedAddr;
    if (bt_config_get_bonded_addr(bondedAddr)) {
        gap_discoverable_control(0);  // Not discoverable — we'll initiate reconnect
        _reconnectNeeded = true;
        _reconnectAfterMs = to_ms_since_boot(get_absolute_time()) + 1000;
    }

    _initialized = true;
}

void BTHIDManager::process() {
    if (_pendingInit && !_initialized && !_initFailed) {
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - _bootTimeMs;
        if (tud_mounted() || elapsed > 3000) {
            _doInit();
            _pendingInit = false;
        }
    }

    if (!_initialized) {
        return;
    }

    // Auto-reconnect after disconnect when a bonded device is stored
    if (_reconnectNeeded && !_connected) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (now >= _reconnectAfterMs) {
            _reconnectNeeded = false;
            bd_addr_t bondedAddr;
            if (bt_config_get_bonded_addr(bondedAddr)) {
                hid_device_connect(bondedAddr, &_hid_cid);
            }
        }
    }

    cyw43_arch_poll();
}

bool BTHIDManager::sendReport(const uint8_t* report, uint16_t len) {
    if (!_initialized || !_connected || _hid_cid == 0) {
        return false;
    }
    hid_device_send_interrupt_message(_hid_cid, report, len);
    return true;
}

void BTHIDManager::setPairingMode(bool enabled) {
    if (!_initialized) {
        return;
    }
    _pairingMode = enabled;
    gap_discoverable_control(enabled ? 1 : 0);
}

bool BTHIDManager::isConnected() const {
    return _connected;
}

bool BTHIDManager::isEnabled() const {
    return _initialized;
}

static void packet_handler(uint8_t packet_type, uint16_t channel, uint8_t *packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    BTHIDManager& mgr = BTHIDManager::getInstance();

    switch (packet_type) {
        case HCI_EVENT_PACKET:
            switch (hci_event_packet_get_type(packet)) {
                case HCI_EVENT_USER_CONFIRMATION_REQUEST: {
                    bd_addr_t addr;
                    reverse_bd_addr(&packet[2], addr);
                    gap_ssp_confirmation_response(addr);
                    break;
                }
                case HCI_EVENT_HID_META:
                    switch (hci_event_hid_meta_get_subevent_code(packet)) {
                        case HID_SUBEVENT_CONNECTION_OPENED:
                            if (hid_subevent_connection_opened_get_status(packet) == ERROR_CODE_SUCCESS) {
                                mgr._connected = true;
                                mgr._reconnectNeeded = false;
                                mgr._hid_cid = hid_subevent_connection_opened_get_hid_cid(packet);

                                // Stop advertising once connected — stays connectable but not discoverable
                                gap_discoverable_control(0);

                                // Persist bonded device address for automatic reconnect on next boot
                                bd_addr_t addr;
                                hid_subevent_connection_opened_get_bd_addr(packet, addr);
                                bt_config_save_bonded_addr(addr);
                            } else {
                                // Connection attempt failed — retry after 2 seconds
                                mgr._reconnectNeeded = true;
                                mgr._reconnectAfterMs = to_ms_since_boot(get_absolute_time()) + 2000;
                            }
                            break;
                        case HID_SUBEVENT_CONNECTION_CLOSED:
                            mgr._connected = false;
                            mgr._hid_cid = 0;
                            // Re-enable discoverability and schedule reconnect after 2 seconds
                            gap_discoverable_control(1);
                            mgr._reconnectNeeded = true;
                            mgr._reconnectAfterMs = to_ms_since_boot(get_absolute_time()) + 2000;
                            break;
                        default:
                            break;
                    }
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
}

#endif // ENABLE_BLUETOOTH
