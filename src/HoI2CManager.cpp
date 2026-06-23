#include "HoI2CManager.h"

#include <string.h>

void HoI2CManager::init() {
    if (_initialized) {
        return;
    }

    _initialized = true;
    _reportPending = false;
    _reportLength = 0;
    memset(_reportBuffer, 0, sizeof(_reportBuffer));
}

void HoI2CManager::process() {
    if (!_initialized) {
        return;
    }

    // Phase 1 scaffold: transport IRQ/register handling is wired in a later step.
}

bool HoI2CManager::sendReport(const uint8_t* report, uint16_t len) {
    if (!_initialized || report == nullptr || len == 0) {
        return false;
    }

    if (len > REPORT_BUFFER_SIZE) {
        len = REPORT_BUFFER_SIZE;
    }

    memset(_reportBuffer, 0, sizeof(_reportBuffer));
    memcpy(_reportBuffer, report, len);

    _reportLength = len;
    _reportPending = true;
    return true;
}