# MAX17048 Battery Monitor Addon

The MAX17048 Battery Monitor addon provides real-time battery monitoring capabilities for GP2040-CE using the MAX17048 fuel gauge IC. This addon monitors battery voltage, state of charge, and charge/discharge rates over I2C.

## Overview

The MAX17048 is a tiny, micropower fuel gauge for lithium-ion (Li+) batteries. This addon integrates the MAX17048 into GP2040-CE to provide:

- Real-time battery voltage monitoring
- State of charge (SoC) percentage
- Charge/discharge rate monitoring
- Battery alerts and warnings
- Power management features (hibernation, sleep)

## Hardware Requirements

- MAX17048 IC or breakout board
- I2C connection (SDA/SCL pins)
- Lithium-ion battery connected to the MAX17048

### Wiring

Connect the MAX17048 to your board via I2C:

- **VDD**: Connect to 3.3V (or 5V if your board supports it)
- **GND**: Connect to ground
- **SDA**: Connect to the configured I2C SDA pin
- **SCL**: Connect to the configured I2C SCL pin
- **BATT+**: Connect to the positive terminal of your Li-ion battery
- **BATT-**: Connect to the negative terminal of your Li-ion battery

**Note**: The MAX17048 has a fixed I2C address of 0x36 and cannot be changed.

## Configuration

The addon can be configured through the web interface or by modifying the board configuration files.

### Board Configuration

Add these defines to your board config file:

```cpp
// Enable the MAX17048 addon
#define MAX17048_MONITOR_ENABLED 1

// I2C Configuration (optional, uses board defaults if not specified)
#define MAX17048_MONITOR_I2C_SDA_PIN 2
#define MAX17048_MONITOR_I2C_SCL_PIN 3
#define MAX17048_MONITOR_I2C_BLOCK i2c0
#define MAX17048_MONITOR_I2C_SPEED 400000

// Monitoring Configuration (optional)
#define MAX17048_MONITOR_INTERVAL_MS 5000              // Monitor every 5 seconds
#define MAX17048_MONITOR_ALERT_VOLTAGE_MIN 3.2f        // Low voltage alert at 3.2V
#define MAX17048_MONITOR_ALERT_VOLTAGE_MAX 4.2f        // High voltage alert at 4.2V
```

### Web Configuration

When the addon is enabled, you can configure it through the GP2040-CE web interface:

- **Enabled**: Enable/disable the addon
- **Monitoring Interval**: How often to read battery status (in milliseconds)
- **Alert Voltage Min**: Minimum voltage threshold for alerts (in volts)
- **Alert Voltage Max**: Maximum voltage threshold for alerts (in volts)
- **Enable Hibernation**: Enable automatic hibernation mode for power saving
- **Hibernation Threshold**: Activity threshold for hibernation (in %/hour)

## Features

### Battery Monitoring

The addon provides comprehensive battery monitoring:

```cpp
// Get battery status
const BatteryStatus& status = max17048Monitor.getBatteryStatus();

// Available information:
status.voltage;        // Battery voltage in volts
status.percentage;     // State of charge (0-100%)
status.chargeRate;     // Charge/discharge rate in %/hour
status.isCharging;     // True if battery is charging
status.isLowBattery;   // True if battery is below 20%
status.isAlert;        // True if any alert is active
status.alertFlags;     // Bit flags for specific alerts
status.deviceReady;    // True if MAX17048 is connected and ready
```

### Alert System

The addon monitors for various battery conditions:

- **Voltage High**: Battery voltage exceeds maximum threshold
- **Voltage Low**: Battery voltage below minimum threshold
- **State of Charge Low**: Battery percentage is critically low
- **State of Charge Change**: Significant change in battery percentage
- **Voltage Reset**: Battery disconnection/reconnection detected
- **Reset Indicator**: IC reset occurred

### Power Management

The addon supports advanced power management features:

- **Hibernation Mode**: Automatically enters low-power mode when battery activity is low
- **Sleep Mode**: Ultra-low-power sleep mode (1µA current draw)
- **Activity Monitoring**: Tracks battery activity to optimize power consumption

## Integration Examples

### Board Config Integration

```cpp
// In your board configuration
#if defined(MAX17048_MONITOR_ENABLED) && MAX17048_MONITOR_ENABLED == 1
    // Battery monitoring is enabled
    // You can access battery status in your board-specific code
#endif
```

### Custom Code Integration

```cpp
#include "addons/max17048monitor.h"

// Access the addon instance
MAX17048Monitor* batteryMonitor = /* get from addon manager */;

// Check if battery is connected
if (batteryMonitor->isBatteryConnected()) {
    const BatteryStatus& status = batteryMonitor->getBatteryStatus();
    
    // Handle low battery condition
    if (status.isLowBattery) {
        // Implement low battery warning (LED, display, etc.)
    }
    
    // Handle charging state
    if (status.isCharging) {
        // Show charging indicator
    }
}
```

## Technical Details

### Implementation

The addon is built using the existing GP2040-CE patterns:

- **Device Library**: `lib/MAX17048/` - Contains the core MAX17048 driver
- **Device Interface**: `headers/interfaces/i2c/max17048/` - I2C device integration
- **Addon Implementation**: `src/addons/max17048monitor.cpp` - GP2040-CE addon wrapper
- **Configuration**: Integrated with GP2040-CE's protobuf configuration system

### I2C Communication

The addon uses the GP2040-CE PicoPeripherals I2C system:

- Automatically scans for MAX17048 at address 0x36
- Integrates with the peripheral manager for I2C resource sharing
- Follows established patterns for I2C device management

### Register Access

The implementation provides direct access to all MAX17048 registers:

- **VCELL** (0x02): Cell voltage
- **SOC** (0x04): State of charge
- **MODE** (0x06): Operating mode
- **VERSION** (0x08): IC version
- **HIBRT** (0x0A): Hibernation configuration
- **CONFIG** (0x0C): Device configuration
- **VALERT** (0x14): Voltage alert thresholds
- **CRATE** (0x16): Charge rate
- **VRESET** (0x18): Reset voltage threshold
- **STATUS** (0x1A): Alert status

## Troubleshooting

### Common Issues

1. **Device Not Found**
   - Check I2C wiring (SDA/SCL connections)
   - Verify power supply to MAX17048
   - Ensure battery is connected to the MAX17048

2. **Incorrect Readings**
   - Check battery connections
   - Allow time for fuel gauge calibration
   - Verify voltage reference configuration

3. **High Power Consumption**
   - Enable hibernation mode
   - Increase monitoring interval
   - Check for I2C bus issues

### Debug Information

Enable debugging by defining:
```cpp
#define DEBUG_PERIPHERALI2C
```

This will output I2C communication details to help diagnose connection issues.

## Porting from Adafruit Library

This implementation is ported from the Adafruit MAX1704X library, adapted for GP2040-CE's architecture:

- **Original Library**: https://github.com/adafruit/Adafruit_MAX1704X
- **Licensing**: BSD license (compatible with GP2040-CE)
- **Modifications**: Adapted for PicoPeripherals I2C system and GP2040-CE addon framework

## Future Enhancements

Potential future improvements:

- Display integration for battery status
- LED indicators for battery states
- Power management integration
- Custom alert thresholds per board
- Data logging capabilities
- Sleep mode integration with GP2040-CE power management
