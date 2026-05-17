// IMPORTANT: Do NOT include tusb.h or any TinyUSB header in this translation unit.
// hid_report_type_t is defined by both TinyUSB and BTstack — including both
// in the same translation unit causes a compile error.

#include "BLEHIDManager.h"

#include <string.h>

// Pico SDK CYW43 / BTstack headers
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "pico/cyw43_arch.h"
#include "pico/btstack_run_loop_async_context.h"
#include "hardware/adc.h"

// BTstack core
#include "btstack.h"

// Generated GATT database header (from src/ble_hid.gatt via pico_btstack_make_gatt_header)
#include "ble_hid.h"

// HID Report Descriptor: Report ID 1 + 11 named buttons (2 bytes) + hat+padding (1 byte)
//   + LT/RT triggers (2 bytes) + 4 signed 16-bit axes (8 bytes) = 13-byte report body.
// XInput-style layout: named face/shoulder/menu buttons, hat switch, uint8 triggers, int16 sticks.
// Report ID byte is prepended in HIDS_SUBEVENT_CAN_SEND_NOW before sending the notification.
static const uint8_t hid_report_descriptor[] = {
    0x05, 0x01,              // USAGE_PAGE (Generic Desktop)
    0x09, 0x05,              // USAGE (Game Pad)
    0xA1, 0x01,              // COLLECTION (Application)
    0x85, 0x01,              // REPORT_ID (1)

    // ── 11 digital buttons (2 bytes total) ─────────────────────────────────
    // Buttons 1–8: A, B, X, Y, LB, RB, Back, Start
    0x05, 0x09,              //   USAGE_PAGE (Button)
    0x19, 0x01,              //   USAGE_MINIMUM (Button 1)
    0x29, 0x08,              //   USAGE_MAXIMUM (Button 8)
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x25, 0x01,              //   LOGICAL_MAXIMUM (1)
    0x95, 0x08,              //   REPORT_COUNT (8)
    0x75, 0x01,              //   REPORT_SIZE (1)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    // Buttons 9–11: Guide, LS, RS
    0x19, 0x09,              //   USAGE_MINIMUM (Button 9)
    0x29, 0x0B,              //   USAGE_MAXIMUM (Button 11)
    0x95, 0x03,              //   REPORT_COUNT (3)
    0x75, 0x01,              //   REPORT_SIZE (1)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    // 5 padding bits to complete byte 1
    0x95, 0x05,              //   REPORT_COUNT (5)
    0x75, 0x01,              //   REPORT_SIZE (1)
    0x81, 0x03,              //   INPUT (Cnst,Var,Abs)

    // ── D-pad as hat switch (1 byte total) ─────────────────────────────────
    0x05, 0x01,              //   USAGE_PAGE (Generic Desktop)
    0x09, 0x39,              //   USAGE (Hat switch)
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x25, 0x07,              //   LOGICAL_MAXIMUM (7)
    0x35, 0x00,              //   PHYSICAL_MINIMUM (0)
    0x46, 0x3B, 0x01,        //   PHYSICAL_MAXIMUM (315 = 7×45 degrees)
    0x65, 0x14,              //   UNIT (Eng Rot: Angular Position)
    0x75, 0x04,              //   REPORT_SIZE (4)
    0x95, 0x01,              //   REPORT_COUNT (1)
    0x81, 0x42,              //   INPUT (Data,Var,Abs,Null)

    // 4 padding bits to complete the hat byte
    0x65, 0x00,              //   UNIT (None)
    0x75, 0x04,              //   REPORT_SIZE (4)
    0x95, 0x01,              //   REPORT_COUNT (1)
    0x81, 0x03,              //   INPUT (Cnst,Var,Abs)

    // ── Analog triggers: LT, RT (2 bytes total) ────────────────────────────
    0x05, 0x02,              //   USAGE_PAGE (Simulation Controls)
    0x09, 0xC5,              //   USAGE (Brake)       = LT
    0x09, 0xC4,              //   USAGE (Accelerator) = RT
    0x15, 0x00,              //   LOGICAL_MINIMUM (0)
    0x26, 0xFF, 0x00,        //   LOGICAL_MAXIMUM (255)
    0x75, 0x08,              //   REPORT_SIZE (8)
    0x95, 0x02,              //   REPORT_COUNT (2)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    // ── Analog sticks: LX, LY, RX, RY (8 bytes total) ─────────────────────
    // Signed 16-bit, little-endian; LOGICAL_MINIMUM(-32768) = 0x16 0x00 0x80
    0x05, 0x01,              //   USAGE_PAGE (Generic Desktop)
    0x09, 0x30,              //   USAGE (X)  = LX
    0x09, 0x31,              //   USAGE (Y)  = LY
    0x09, 0x32,              //   USAGE (Z)  = RX
    0x09, 0x35,              //   USAGE (Rz) = RY
    0x16, 0x00, 0x80,        //   LOGICAL_MINIMUM (-32768)
    0x26, 0xFF, 0x7F,        //   LOGICAL_MAXIMUM (32767)
    0x75, 0x10,              //   REPORT_SIZE (16)
    0x95, 0x04,              //   REPORT_COUNT (4)
    0x81, 0x02,              //   INPUT (Data,Var,Abs)

    0xC0,                    // END_COLLECTION
};

