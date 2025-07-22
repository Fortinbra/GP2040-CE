#include "addons/max17048monitor.h"
#include "storagemanager.h"
#include "helper.h"
#include "config.pb.h"

bool MAX17048Monitor::available() {
    const MAX17048MonitorOptions& options = Storage::getInstance().getAddonOptions().max17048MonitorOptions;
    if (options.enabled) {
        max17048 = new MAX17048Device();
        PeripheralI2CScanResult result = PeripheralManager::getInstance().scanForI2CDevice(max17048->getDeviceAddresses());
        if (result.address > -1) {
            max17048->setAddress(result.address);
            max17048->setI2C(PeripheralManager::getInstance().getI2C(result.block));
            return true;
        } else {
            delete max17048;
            max17048 = nullptr;
        }
    }
    return false;
}

void MAX17048Monitor::setup() {
    const MAX17048MonitorOptions& options = Storage::getInstance().getAddonOptions().max17048MonitorOptions;
    
    // Initialize timing
    nextUpdate = getMillis();
    monitorIntervalMs = MAX17048_MONITOR_INTERVAL_MS;
    
    // Initialize battery status
    batteryStatus = {};
    
    // Initialize the device
    initializeDevice();
    
    // Apply configuration options if available
    if (options.monitoringIntervalMs > 0) {
        monitorIntervalMs = options.monitoringIntervalMs;
    }
    
    // Set up alert voltages
    float minVoltage = (options.alertVoltageMin > 0) ? options.alertVoltageMin : MAX17048_MONITOR_ALERT_VOLTAGE_MIN;
    float maxVoltage = (options.alertVoltageMax > 0) ? options.alertVoltageMax : MAX17048_MONITOR_ALERT_VOLTAGE_MAX;
    setAlertVoltages(minVoltage, maxVoltage);
    
    // Configure hibernation if enabled
    if (options.enableHibernation) {
        float threshold = (options.hibernationThreshold > 0) ? options.hibernationThreshold : 5.0f;
        enableHibernation(true, threshold);
    }
    
    // Perform initial battery status update
    updateBatteryStatus();
}

void MAX17048Monitor::process() {
    if (!max17048 || !max17048->isDeviceReady()) {
        return;
    }
    
    uint32_t currentTime = getMillis();
    if (currentTime >= nextUpdate) {
        updateBatteryStatus();
        handleAlerts();
        nextUpdate = currentTime + monitorIntervalMs;
    }
}

void MAX17048Monitor::setAlertVoltages(float minV, float maxV) {
    if (max17048 && max17048->isDeviceReady()) {
        max17048->setAlertVoltages(minV, maxV);
    }
}

void MAX17048Monitor::enableHibernation(bool enable, float threshold) {
    hibernationEnabled = enable;
    hibernationThreshold = threshold;
    
    if (max17048 && max17048->isDeviceReady()) {
        if (enable) {
            max17048->setHibernationThreshold(threshold);
            max17048->setActivityThreshold(0.15f); // Default activity threshold
        } else {
            max17048->wake(); // Ensure we're not hibernating
        }
    }
}

void MAX17048Monitor::initializeDevice() {
    if (!max17048) {
        return;
    }
    
    // Initialize the MAX17048 device
    if (!max17048->begin()) {
        batteryStatus.deviceReady = false;
        return;
    }
    
    batteryStatus.deviceReady = true;
    
    // Set default reset voltage (voltage below which battery is considered disconnected)
    max17048->setResetVoltage(3.0f);
    
    // Configure power management
    max17048->enableSleep(false);
    max17048->sleep(false);
    
    // Clear any existing alerts
    max17048->clearAlertFlag(0xFF);
}

void MAX17048Monitor::updateBatteryStatus() {
    if (!max17048) {
        batteryStatus.deviceReady = false;
        return;
    }
    
    if (!max17048->isDeviceReady()) {
        batteryStatus.deviceReady = false;
        batteryStatus.voltage = 0.0f;
        batteryStatus.percentage = 0.0f;
        batteryStatus.chargeRate = 0.0f;
        batteryStatus.isCharging = false;
        batteryStatus.isLowBattery = false;
        batteryStatus.isAlert = false;
        batteryStatus.alertFlags = 0;
        return;
    }
    
    batteryStatus.deviceReady = true;
    
    // Read basic battery metrics
    batteryStatus.voltage = max17048->cellVoltage();
    batteryStatus.percentage = max17048->cellPercent();
    batteryStatus.chargeRate = max17048->chargeRate();
    
    // Determine charging state (positive charge rate indicates charging)
    batteryStatus.isCharging = batteryStatus.chargeRate > 0.1f; // Small threshold to avoid noise
    
    // Determine low battery state (configurable threshold)
    batteryStatus.isLowBattery = batteryStatus.percentage < 20.0f; // 20% threshold
    
    // Check for alerts
    batteryStatus.isAlert = max17048->isActiveAlert();
    if (batteryStatus.isAlert) {
        batteryStatus.alertFlags = max17048->getAlertStatus();
    } else {
        batteryStatus.alertFlags = 0;
    }
}

void MAX17048Monitor::handleAlerts() {
    if (!batteryStatus.isAlert || !max17048) {
        return;
    }
    
    uint8_t alertFlags = batteryStatus.alertFlags;
    
    // Handle different types of alerts
    if (alertFlags & MAX17048_ALERTFLAG_VOLTAGE_LOW) {
        // Low voltage alert - could trigger low battery warning
        // Clear the alert after handling
        max17048->clearAlertFlag(MAX17048_ALERTFLAG_VOLTAGE_LOW);
    }
    
    if (alertFlags & MAX17048_ALERTFLAG_VOLTAGE_HIGH) {
        // High voltage alert - battery might be overcharged
        max17048->clearAlertFlag(MAX17048_ALERTFLAG_VOLTAGE_HIGH);
    }
    
    if (alertFlags & MAX17048_ALERTFLAG_SOC_LOW) {
        // State of charge low alert
        max17048->clearAlertFlag(MAX17048_ALERTFLAG_SOC_LOW);
    }
    
    if (alertFlags & MAX17048_ALERTFLAG_SOC_CHANGE) {
        // State of charge change alert
        max17048->clearAlertFlag(MAX17048_ALERTFLAG_SOC_CHANGE);
    }
    
    if (alertFlags & MAX17048_ALERTFLAG_VOLTAGE_RESET) {
        // Voltage reset alert - battery might have been disconnected
        max17048->clearAlertFlag(MAX17048_ALERTFLAG_VOLTAGE_RESET);
    }
    
    if (alertFlags & MAX17048_ALERTFLAG_RESET_INDICATOR) {
        // IC reset indicator
        max17048->clearAlertFlag(MAX17048_ALERTFLAG_RESET_INDICATOR);
    }
}
