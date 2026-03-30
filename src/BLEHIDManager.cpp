/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 *
 * BLE HID (HID over GATT Profile) manager for GP2040-CE.
 *
 * TU ISOLATION: this file must NOT include tusb.h or any TinyUSB header.
 * The hid_report_type_t name collides between TinyUSB and BTstack.
 * Use bt_config_bridge.h for StorageManager access.
 */

#ifdef ENABLE_BLUETOOTH

#include "BLEHIDManager.h"
#include "bt_config_bridge.h"

#include "btstack.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_run_loop_async_context.h"
#include "pico/btstack_flash_bank.h"
#include "btstack_run_loop.h"
#include "btstack_tlv.h"
#include "platform/embedded/btstack_tlv_flash_bank.h"
#include "ble/le_device_db_tlv.h"
#include "ble/sm.h"
#include "ble/att_server.h"
#include "ble/gatt-service/hids_device.h"
#include "ble/gatt-service/battery_service_server.h"
#include "ble/gatt-service/device_information_service_server.h"
#include "pico/time.h"

#include <cstring>

// Compiled GATT database generated from src/ble_hid.gatt by pico_btstack_make_gatt_header
#include "ble_hid.h"

// HID report descriptor — matches BTHIDManager.cpp for consistency across BT modes.
// 32 buttons + 1 hat (4-bit + 4-bit padding) + 4 axes (8-bit each) = 9 bytes per report.
// Report ID 1 is required: the GATT Report Reference descriptor maps this report to ID=1.
// Without a Report ID tag in the descriptor, Windows cannot match the GATT report reference
// to a report in the HID descriptor and service setup fails.
static const uint8_t hid_descriptor_gamepad[] = {
    0x05, 0x01,        // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,        // USAGE (Gamepad)
    0xa1, 0x01,        // COLLECTION (Application)
    0x85, 0x01,        //   REPORT_ID (1)
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

// BLE advertising data
// Flags (0x01): LE General Discoverable + BR/EDR capable
// Complete 16-bit UUIDs (0x03): HID Service 0x1812
// Appearance (0x19): Gamepad 0x03C4
static const uint8_t adv_data[] = {
    0x02, 0x01, 0x06,                         // Flags: LE General Discoverable, BR/EDR capable
    0x03, 0x03, 0x12, 0x18,                   // Complete 16-bit UUIDs: HID (0x1812)
    0x03, 0x19, 0xC4, 0x03,                   // Appearance: Gamepad (0x03C4)
};

// Scan response: complete local name
// Length = 0x0A (10) = 1 type byte + 9 name bytes. No null terminator — AD type 0x09 is not C-string.
static const uint8_t scan_resp_data[] = {
    0x0A, 0x09, 'G','P','2','0','4','0','-','C','E',  // Complete Local Name: "GP2040-CE"
};

static void ble_packet_handler(uint8_t packet_type, uint16_t channel, uint8_t* packet, uint16_t size);

BLEHIDManager& BLEHIDManager::getInstance() {
    static BLEHIDManager instance;
    return instance;
}

// Blink the CYW43 onboard LED n times.
// Only call after cyw43_arch_init() has succeeded — the LED is CYW43-controlled on Pico W/2W.
static void blink_cyw43_led(int count, int on_ms, int off_ms) {
    for (int i = 0; i < count; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(on_ms);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(off_ms);
    }
}

void BLEHIDManager::init() {
    _bootTimeMs   = to_ms_since_boot(get_absolute_time());
    _initDelayMs  = 3000;
}

void BLEHIDManager::_doInit() {
    if (_initialized) return;

    if (cyw43_arch_init() != 0) {
        // CYW43 hardware not responding — no LED available yet; caller will retry.
        return;
    }

    // 2 fast blinks: CYW43 init succeeded, BTstack setup starting.
    blink_cyw43_led(2, 100, 100);

    // Hook BTstack into CYW43's async context so timers and keepalives fire.
    btstack_run_loop_init(btstack_run_loop_async_context_get_instance(cyw43_arch_async_context()));

    // Flash-backed TLV storage for BLE bonding keys (persists across power cycles).
    // IMPORTANT: pass &tlv_context (the btstack_tlv_flash_bank_t state object) as the second
    // argument to le_device_db_tlv_configure — NOT NULL. sm_init() calls le_device_db_init()
    // which calls get_tag(context, ...) immediately; passing NULL causes a NULL dereference
    // hard fault before the controller ever advertises.
    static btstack_tlv_flash_bank_t tlv_context;
    const btstack_tlv_t* tlv_impl = btstack_tlv_flash_bank_init_instance(
        &tlv_context, pico_flash_bank_instance(), NULL);
    le_device_db_tlv_configure(tlv_impl, &tlv_context);

    // ATT server — uses GATT database compiled from ble_hid.gatt.
    // hids_device, battery_service, and device_information_service handle all ATT reads/writes internally.
    att_server_init(profile_data, NULL, NULL);

    // HID over GATT Profile server.
    hids_device_init(0, hid_descriptor_gamepad, sizeof(hid_descriptor_gamepad));
    hids_device_register_packet_handler(ble_packet_handler);

    // Battery Service (100% placeholder; Phase 3 will read ADC).
    battery_service_server_set_battery_value(100);

    // Device Information Service.
    device_information_service_server_set_manufacturer_name("OpenStick Community");
    device_information_service_server_set_model_number("GP2040-CE");

    // Security Manager: no display/input, bonding enabled, no MITM.
    // SM_AUTHREQ_BONDING signals bonding intent to the host. BTstack sets hci_stack->bondable=1
    // by default in hci_init, so no explicit gap_set_bondable_mode() call is required.
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_BONDING);

    // Register SM and ATT event handlers.
    static btstack_packet_callback_registration_t sm_event_callback;
    sm_event_callback.callback = ble_packet_handler;
    sm_add_event_handler(&sm_event_callback);

    static btstack_packet_callback_registration_t hci_event_callback;
    hci_event_callback.callback = ble_packet_handler;
    hci_add_event_handler(&hci_event_callback);

    // 3 fast blinks: full stack configured, powering on HCI.
    // _startAdvertising() is called in the BTSTACK_EVENT_STATE handler once HCI_STATE_WORKING fires.
    blink_cyw43_led(3, 100, 100);

    hci_power_control(HCI_POWER_ON);
    _initialized = true;
}