static_assert(sizeof(hid_report_descriptor) > 0, "HID descriptor must not be empty");

// BLE advertising data: Flags + Appearance (Gamepad, 964 = 0x03C4) + HID Service UUID
static const uint8_t adv_data[] = {
    2, BLUETOOTH_DATA_TYPE_FLAGS, 0x06,                  // LE General Discoverable, BR/EDR Not Supported
    3, BLUETOOTH_DATA_TYPE_APPEARANCE, 0xC4, 0x03,       // Gamepad (964 = 0x03C4, little-endian)
    7, BLUETOOTH_DATA_TYPE_COMPLETE_LIST_OF_16_BIT_SERVICE_CLASS_UUIDS,
        0x12, 0x18,   // HID Service (0x1812)
        0x0F, 0x18,   // Battery Service (0x180F)
        0x0A, 0x18,   // Device Information Service (0x180A)
};

// Scan response: complete local name
static const uint8_t scan_resp_data[] = {
    18, BLUETOOTH_DATA_TYPE_COMPLETE_LOCAL_NAME,
    'G','P','2','0','4','0','-','C','E',' ','G','a','m','e','p','a','d',
};

// Static TLV context removed — bonding database is now backed by protobuf config.

// BTstack event handler registrations
static btstack_packet_callback_registration_t hci_event_callback_registration;
static btstack_packet_callback_registration_t sm_event_callback_registration;

// ─────────────────────────────────────────────────────────────────────────────
// Public API
// ─────────────────────────────────────────────────────────────────────────────

void BLEHIDManager::init() {
    if (_initialized || _bootTimeMs != 0) return;
    _bootTimeMs = to_ms_since_boot(get_absolute_time());
}

