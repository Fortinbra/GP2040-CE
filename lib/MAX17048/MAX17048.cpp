/*!
 *  @file MAX17048.cpp
 *
 *  MAX17048 I2C Driver for GP2040-CE Battery Monitor
 *
 *  Ported from Adafruit MAX1704X Library:
 *  https://github.com/adafruit/Adafruit_MAX1704X
 *
 *  Original Author: Limor Fried (Adafruit Industries)
 *  GP2040-CE Port: GP2040-CE Contributors
 *
 *  BSD license (see license in Adafruit original)
 */

#include <cmath>
#include <algorithm>
#include "MAX17048.h"

MAX17048::MAX17048() : address(MAX17048_I2CADDR_DEFAULT), i2c(nullptr) {}

MAX17048::MAX17048(PeripheralI2C *i2cController, uint8_t addr) 
    : address(addr), i2c(i2cController) {}

MAX17048::~MAX17048() {}

/*!
 *    @brief  Sets up the hardware and initializes I2C
 *    @return True if initialization was successful, otherwise false.
 */
bool MAX17048::begin() {
    if (!i2c || !i2c->configured) {
        return false;
    }

    if (!isDeviceReady()) {
        return false;
    }

    if (!reset()) {
        return false;
    }

    enableSleep(false);
    sleep(false);

    return true;
}

/*!
 *    @brief  Get IC LSI version
 *    @return 16-bit value read from MAX17048_VERSION_REG register
 */
uint16_t MAX17048::getICversion() {
    return read16(MAX17048_VERSION_REG);
}

/*!
 *    @brief  Get semi-unique chip ID
 *    @return 8-bit value read from MAX17048_CHIPID_REG register
 */
uint8_t MAX17048::getChipID() {
    return read8(MAX17048_CHIPID_REG);
}

/*!
 *    @brief  Check if the MAX17048 is ready to be read from.
 *    Chip ID = 0xFF and Version = 0xFFFF if no battery is attached
 *    @return True if the MAX17048 is ready to be read from
 */
bool MAX17048::isDeviceReady() {
    return (getICversion() & 0xFFF0) == 0x0010;
}

/*!
 *    @brief  Soft reset the MAX17048
 *    @return True on reset success
 */
bool MAX17048::reset() {
    // Send reset command, the MAX17048 will reset before ACKing,
    // so I2C xfer is expected to *fail* with a NACK
    if (write16(MAX17048_CMD_REG, 0x5400)) {
        return false;
    }

    // Loop and attempt to clear alert until success
    for (uint8_t retries = 0; retries < 3; retries++) {
        if (clearAlertFlag(MAX17048_ALERTFLAG_RESET_INDICATOR)) {
            return true;
        }
    }

    return false;
}

/*!
 *    @brief  Function for clearing an alert flag once it has been handled.
 *    @param flags A byte that can have any number of OR'ed alert flags
 *    @return True if the status register write succeeded
 */
bool MAX17048::clearAlertFlag(uint8_t flags) {
    uint8_t status = read8(MAX17048_STATUS_REG);
    return write8(MAX17048_STATUS_REG, status & ~flags);
}

/*!
 *    @brief  Get battery voltage
 *    @return Floating point value read in Volts
 */
float MAX17048::cellVoltage() {
    if (!isDeviceReady())
        return NAN;
    
    uint16_t voltage = read16(MAX17048_VCELL_REG);
    return voltage * 78.125f / 1000000.0f;
}

/*!
 *    @brief  Get battery state in percent (0-100%)
 *    @return Floating point value from 0 to 100.0
 */
float MAX17048::cellPercent() {
    if (!isDeviceReady())
        return NAN;
    
    uint16_t percent = read16(MAX17048_SOC_REG);
    return percent / 256.0f;
}

/*!
 *    @brief  Charge or discharge rate of the battery in percent/hour
 *    @return Floating point value from 0 to 100.0% per hour
 */