void BLEHIDManager::_startAdvertising() {
    gap_advertisements_set_data(sizeof(adv_data), const_cast<uint8_t*>(adv_data));
    gap_scan_response_set_data(sizeof(scan_resp_data), const_cast<uint8_t*>(scan_resp_data));
    gap_advertisements_enable(1);
}

void BLEHIDManager::process() {
    if (!_initialized) {
        uint32_t elapsed = to_ms_since_boot(get_absolute_time()) - _bootTimeMs;
        if (elapsed > _initDelayMs) {
            _doInit();
            if (!_initialized) {
                // cyw43_arch_init() failed; reset timer to retry in 5 seconds.
                _bootTimeMs  = to_ms_since_boot(get_absolute_time());
                _initDelayMs = 5000;
            }
        }
    }

    if (!_initialized) return;

    cyw43_arch_poll();
}

bool BLEHIDManager::sendReport(const uint8_t* report, uint16_t len) {
    if (!_initialized || !_connected || !_notificationsEnabled) return false;
    if (len > BLE_HID_REPORT_SIZE) return false;

    memcpy(_pendingReport, report, len);
    _pendingReportLen = len;
    _reportPending = true;

    hids_device_request_can_send_now_event(_conHandle);
    return true;
}

void BLEHIDManager::setPairingMode(bool enabled) {
    _pairingMode = enabled;
    if (_initialized) {
        gap_advertisements_enable(enabled ? 1 : 0);
    }
}

bool BLEHIDManager::isConnected() const { return _connected; }
bool BLEHIDManager::isEnabled() const   { return _initialized; }

// ----- BLE packet handler -----
// Handles HCI events, SM pairing events, and HIDS events.
// ATT read/write callbacks are NOT needed — hids_device, battery_service, and device_information_service
// handle all GATT operations internally when initialized with #import <hids.gatt>.
static void ble_packet_handler(uint8_t packet_type, uint16_t channel,
                                uint8_t* packet, uint16_t size) {
    UNUSED(channel);
    UNUSED(size);

    BLEHIDManager& mgr = BLEHIDManager::getInstance();

    if (packet_type == HCI_EVENT_PACKET) {
        switch (hci_event_packet_get_type(packet)) {

            // HCI stack is fully up and ready — safe to start advertising now.
            case BTSTACK_EVENT_STATE:
                if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                    mgr._startAdvertising();
                }
                break;

            case HCI_EVENT_DISCONNECTION_COMPLETE:
                mgr._connected = false;
                mgr._notificationsEnabled = false;
                mgr._conHandle = HCI_CON_HANDLE_INVALID;
                // Restart advertising so a new host or the same host can reconnect.
                mgr._startAdvertising();
                break;

            case SM_EVENT_JUST_WORKS_REQUEST:
                // No display or input — confirm automatically.
                sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
                break;

            case SM_EVENT_PAIRING_COMPLETE: {
                uint8_t status = sm_event_pairing_complete_get_status(packet);
                if (status != ERROR_CODE_SUCCESS) {
                    // Pairing failed. BTstack will disconnect; restart advertising so a
                    // new connection attempt can be made. Blink fast 5x to signal failure.
                    blink_cyw43_led(5, 50, 50);
                    mgr._startAdvertising();
                }
                break;
            }

            case HCI_EVENT_LE_META:
                switch (hci_event_le_meta_get_subevent_code(packet)) {
                    case HCI_SUBEVENT_LE_CONNECTION_COMPLETE: {
                        uint8_t status = hci_subevent_le_connection_complete_get_status(packet);
                        if (status == ERROR_CODE_SUCCESS) {
                            mgr._conHandle = hci_subevent_le_connection_complete_get_connection_handle(packet);
                            mgr._connected = true;
                            gap_advertisements_enable(0);  // Stop advertising while connected
                        }
                        break;
                    }
                    default:
                        break;
                }
                break;

            case HCI_EVENT_HIDS_META:
                switch (hci_event_hids_meta_get_subevent_code(packet)) {
                    case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                        mgr._notificationsEnabled =
                            (hids_subevent_input_report_enable_get_enable(packet) != 0);
                        break;
                    case HIDS_SUBEVENT_CAN_SEND_NOW:
                        if (mgr._reportPending && mgr._notificationsEnabled) {
                            hids_device_send_input_report(
                                mgr._conHandle,
                                mgr._pendingReport,
                                mgr._pendingReportLen);
                            mgr._reportPending = false;
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
}

#endif // ENABLE_BLUETOOTH