void BLEHIDManager::process() {
    uint32_t now = to_ms_since_boot(get_absolute_time());

    if (!_initialized) {
        if (_bootTimeMs == 0) return;  // init() not called yet

        // Retry-delay after a failed cyw43_arch_init
        if (_initFailed) {
            if ((now - _retryTimeMs) < 5000) return;
            _initFailed = false;
        }

        // Deferred init: wait _initDelayMs ms after boot before touching the radio
        if ((now - _bootTimeMs) < _initDelayMs) return;

        _doInit();
        return;
    }

    cyw43_arch_poll();

    // Periodic battery level reporting — throttled to once per 30 seconds.
    // Uses direct ADC read on GPIO29 (ADC3) via 3:1 voltage divider.
    // Only fires when connected and notifications are enabled.
    {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if (_connected && _notificationsEnabled &&
                (now - _lastBatteryUpdateMs >= 30000 || _lastBatteryLevel == 255)) {
            uint8_t level = _readBatteryPercent();
            if (level != _lastBatteryLevel) {
                battery_service_server_set_battery_value(level);
                _lastBatteryLevel = level;
            }
            _lastBatteryUpdateMs = now;
        }
    }

    // Power state management — transition ACTIVE → IDLE after 30s of no input change.
    if (_powerState == BLEPowerState::ACTIVE) {
        if ((now - _lastInputChangeMs) >= 30000) {
            _powerState = BLEPowerState::IDLE;
            // Optional connection parameter request for longer interval in idle:
            // gap_request_connection_parameter_update(_conHandle, 80, 80, 0, 200);
        }
    }

    // Deferred advertising restart — set by disconnect handler, executed here in main loop
    if (_needsAdvRestart && !_connected) {
        gap_advertisements_enable(1);
        _advStarted = true;
        _needsAdvRestart = false;
    }

    // Non-blocking LED blink state machine — replaces blocking _ledBlink() calls.
    // _pendingBlinkType is set by the IRQ handler; we execute the blink here without sleep_ms().
    {
        static uint8_t         blinkRemaining = 0;
        static bool            blinkLedOn     = false;
        static uint32_t        blinkOnMs      = 200;
        static uint32_t        blinkOffMs     = 200;
        static absolute_time_t blinkNext      = {0};

        if (_pendingBlinkType != 0) {
            uint8_t blinkType = _pendingBlinkType;
            _pendingBlinkType = 0;
            if (blinkType == 5) {
                blinkOnMs = 200; blinkOffMs = 200; blinkRemaining = 10;
            } else if (blinkType == 3) {
                blinkOnMs = 300; blinkOffMs = 300; blinkRemaining = 6;
            } else if (blinkType == 1) {
                blinkOnMs = 50;  blinkOffMs = 50;  blinkRemaining = 2;
            }
            blinkLedOn = true;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
            blinkNext  = make_timeout_time_ms(blinkOnMs);
        }

        if (blinkRemaining > 0 &&
                absolute_time_diff_us(blinkNext, get_absolute_time()) >= 0) {
            blinkRemaining--;
            blinkLedOn = !blinkLedOn;
            if (blinkRemaining == 0) {
                // Sequence complete — always end with LED off
                blinkLedOn = false;
            }
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, blinkLedOn ? 1 : 0);
            if (blinkRemaining > 0) {
                blinkNext = make_timeout_time_ms(blinkLedOn ? blinkOnMs : blinkOffMs);
            }
        }
    }

    // Blink out the HCI disconnect reason code (N slow blinks = reason code value, capped at 15)
    // Common codes: 8=timeout, 0x13=remote terminated, 0x16=local terminated, 0x3B=bad params
    if (_lastDisconnectReason != 0 && _advStarted && !_connected) {
        static uint8_t diagBlinkCount = 0;
        static absolute_time_t diagBlinkNext = {0};
        static bool diagBlinkInit = false;
        if (!diagBlinkInit) {
            diagBlinkCount = (_lastDisconnectReason > 15) ? 15 : _lastDisconnectReason;
            diagBlinkNext = make_timeout_time_ms(1000); // 1s initial pause
            diagBlinkInit = true;
        }
        if (diagBlinkCount > 0 && absolute_time_diff_us(diagBlinkNext, get_absolute_time()) >= 0) {
            static bool diagLedOn = false;
            diagLedOn = !diagLedOn;
            cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, diagLedOn ? 1 : 0);
            diagBlinkNext = make_timeout_time_ms(diagLedOn ? 300 : 300);
            if (!diagLedOn) diagBlinkCount--;
            if (diagBlinkCount == 0) {
                _lastDisconnectReason = 0;
                diagBlinkInit = false;
            }
        }
    }

    if (_reportPending && _connected && _notificationsEnabled) {
        hids_device_request_can_send_now_event(_conHandle);
    }

    // Periodic UART status dump (every 5 s)
    {
        static uint32_t lastDumpMs = 0;
        if (now - lastDumpMs >= 5000) {
            lastDumpMs = now;
            printf("[BLE] status: connected=%d notif=%d power=%d bonds=%d handle=0x%04X\n",
                   (int)_connected, (int)_notificationsEnabled,
                   (int)_powerState, (int)_hasBondedPeers, (unsigned)_conHandle);
        }
    }
}