float MAX17048::chargeRate() {
    if (!isDeviceReady())
        return NAN;
    
    int16_t percenthr = (int16_t)read16(MAX17048_CRATE_REG);
    return percenthr * 0.208f;
}

/*!
 *    @brief Setter function for the voltage that the IC considers 'resetting'
 *    @param reset_v Floating point voltage that, when we go below, should be
 * considered a reset
 */
void MAX17048::setResetVoltage(float reset_v) {
    reset_v /= 0.04f; // 40mV / LSB
    uint8_t reset_bits = std::max(0, std::min(127, (int)reset_v));
    
    uint8_t reg_val = read8(MAX17048_VRESET_REG);
    reg_val = (reg_val & 0x80) | (reset_bits & 0x7F);
    write8(MAX17048_VRESET_REG, reg_val);
}

/*!
 *    @brief Getter function for the voltage that the IC considers 'resetting'
 *    @returns Floating point voltage that, when we go below, should be
 * considered a reset
 */
float MAX17048::getResetVoltage() {
    uint8_t reg_val = read8(MAX17048_VRESET_REG);
    float val = reg_val & 0x7F;
    val *= 0.04f; // 40mV / LSB
    return val;
}

/*!
 *    @brief Setter function for the voltage alert min/max settings
 *    @param minv The minimum voltage: alert if we go below
 *    @param maxv The maximum voltage: alert if we go above
 */
void MAX17048::setAlertVoltages(float minv, float maxv) {
    uint8_t minv_int = std::max(0, std::min(255, (int)(minv / 0.02f)));
    uint8_t maxv_int = std::max(0, std::min(255, (int)(maxv / 0.02f)));
    
    write8(MAX17048_VALERT_REG, minv_int);
    write8(MAX17048_VALERT_REG + 1, maxv_int);
}

/*!
 *    @brief Getter function for the voltage alert min/max settings
 *    @param minv The minimum voltage: alert if we go below
 *    @param maxv The maximum voltage: alert if we go above
 */
void MAX17048::getAlertVoltages(float &minv, float &maxv) {
    minv = read8(MAX17048_VALERT_REG) * 0.02f; // 20mV / LSB
    maxv = read8(MAX17048_VALERT_REG + 1) * 0.02f; // 20mV / LSB
}

/*!
 *    @brief A check to determine if there is an unhandled alert
 *    @returns True if there is an alert status flag
 */
bool MAX17048::isActiveAlert() {
    uint16_t config = read16(MAX17048_CONFIG_REG);
    return (config & (1 << 5)) != 0; // Alert bit
}

/*!
 *    @brief Get all 7 alert flags from the status register in a uint8_t
 *    @returns A byte that has all 7 alert flags
 */
uint8_t MAX17048::getAlertStatus() {
    return read8(MAX17048_STATUS_REG) & 0x7F;
}

/*!
 *    @brief The voltage change that will trigger exiting hibernation mode.
 *    @returns The threshold, from 0-0.31874 V that will be used to determine
 *    whether its time to exit hibernation.
 */
float MAX17048::getActivityThreshold() {
    return (float)read8(MAX17048_HIBRT_REG + 1) * 0.00125f; // 1.25mV per LSB
}

/*!
 *    @brief Set the voltage change that will trigger exiting hibernation mode.
 *    @param actthresh The threshold voltage, from 0-0.31874 V that will be
 *    used to determine whether its time to exit hibernation.
 */
void MAX17048::setActivityThreshold(float actthresh) {
    uint8_t val = std::max(0, std::min(255, (int)(actthresh / 0.00125f))); // 1.25mV per LSB
    write8(MAX17048_HIBRT_REG + 1, val);
}

/*!
 *    @brief The %/hour change that will trigger hibernation mode.
 *    @returns The threshold, from 0-53% that will be used to determine
 *    whether its time to hibernate.
 */
float MAX17048::getHibernationThreshold() {
    return (float)read8(MAX17048_HIBRT_REG) * 0.208f; // 0.208% per hour
}

