#ifndef BLE_HID_MANAGER_H
#define BLE_HID_MANAGER_H

// IMPORTANT: Do NOT include tusb.h or any TinyUSB header in this file.
// hid_report_type_t is defined by both TinyUSB and BTstack — including both
// in the same translation unit causes a compile error.

#include <stdint.h>
#include <stdbool.h>

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

    // Queue a 9-byte HID input report for transmission via BLE ATT notification.
    // Returns false if not connected or notifications not enabled.
    bool sendReport(const uint8_t* report, uint16_t len);

    // Enable or disable BLE pairing (advertising with general discoverability).
    void setPairingMode(bool enabled);

    bool isConnected() const    { return _connected; }
    bool isEnabled() const      { return _initialized; }
    bool isNotifying() const    { return _notificationsEnabled; }
    bool hasBondedPeers() const { return _hasBondedPeers; }

private:
    BLEHIDManager() = default;

    void _doInit();
    void _ledBlink(uint32_t count, uint32_t onMs, uint32_t offMs);

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
    volatile uint16_t _conHandle            = 0xFFFF;
    volatile uint16_t _pendingReportLen     = 0;
    volatile uint8_t  _pendingBlinkType     = 0;  // 1 = report sent, 3 = disabled, 5 = enabled
    uint8_t  _pendingReport[9]     = {};
};

#endif // BLE_HID_MANAGER_H