bool BLEHIDManager::sendReport(const uint8_t* report, uint16_t len) {
    if (!_connected || !_notificationsEnabled) {
        // Periodic "blocked" log — avoids flooding at frame rate
        static uint32_t lastBlockLogMs = 0;
        uint32_t now3 = to_ms_since_boot(get_absolute_time());
        if (now3 - lastBlockLogMs >= 2000) {
            lastBlockLogMs = now3;
            printf("[BLE] sendReport blocked: connected=%d notif=%d\n",
                   (int)_connected, (int)_notificationsEnabled);
        }
        return false;
    }
    // Periodic "flowing" log
    {
        static uint32_t lastSendMs = 0;
        uint32_t now4 = to_ms_since_boot(get_absolute_time());
        if (now4 - lastSendMs >= 2000) {
            lastSendMs = now4;
            printf("[BLE] sendReport queued len=%u\n", (unsigned)len);
        }
    }

    // In IDLE state, throttle report submission to ~50ms to reduce power consumption.
    if (_powerState == BLEPowerState::IDLE) {
        uint32_t now = to_ms_since_boot(get_absolute_time());
        if ((now - _lastReportMs) < 50) return false;
    }

    if (len > REPORT_SIZE_BYTES) len = REPORT_SIZE_BYTES;

    const uint8_t* previousReport = _lastSentReport;
    uint16_t previousLen = REPORT_SIZE_BYTES;
    if (_reportQueueCount > 0) {
        uint8_t lastIndex = (_reportQueueTail + REPORT_QUEUE_DEPTH - 1) % REPORT_QUEUE_DEPTH;
        previousReport = _reportQueue[lastIndex];
        previousLen = _reportQueueLen[lastIndex];
    }

    if (previousLen == len && memcmp(previousReport, report, len) == 0) {
        return true;
    }

    uint8_t writeIndex;
    if (_reportQueueCount < REPORT_QUEUE_DEPTH) {
        writeIndex = _reportQueueTail;
        _reportQueueTail = (_reportQueueTail + 1) % REPORT_QUEUE_DEPTH;
        _reportQueueCount++;
    } else {
        // Preserve the oldest unsent state and collapse only the newest queued state.
        writeIndex = (_reportQueueTail + REPORT_QUEUE_DEPTH - 1) % REPORT_QUEUE_DEPTH;
    }

    memset(_reportQueue[writeIndex], 0, REPORT_SIZE_BYTES);
    memcpy(_reportQueue[writeIndex], report, len);
    _reportQueueLen[writeIndex] = len;
    _reportPending    = (_reportQueueCount > 0);
    _lastReportMs     = to_ms_since_boot(get_absolute_time());
    return true;
}