/*!
 *    @brief Determine the %/hour change that will trigger hibernation mode
 *    @param hibthresh The threshold, from 0-53% that will be used to determine
 *    whether its time to hibernate.
 */
void MAX17048::setHibernationThreshold(float hibthresh) {
    uint8_t val = std::max(0, std::min(255, (int)(hibthresh / 0.208f))); // 0.208% per hour
    write8(MAX17048_HIBRT_REG, val);
}

/*!
 *    @brief Query whether the chip is hibernating now
 *    @returns True if hibernating
 */
bool MAX17048::isHibernating() {
    uint8_t mode = read8(MAX17048_MODE_REG);
    return (mode & (1 << 4)) != 0; // Hibernation bit
}

/*!
 *    @brief Enter hibernation mode.
 */
void MAX17048::hibernate() {
    write8(MAX17048_HIBRT_REG + 1, 0xFF);
    write8(MAX17048_HIBRT_REG, 0xFF);
}

/*!
 *    @brief Wake up from hibernation mode.
 */
void MAX17048::wake() {
    write8(MAX17048_HIBRT_REG + 1, 0x0);
    write8(MAX17048_HIBRT_REG, 0x0);
}

/*!
 *    @brief Enter ultra-low-power sleep mode (1uA draw)
 *    @param s True to force-enter sleep mode, False to leave sleep
 */
void MAX17048::sleep(bool s) {
    uint16_t config = read16(MAX17048_CONFIG_REG);
    if (s) {
        config |= (1 << 7); // Set sleep bit
    } else {
        config &= ~(1 << 7); // Clear sleep bit
    }
    write16(MAX17048_CONFIG_REG, config);
}

/*!
 *    @brief Enable the ability to enter ultra-low-power sleep mode (1uA draw)
 *    @param en True to enable sleep mode, False to only allow hibernation
 */
void MAX17048::enableSleep(bool en) {
    uint8_t mode = read8(MAX17048_MODE_REG);
    if (en) {
        mode |= (1 << 5); // Set sleep enable bit
    } else {
        mode &= ~(1 << 5); // Clear sleep enable bit
    }
    write8(MAX17048_MODE_REG, mode);
}

/*!
 *    @brief  Quick starting allows an instant 'auto-calibration' of the
 * battery. However, its a bad idea to do this right when the battery is first
 * plugged in or if there's a lot of load on the battery so uncomment only if
 * you're sure you want to 'reset' the chips charge calculator.
 */
void MAX17048::quickStart() {
    uint8_t mode = read8(MAX17048_MODE_REG);
    mode |= (1 << 6); // Set quick start bit
    write8(MAX17048_MODE_REG, mode);
    // Bit is cleared immediately by the chip
}

/*!
 *    @brief Read 16-bit register value
 *    @param reg Register address
 *    @return 16-bit value in MSB first format
 */
uint16_t MAX17048::read16(uint8_t reg) {
    uint8_t data[2];
    if (i2c->readRegister(address, reg, data, 2) < 0) {
        return 0;
    }
    return (data[0] << 8) | data[1];
}

/*!
 *    @brief Read 8-bit register value
 *    @param reg Register address
 *    @return 8-bit value
 */
uint8_t MAX17048::read8(uint8_t reg) {
    uint8_t data;
    if (i2c->readRegister(address, reg, &data, 1) < 0) {
        return 0;
    }
    return data;
}

/*!
 *    @brief Write 16-bit register value
 *    @param reg Register address
 *    @param value 16-bit value in MSB first format
 *    @return True if write succeeded
 */
bool MAX17048::write16(uint8_t reg, uint16_t value) {
    uint8_t data[3] = {reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xFF)};
    return i2c->write(address, data, 3) >= 0;
}

/*!
 *    @brief Write 8-bit register value
 *    @param reg Register address
 *    @param value 8-bit value
 *    @return True if write succeeded
 */
bool MAX17048::write8(uint8_t reg, uint8_t value) {
    uint8_t data[2] = {reg, value};
    return i2c->write(address, data, 2) >= 0;
}
