# Driver Manager V2 - Protocol/Transport Architecture

## Overview

The new driver architecture separates protocol logic from transport mechanisms, allowing the same gamepad protocols to work over different communication methods (USB, Bluetooth, GPIO, etc.). This eliminates the tight coupling to TinyUSB and enables support for BTStack, custom GPIO protocols, and other transport layers.

## Architecture Components

### 1. TransportInterface
Abstract base class for all transport mechanisms:
- **USBTransport**: Wraps TinyUSB functionality
- **BluetoothTransport**: For BTStack integration
- **GPIOTransport**: For retro console communication

### 2. ProtocolDriver
Abstract base class for gamepad protocols:
- **XInputProtocolDriver**: XInput protocol without TinyUSB dependencies
- **PS4ProtocolDriver**: PS4 DualShock protocol (future)
- **SNESProtocolDriver**: SNES controller protocol (example)

### 3. DriverManagerV2
Enhanced manager supporting both architectures:
- Legacy GPDriver support for backward compatibility
- New protocol + transport combinations
- Runtime switching between transports/protocols

## Usage Examples

### Basic Setup with USB Transport
```cpp
DriverManagerV2& manager = DriverManagerV2::getInstance();

// Setup XInput protocol over USB
bool success = manager.setup(INPUT_MODE_XINPUT, TransportType::USB);
if (success) {
    // Process gamepad input
    manager.process(&gamepad);
    manager.processAux();
    manager.processTransport();
}
```

### Setup with Bluetooth Transport
```cpp
DriverManagerV2& manager = DriverManagerV2::getInstance();

// Setup XInput protocol over Bluetooth
bool success = manager.setup(INPUT_MODE_XINPUT, TransportType::BLUETOOTH);
if (success) {
    // Configure Bluetooth-specific settings
    BluetoothTransport* bt = static_cast<BluetoothTransport*>(manager.getTransport());
    bt->startAdvertising("GP2040-CE Controller");
    
    // Process normally
    manager.process(&gamepad);
    manager.processAux();
    manager.processTransport();
}
```

### Runtime Transport Switching
```cpp
DriverManagerV2& manager = DriverManagerV2::getInstance();

// Start with USB
manager.setup(INPUT_MODE_XINPUT, TransportType::USB);

// Switch to Bluetooth while keeping XInput protocol
if (manager.switchTransport(TransportType::BLUETOOTH)) {
    // Now using XInput over Bluetooth
}
```

### Custom Protocol Example (SNES)
```cpp
// Add SNES support to the manager
// In drivermanager_v2.cpp, add to createProtocolDriver():
case INPUT_MODE_SNES:  // New enum value
    return std::make_unique<SNESProtocolDriver>();

// Usage:
DriverManagerV2& manager = DriverManagerV2::getInstance();
manager.setup(INPUT_MODE_SNES, TransportType::GPIO);

// Configure GPIO transport for SNES
GPIOTransport* gpio = static_cast<GPIOTransport*>(manager.getTransport());
GPIOTransport::GPIOConfig config = {
    .clock_pin = 2,
    .latch_pin = 3,
    .data_pin = 4,
    .pulse_width_us = 6,
    .setup_time_us = 6,
    .hold_time_us = 6
};
gpio->configure(config);
```

## Migration Guide

### For Existing Drivers

1. **Keep Legacy Support**: Existing drivers continue to work through the legacy path
2. **Gradual Migration**: Convert drivers one at a time to the new architecture
3. **Protocol Extraction**: Move protocol-specific logic to ProtocolDriver classes
4. **Transport Abstraction**: Replace direct TinyUSB calls with transport interface

### Implementation Steps

1. **Identify Protocol Logic**: Extract button mapping, report generation, etc.
2. **Create ProtocolDriver**: Implement the protocol without transport dependencies
3. **Update Manager**: Add new protocol to `createProtocolDriver()`
4. **Test Both Paths**: Ensure legacy and new architecture work correctly

## Benefits

### 1. Decoupling
- Protocol logic independent of transport mechanism
- Same protocol works over USB, Bluetooth, Serial, etc.
- Easier to add new transports or protocols

### 2. Bluetooth Support
- BTStack can be integrated without TinyUSB conflicts
- Multiple transport types can coexist

### 3. Retro Console Support
- Custom GPIO protocols for SNES, PlayStation, etc.
- Direct console communication without USB overhead

### 4. Testability
- Protocol logic can be unit tested independently
- Mock transports for testing

### 5. Flexibility
- Runtime switching between transports
- Protocol-specific optimizations
- Transport-specific features (MTU, flow control, etc.)

## Future Enhancements

### Additional Protocols
- PS4/PS5 DualShock protocols
- Nintendo Switch Pro Controller
- Fighting game stick protocols

### Additional Transports
- I2C for inter-device communication
- SPI for high-speed custom protocols
- Wireless (ESP-NOW, LoRa, etc.)

### Advanced Features
- Protocol negotiation
- Transport failover
- Multi-transport broadcasting
- Transport-specific authentication

## Backward Compatibility

The new architecture maintains full backward compatibility:
- Existing configurations continue to work
- Legacy GPDriver interface remains functional
- No breaking changes to user code
- Gradual migration path available