void BLEHIDManager::setPairingMode(bool enabled) {
    _pairingMode = enabled;
    if (_initialized) {
        gap_advertisements_enable(enabled ? 1 : 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

void BLEHIDManager::_ledBlink(uint32_t count, uint32_t onMs, uint32_t offMs) {
    for (uint32_t i = 0; i < count; i++) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(onMs);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(offMs);
    }
}

// Read battery percentage from the onboard voltage divider on GPIO29/ADC3.
// Divider ratio is 3:1 → Vbat = Vadc * 3. LiPo range: 3.0 V (0%) to 4.2 V (100%).
// Raw ADC thresholds for a 3.3 V reference on a 12-bit (0–4095) converter:
//   3.0 V → Vadc = 1.0 V → raw ≈ 1241
//   4.2 V → Vadc = 1.4 V → raw ≈ 1737 (span = 496 counts)
// Falls back to 100 % on boards that do not define BATTERY_ADC_GPIO.
uint8_t BLEHIDManager::_readBatteryPercent() {
#ifdef BATTERY_ADC_GPIO
    adc_select_input(BATTERY_ADC_CHANNEL);
    uint16_t raw = adc_read();
    if (raw <= 1241) return 0;
    if (raw >= 1737) return 100;
    return (uint8_t)((raw - 1241) * 100 / 496);
#else
    return 100;
#endif
}

void BLEHIDManager::_doInit() {
    // Initialize CYW43 wireless chip
    int err = cyw43_arch_init();
    if (err != 0) {
        _initFailed  = true;
        _retryTimeMs = to_ms_since_boot(get_absolute_time());
        return;
    }

    // 2 fast blinks: CYW43 init succeeded
    _ledBlink(2, 100, 100);

    // Initialize BTstack run loop integrated with the CYW43 async context
    btstack_run_loop_init(btstack_run_loop_async_context_get_instance(cyw43_arch_async_context()));

    // Check for previously bonded peers — used to skip re-pairing on reconnect
    _hasBondedPeers = (le_device_db_count() > 0);

    // Bond-count diagnostic: blink count = number of stored bonds.
    // 0 bonds → 5 slow blinks (obvious "no stored bonds" indicator).
    // N bonds → N medium blinks (confirmed persistence).
    // Watch carefully at boot — this runs before the 3-blink HCI power-on sequence.
    {
        int bondCount = le_device_db_count();
        int blinkCount = (bondCount > 0) ? bondCount : 5;
        uint32_t onMs  = (bondCount > 0) ? 200 : 400;
        uint32_t offMs = (bondCount > 0) ? 200 : 400;
        _ledBlink(blinkCount, onMs, offMs);
    }

    // Core protocol layers — SM must be initialized before ATT/GATT services
    l2cap_init();
    sm_init();
    sm_set_io_capabilities(IO_CAPABILITY_NO_INPUT_NO_OUTPUT);
    sm_set_authentication_requirements(SM_AUTHREQ_SECURE_CONNECTION | SM_AUTHREQ_BONDING);

    // ATT server — profile_data is generated from ble_hid.gatt by pico_btstack_make_gatt_header.
    // NULL callbacks: hids_device registers its own service handler for all HIDS characteristics.
    att_server_init(profile_data, NULL, NULL);

    // HID over GATT device — boot mode 0 (no boot keyboard/mouse)
    hids_device_init(0, hid_report_descriptor, sizeof(hid_report_descriptor));

    // Battery ADC init — GPIO29/ADC3 is the onboard voltage divider on boards that define it.
    // Safe to call after cyw43_arch_init(); the CYW43 SPI and ADC reads coexist on GPIO29.
#ifdef BATTERY_ADC_GPIO
    adc_init();
    adc_gpio_init(BATTERY_ADC_GPIO);
#endif

    // Battery service — init() registers with ATT server; must be called before hci_power_control.
    battery_service_server_init(_readBatteryPercent());

    // Device Information Service
    device_information_service_server_set_manufacturer_name("OpenStickCommunity");
    device_information_service_server_set_model_number("GP2040-CE");
    device_information_service_server_set_firmware_revision("1.0");

    // Register event handlers
    hci_event_callback_registration.callback = &_hciPacketHandler;
    hci_add_event_handler(&hci_event_callback_registration);

    sm_event_callback_registration.callback = &_smPacketHandler;
    sm_add_event_handler(&sm_event_callback_registration);

    // HIDS meta events (HIDS_SUBEVENT_INPUT_REPORT_ENABLE, HIDS_SUBEVENT_CAN_SEND_NOW, etc.)
    // are delivered ONLY through hids_device_register_packet_handler — NOT via hci_add_event_handler.
    // Without this, the handler never fires and reports can never be sent.
    hids_device_register_packet_handler(&_hciPacketHandler);

    // Set up advertising parameters and data (adv_type = 0 = ADV_IND, undirected connectable)
    gap_advertisements_set_params(0x0030, 0x0060, 0, 0, NULL, 0x07, 0x00);
    gap_advertisements_set_data(sizeof(adv_data), (uint8_t*)adv_data);
    gap_scan_response_set_data(sizeof(scan_resp_data), (uint8_t*)scan_resp_data);

    // 3 fast blinks before powering on HCI
    _ledBlink(3, 80, 80);

    // Power on the Bluetooth controller — advertising starts ONLY after
    // BTSTACK_EVENT_STATE / HCI_STATE_WORKING fires (see _hciPacketHandler)
    hci_power_control(HCI_POWER_ON);

    _initialized = true;
}

// ─────────────────────────────────────────────────────────────────────────────
// HCI / HIDS event handler
// ─────────────────────────────────────────────────────────────────────────────

void BLEHIDManager::_hciPacketHandler(uint8_t packetType, uint16_t channel,
                                      uint8_t* packet, uint16_t size) {
    (void)channel;
    (void)size;

    BLEHIDManager& mgr = getInstance();

    if (packetType != HCI_EVENT_PACKET) return;

    uint8_t eventCode = hci_event_packet_get_type(packet);

    switch (eventCode) {
        case BTSTACK_EVENT_STATE:
            if (btstack_event_state_get_state(packet) == HCI_STATE_WORKING) {
                // Advertising starts ONLY here — not before hci_power_control returns
                if (!mgr._advStarted) {
                    gap_advertisements_enable(1);
                    mgr._advStarted = true;
                }
            }
            break;

        case HCI_EVENT_DISCONNECTION_COMPLETE: {
            uint8_t reason = hci_event_disconnection_complete_get_reason(packet);
            printf("[BLE] Disconnected reason=0x%02X\n", (unsigned)reason);
            mgr._lastDisconnectReason = reason;
            mgr._reportPending        = false;
            mgr._reportQueueHead      = 0;
            mgr._reportQueueTail      = 0;
            mgr._reportQueueCount     = 0;
            mgr._connected            = false;
            mgr._notificationsEnabled = false;
            mgr._conHandle            = HCI_CON_HANDLE_INVALID;
            mgr._advStarted           = false;
            mgr._powerState           = BLEPowerState::ADVERTISING;
            mgr._lastInputChangeMs    = 0;
            // Defer advertising restart to process() to avoid race with LL cleanup
            mgr._needsAdvRestart = true;
            break;
        }

        case HCI_EVENT_LE_META:
            if (hci_event_le_meta_get_subevent_code(packet) ==
                    HCI_SUBEVENT_LE_CONNECTION_COMPLETE) {
                mgr._lastDisconnectReason = 0; // clear on new connection
                mgr._conHandle  = hci_subevent_le_connection_complete_get_connection_handle(packet);
                mgr._connected  = true;
                mgr._advStarted = false;
                printf("[BLE] Connected handle=0x%04X\n", (unsigned)mgr._conHandle);
            }
            break;

        case HCI_EVENT_HIDS_META:
            switch (hci_event_hids_meta_get_subevent_code(packet)) {
                case HIDS_SUBEVENT_INPUT_REPORT_ENABLE:
                    {
                        uint8_t enable = hids_subevent_input_report_enable_get_enable(packet);
                        mgr._notificationsEnabled = (enable != 0);
                        printf("[BLE] INPUT_REPORT_ENABLE enable=%u\n", (unsigned)enable);
                        // Set flag to blink LED in process() — don't block IRQ handler with sleep_ms
                        mgr._pendingBlinkType = (enable != 0) ? 5 : 3;
                        if (enable != 0) {
                            mgr._powerState        = BLEPowerState::ACTIVE;
                            mgr._lastInputChangeMs = to_ms_since_boot(get_absolute_time());
                        }
                    }
                    break;
                case HIDS_SUBEVENT_CAN_SEND_NOW:
                    if (mgr._reportPending && mgr._reportQueueCount > 0) {
                        uint8_t readIndex = mgr._reportQueueHead;
                        uint16_t reportLen = mgr._reportQueueLen[readIndex];

                        hids_device_send_input_report(mgr._conHandle,
                                                      mgr._reportQueue[readIndex],
                                                      reportLen);
                        // Detect input change for IDLE → ACTIVE transition.
                        // Periodic log: first send + every 2 s
                        {
                            static uint32_t lastSendLogMs = 0;
                            static bool firstSend = true;
                            uint32_t t = to_ms_since_boot(get_absolute_time());
                            if (firstSend || t - lastSendLogMs >= 2000) {
                                firstSend    = false;
                                lastSendLogMs = t;
                                printf("[BLE] CAN_SEND_NOW: sent %u bytes"
                                       " btns=[0x%02X 0x%02X] hat=0x%02X\n",
                                        (unsigned)reportLen,
                                       mgr._reportQueue[readIndex][0], mgr._reportQueue[readIndex][1],
                                       mgr._reportQueue[readIndex][2]);
                            }
                        }
                        // _lastSentReport is only accessed here (IRQ context) so no volatile needed.
                        if (memcmp(mgr._lastSentReport, mgr._reportQueue[readIndex], reportLen) != 0) {
                            memset(mgr._lastSentReport, 0, BLEHIDManager::REPORT_SIZE_BYTES);
                            memcpy(mgr._lastSentReport, mgr._reportQueue[readIndex], reportLen);
                            mgr._lastInputChangeMs = to_ms_since_boot(get_absolute_time());
                            if (mgr._powerState == BLEPowerState::IDLE) {
                                mgr._powerState = BLEPowerState::ACTIVE;
                                // Optional: request shorter connection interval for gaming latency:
                                // gap_request_connection_parameter_update(mgr._conHandle, 6, 6, 0, 200);
                            }
                        }
                        mgr._reportQueueHead = (mgr._reportQueueHead + 1) % BLEHIDManager::REPORT_QUEUE_DEPTH;
                        mgr._reportQueueCount--;
                        mgr._reportPending = (mgr._reportQueueCount > 0);
                        // Don't blink per-report — reports fire at HID frame rate,
                        // continuously restarting the blink sequence keeps the LED solid.
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

// ─────────────────────────────────────────────────────────────────────────────
// SM (Security Manager) event handler
// ─────────────────────────────────────────────────────────────────────────────

void BLEHIDManager::_smPacketHandler(uint8_t packetType, uint16_t channel,
                                     uint8_t* packet, uint16_t size) {
    (void)channel;
    (void)size;

    if (packetType != HCI_EVENT_PACKET) return;

    BLEHIDManager& mgr = getInstance();

    switch (hci_event_packet_get_type(packet)) {
        case SM_EVENT_JUST_WORKS_REQUEST:
            sm_just_works_confirm(sm_event_just_works_request_get_handle(packet));
            break;
        case SM_EVENT_IDENTITY_RESOLVING_SUCCEEDED:
            // BTstack recognized a reconnecting bonded peer via IRK resolution.
            // Do NOT set _notificationsEnabled here — the LTK encryption handshake
            // has not completed yet. Arming notifications before the link is encrypted
            // causes an ATT security mode error, which disconnects the peer and creates
            // a reconnect loop. _notificationsEnabled is set in HCI_EVENT_ENCRYPTION_CHANGE.
            mgr._hasBondedPeers = true;
            break;
        case SM_EVENT_IDENTITY_RESOLVING_FAILED: {
            // Cannot resolve the peer's address using stored IRKs — likely a stale bond
            // on the host side. Clear all stored bonds so fresh pairing can succeed.
            // CRITICAL: Do NOT call sm_request_pairing() here — sending an unsolicited
            // pairing request to a host that believes it already has a valid bond will
            // cause the host to DELETE its stored bond (Android behavior observed).
            // Let the host either initiate re-pairing or allow the connection to proceed
            // (some hosts use non-resolvable addresses that still work without IRK match).
            int deviceCount = le_device_db_count();
            for (int i = deviceCount - 1; i >= 0; i--) {
                le_device_db_remove(i);
            }
            mgr._hasBondedPeers = false;
            break;
        }
        case SM_EVENT_PAIRING_COMPLETE: {
            uint8_t status = sm_event_pairing_complete_get_status(packet);
            if (status == ERROR_CODE_SUCCESS) {
                mgr._hasBondedPeers = true;
                // Re-verify bond was stored — 2 fast blinks = pairing+bond confirmed
                mgr._pendingBlinkType = 1;  // use the report-sent blink as "success" indicator
            }
            // On failure: do nothing. Bond was not stored. Advertising will restart
            // on disconnect and the host can try again.
            break;
        }
        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ATT read callback
// ─────────────────────────────────────────────────────────────────────────────

uint16_t BLEHIDManager::_attReadCallback(uint16_t conHandle, uint16_t attHandle,
                                          uint16_t offset, uint8_t* buffer,
                                          uint16_t bufferSize) {
    (void)conHandle;
    return att_read_callback_handle_blob(
        (const uint8_t*)hid_report_descriptor,
        (uint16_t)sizeof(hid_report_descriptor),
        offset, buffer, bufferSize);
}

// ─────────────────────────────────────────────────────────────────────────────
// ATT write callback
// ─────────────────────────────────────────────────────────────────────────────

int BLEHIDManager::_attWriteCallback(uint16_t conHandle, uint16_t attHandle,
                                      uint16_t transactionMode, uint16_t offset,
                                      uint8_t* buffer, uint16_t bufferSize) {
    (void)conHandle;
    (void)attHandle;
    (void)transactionMode;
    (void)offset;
    (void)buffer;
    (void)bufferSize;
    return 0;
}
