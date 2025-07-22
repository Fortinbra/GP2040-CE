# MAX17048 Library for GP2040-CE

This library provides support for the MAX17048 fuel gauge IC, a tiny micropower current fuel gauge for lithium-ion (Li+) batteries.

## Features

- Battery voltage measurement
- State of charge (SoC) monitoring
- Charge/discharge rate tracking
- Configurable voltage alerts
- Power management (hibernation, sleep modes)
- Reset detection
- I2C communication

## Hardware

The MAX17048 is a single-cell fuel gauge that communicates over I2C at address 0x36. It requires minimal external components and can monitor a single lithium-ion battery.

### Pinout

- **VDD**: Power supply (2.5V to 5.5V)
- **GND**: Ground
- **SDA**: I2C data line
- **SCL**: I2C clock line
- **BATT+**: Battery positive terminal
- **BATT-**: Battery negative terminal

## Usage

```cpp
#include "MAX17048.h"
#include "peripheral_i2c.h"

// Initialize I2C
PeripheralI2C i2c;
i2c.setConfig(0, 2, 3, 400000); // I2C0, SDA=GPIO2, SCL=GPIO3, 400kHz

// Create MAX17048 instance
MAX17048 batteryMonitor(&i2c);

// Initialize device
if (batteryMonitor.begin()) {
    // Read battery status
    float voltage = batteryMonitor.cellVoltage();      // Volts
    float percentage = batteryMonitor.cellPercent();   // 0-100%
    float chargeRate = batteryMonitor.chargeRate();    // %/hour
    
    // Configure alerts
    batteryMonitor.setAlertVoltages(3.2f, 4.2f);      // Min/max alert voltages
    
    // Power management
    batteryMonitor.setHibernationThreshold(5.0f);     // Hibernation at 5%/hour
    batteryMonitor.enableSleep(true);                 // Enable sleep mode
}
```

## API Reference

### Basic Functions

- `bool begin()` - Initialize the device
- `bool isDeviceReady()` - Check if device is ready
- `float cellVoltage()` - Get battery voltage in volts
- `float cellPercent()` - Get state of charge (0-100%)
- `float chargeRate()` - Get charge/discharge rate in %/hour

### Configuration

- `void setAlertVoltages(float min, float max)` - Set voltage alert thresholds
- `void setResetVoltage(float voltage)` - Set reset detection voltage
- `void setHibernationThreshold(float threshold)` - Set hibernation threshold
- `void setActivityThreshold(float threshold)` - Set activity detection threshold

### Power Management

- `void hibernate()` - Enter hibernation mode
- `void wake()` - Exit hibernation mode
- `bool isHibernating()` - Check hibernation status
- `void sleep(bool enable)` - Control sleep mode
- `void enableSleep(bool enable)` - Enable/disable sleep capability

### Alerts

- `bool isActiveAlert()` - Check for active alerts
- `uint8_t getAlertStatus()` - Get alert status flags
- `bool clearAlertFlag(uint8_t flags)` - Clear specific alert flags

### Device Information

- `uint16_t getICversion()` - Get IC version
- `uint8_t getChipID()` - Get chip ID
- `bool reset()` - Software reset

## Alert Flags

- `MAX17048_ALERTFLAG_SOC_CHANGE` - State of charge changed
- `MAX17048_ALERTFLAG_SOC_LOW` - State of charge low
- `MAX17048_ALERTFLAG_VOLTAGE_RESET` - Voltage reset detected
- `MAX17048_ALERTFLAG_VOLTAGE_LOW` - Voltage below threshold
- `MAX17048_ALERTFLAG_VOLTAGE_HIGH` - Voltage above threshold
- `MAX17048_ALERTFLAG_RESET_INDICATOR` - IC reset occurred

## Integration with GP2040-CE

This library is designed to work with the GP2040-CE PicoPeripherals I2C system. It automatically integrates with the peripheral manager for resource sharing and device scanning.

## License

This library is ported from the Adafruit MAX1704X library and maintains the original BSD license. The port adapts the library for GP2040-CE's architecture while preserving all original functionality.

## Original Source

Based on the Adafruit MAX1704X library:
https://github.com/adafruit/Adafruit_MAX1704X

Original author: Limor Fried (Adafruit Industries)
