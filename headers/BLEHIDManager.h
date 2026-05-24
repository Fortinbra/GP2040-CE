#ifndef BLE_HID_MANAGER_H
#define BLE_HID_MANAGER_H

// IMPORTANT: Do NOT include tusb.h or any TinyUSB header in this file.
// hid_report_type_t is defined by both TinyUSB and BTstack — including both
// in the same translation unit causes a compile error.

#include <stdint.h>
#include <stdbool.h>

// Power management states for BLE HID operation.
// Transitions:
//   ADVERTISING → ACTIVE  : HIDS_SUBEVENT_INPUT_REPORT_ENABLE (notifications enabled)
//   ACTIVE      → IDLE    : no input change for 30 seconds
//   IDLE        → ACTIVE  : any input change detected in CAN_SEND_NOW
//   ACTIVE/IDLE → ADVERTISING : HCI_EVENT_DISCONNECTION_COMPLETE
enum class BLEPowerState : uint8_t {
    ADVERTISING = 0,
    ACTIVE      = 1,
    IDLE        = 2,
};

class BLEHIDManager {
public:
    static BLEHIDManager& getInstance() {
        static BLEHIDManager instance;
        return instance;
    }

    BLEHIDManager(const BLEHIDManager&) = delete;
    BLEHIDManager& operator=(const BLEHIDManager&) = delete;

    // Called once at startup to record boot time. Does NOT call cyw43_arch_init.
    void init();

    // Called every main loop iteration.
    // Handles 3s deferred hardware init and drives cyw43_arch_poll.
    void process();

    // Queue a 13-byte XInput-style HID input report for transmission via BLE ATT notification.
    // Returns false if not connected or notifications not enabled.
    bool sendReport(const uint8_t* report, uint16_t len);

    // Enable or disable BLE pairing (advertising with general discoverability).
    void setPairingMode(bool enabled);

    bool isConnected() const    { return _connected; }
    bool isEnabled() const      { return _initialized; }
    bool isNotifying() const    { return _notificationsEnabled; }
    bool hasBondedPeers() const { return _hasBondedPeers; }
    BLEPowerState getPowerState() const { return _powerState; }
    uint8_t getBatteryLevel() const { return _lastBatteryLevel; }

private:
    static constexpr uint8_t REPORT_SIZE_BYTES = 13;
    static constexpr uint8_t REPORT_QUEUE_DEPTH = 4;

    BLEHIDManager() = default;

    void _doInit();
    void _ledBlink(uint32_t count, uint32_t onMs, uint32_t offMs);
    static uint8_t _readBatteryPercent();

    static void _hciPacketHandler(uint8_t packetType, uint16_t channel,
                                  uint8_t* packet, uint16_t size);
    static void _smPacketHandler(uint8_t packetType, uint16_t channel,
                                 uint8_t* packet, uint16_t size);
    static uint16_t _attReadCallback(uint16_t conHandle, uint16_t attHandle,
                                     uint16_t offset, uint8_t* buffer, uint16_t bufferSize);
    static int _attWriteCallback(uint16_t conHandle, uint16_t attHandle,
                                 uint16_t transactionMode, uint16_t offset,
                                 uint8_t* buffer, uint16_t bufferSize);

    bool     _initialized          = false;
    bool     _advStarted           = false;
    bool     _pairingMode          = false;
    bool     _initFailed           = false;
    uint32_t _bootTimeMs           = 0;
    uint32_t _initDelayMs          = 3000;
    uint32_t _retryTimeMs          = 0;

    // Written by BTstack IRQ context (async_context_threadsafe_background) and
    // read by the main thread — must be volatile so the compiler does not cache
    // them in registers across loop iterations.
    volatile bool     _connected            = false;
    volatile bool     _notificationsEnabled = false;
    volatile bool     _reportPending        = false;
    volatile bool     _hasBondedPeers       = false;
    volatile bool     _needsAdvRestart      = false;
    volatile uint16_t _conHandle            = 0xFFFF;
    volatile uint8_t  _pendingBlinkType     = 0;  // 1 = report sent, 3 = disabled, 5 = enabled
    volatile uint8_t  _lastDisconnectReason = 0;  // HCI disconnect reason code; cleared on new connection
    volatile uint32_t _lastBatteryUpdateMs  = 0;
    volatile uint8_t  _lastBatteryLevel     = 255;  // 255 = uninitialized → forces first update
    volatile BLEPowerState _powerState      = BLEPowerState::ADVERTISING;
    volatile uint32_t _lastInputChangeMs    = 0;    // updated when report payload changes
    volatile uint32_t _lastReportMs         = 0;    // updated each time a report is queued
    volatile uint8_t  _reportQueueHead      = 0;
    volatile uint8_t  _reportQueueTail      = 0;
    volatile uint8_t  _reportQueueCount     = 0;
    uint16_t _reportQueueLen[REPORT_QUEUE_DEPTH] = {};
    uint8_t  _reportQueue[REPORT_QUEUE_DEPTH][REPORT_SIZE_BYTES] = {};
    uint8_t  _lastSentReport[REPORT_SIZE_BYTES]   = {};  // previous payload; compared in CAN_SEND_NOW to detect changes
};

#endif // BLE_HID_MANAGER_H
