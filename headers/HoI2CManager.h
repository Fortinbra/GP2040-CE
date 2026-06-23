#ifndef _HOI2C_MANAGER_H_
#define _HOI2C_MANAGER_H_

#include <stdint.h>

class HoI2CManager {
public:
    HoI2CManager(HoI2CManager const&) = delete;
    void operator=(HoI2CManager const&) = delete;

    static HoI2CManager& getInstance() {
        static HoI2CManager instance;
        return instance;
    }

    void init();
    void process();
    bool sendReport(const uint8_t* report, uint16_t len);

    bool isInitialized() const { return _initialized; }
    bool isReportPending() const { return _reportPending; }
    uint16_t getLastReportLength() const { return _reportLength; }

private:
    HoI2CManager() = default;

    static constexpr uint16_t REPORT_BUFFER_SIZE = 64;

    bool _initialized = false;
    bool _reportPending = false;
    uint16_t _reportLength = 0;
    uint8_t _reportBuffer[REPORT_BUFFER_SIZE] = {};
};

#endif