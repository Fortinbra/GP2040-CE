/*!
 *  @file MAX17048.h
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

#ifndef _MAX17048_H_
#define _MAX17048_H_

#include "peripheral_i2c.h"

// MAX17048 I2C Address
#define MAX17048_I2CADDR_DEFAULT 0x36

// MAX17048 Register Addresses
#define MAX17048_VCELL_REG 0x02   ///< Register that holds cell voltage
#define MAX17048_SOC_REG 0x04     ///< Register that holds cell state of charge
#define MAX17048_MODE_REG 0x06    ///< Register that manages mode
#define MAX17048_VERSION_REG 0x08 ///< Register that has IC version
#define MAX17048_HIBRT_REG 0x0A   ///< Register that manages hibernation
#define MAX17048_CONFIG_REG 0x0C  ///< Register that manages configuration
#define MAX17048_VALERT_REG 0x14  ///< Register that holds voltage alert values
#define MAX17048_CRATE_REG 0x16   ///< Register that holds cell charge rate
#define MAX17048_VRESET_REG 0x18  ///< Register that holds reset voltage setting
#define MAX17048_CHIPID_REG 0x19  ///< Register that holds semi-unique chip ID
#define MAX17048_STATUS_REG 0x1A  ///< Register that holds current alert/status
#define MAX17048_CMD_REG 0xFE     ///< Register that can be written for special commands

// Alert flags
#define MAX17048_ALERTFLAG_SOC_CHANGE 0x20      ///< Alert flag for state-of-charge change
#define MAX17048_ALERTFLAG_SOC_LOW 0x10         ///< Alert flag for state-of-charge low
#define MAX17048_ALERTFLAG_VOLTAGE_RESET 0x08   ///< Alert flag for voltage reset dip
#define MAX17048_ALERTFLAG_VOLTAGE_LOW 0x04     ///< Alert flag for cell voltage low
#define MAX17048_ALERTFLAG_VOLTAGE_HIGH 0x02    ///< Alert flag for cell voltage high
#define MAX17048_ALERTFLAG_RESET_INDICATOR 0x01 ///< Alert flag for IC reset notification

/*!
 *    @brief  Class that stores state and functions for interacting with
 *            the MAX17048 I2C battery monitor
 */
class MAX17048 {
public:
    MAX17048();
    MAX17048(PeripheralI2C *i2cController, uint8_t addr = MAX17048_I2CADDR_DEFAULT);
    ~MAX17048();

    bool begin();
    bool isDeviceReady();

    uint16_t getICversion();
    uint8_t getChipID();

    bool reset();
    bool clearAlertFlag(uint8_t flag);

    float cellVoltage();
    float cellPercent();
    float chargeRate();

    void setResetVoltage(float reset_v);
    float getResetVoltage();

    void setAlertVoltages(float minv, float maxv);
    void getAlertVoltages(float &minv, float &maxv);

    bool isActiveAlert();
    uint8_t getAlertStatus();

    void setActivityThreshold(float actthresh);
    float getActivityThreshold();
    void setHibernationThreshold(float hibthresh);
    float getHibernationThreshold();

    void hibernate();
    void wake();
    bool isHibernating();
    void sleep(bool s);
    void enableSleep(bool en);

    void quickStart();

    // Device management methods
    void setI2C(PeripheralI2C *i2cController) { this->i2c = i2cController; }
    void setAddress(uint8_t addr) { this->address = addr; }

protected:
    PeripheralI2C *i2c = nullptr;
    uint8_t address;

private:
    uint16_t read16(uint8_t reg);
    uint8_t read8(uint8_t reg);
    bool write16(uint8_t reg, uint16_t value);
    bool write8(uint8_t reg, uint8_t value);
};

#endif
