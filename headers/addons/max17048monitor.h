#ifndef _MAX17048Monitor_H
#define _MAX17048Monitor_H

#include "gpaddon.h"
#include "max17048_dev.h"
#include "peripheralmanager.h"

// MAX17048 Module Name
#define MAX17048MonitorName "MAX17048Monitor"

#ifndef MAX17048_MONITOR_ENABLED
#define MAX17048_MONITOR_ENABLED 0
#endif

#ifndef MAX17048_MONITOR_I2C_SDA_PIN
#define MAX17048_MONITOR_I2C_SDA_PIN -1
#endif

#ifndef MAX17048_MONITOR_I2C_SCL_PIN
#define MAX17048_MONITOR_I2C_SCL_PIN -1
#endif

#ifndef MAX17048_MONITOR_I2C_BLOCK
#define MAX17048_MONITOR_I2C_BLOCK i2c0
#endif

#ifndef MAX17048_MONITOR_I2C_SPEED
#define MAX17048_MONITOR_I2C_SPEED 400000
#endif

// Default monitoring interval in milliseconds
#ifndef MAX17048_MONITOR_INTERVAL_MS
#define MAX17048_MONITOR_INTERVAL_MS 5000
#endif

// Default alert voltage thresholds (in volts)
#ifndef MAX17048_MONITOR_ALERT_VOLTAGE_MIN
#define MAX17048_MONITOR_ALERT_VOLTAGE_MIN 3.2f
#endif

#ifndef MAX17048_MONITOR_ALERT_VOLTAGE_MAX
#define MAX17048_MONITOR_ALERT_VOLTAGE_MAX 4.2f
#endif

struct BatteryStatus {
    float voltage;
    float percentage;
    float chargeRate;
    bool isCharging;
    bool isLowBattery;
    bool isAlert;
    uint8_t alertFlags;
    bool deviceReady;
};

class MAX17048Monitor : public GPAddon {
public:
    virtual bool available();
    virtual void setup();
    virtual void process();
    virtual void preprocess() {}
    virtual void postprocess(bool) {}
    virtual void reinit() {}
    virtual std::string name() { return MAX17048MonitorName; }

    // Battery monitoring functions
    const BatteryStatus& getBatteryStatus() const { return batteryStatus; }
    bool isBatteryConnected() const { return max17048 && max17048->isDeviceReady(); }

    // Configuration functions
    void setMonitoringInterval(uint32_t intervalMs) { monitorIntervalMs = intervalMs; }
    void setAlertVoltages(float minV, float maxV);
    void enableHibernation(bool enable, float threshold = 5.0f);

private:
    MAX17048Device* max17048 = nullptr;
    BatteryStatus batteryStatus = {};
    
    uint32_t monitorIntervalMs = MAX17048_MONITOR_INTERVAL_MS;
    uint32_t nextUpdate = 0;
    
    bool hibernationEnabled = false;
    float hibernationThreshold = 5.0f;
    
    void updateBatteryStatus();
    void handleAlerts();
    void initializeDevice();
};

#endif
