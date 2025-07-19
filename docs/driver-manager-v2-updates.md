# Driver Manager V2 Updates - Transport Decoupling

## Overview

This update enhances the GP2040-CE driver architecture to support three main transport types (USB, Bluetooth, and GPIO) with protocol drivers determining their preferred transport methods.

## Key Changes

### 1. Transport Interface Updates

- Added `TransportType::GPIO` to support direct GPIO communication for retro console protocols
- Enhanced `ProtocolDriver` interface with transport preference methods:
  - `getPreferredTransports()`: Returns ordered list of preferred transports
  - `supportsTransport()`: Checks if protocol supports a specific transport

### 2. GPIO Transport Implementation

- Created `GPIOTransport` class for direct GPIO pin communication
- Supports pin configuration, debouncing, PWM, and state change callbacks
- Enables retro console protocols (SNES, Saturn, etc.) to work natively

### 3. Driver Manager Enhancements

- Protocol drivers now determine transport requirements during initialization
- `selectBestTransport()` method chooses optimal transport based on:
  - Protocol preferences
  - Transport availability
  - User preferences
- Improved transport switching with compatibility checks

### 4. Updated Workflow

**Old Approach:**
1. User specifies transport type
2. Create transport
3. Create protocol driver
4. Initialize protocol with transport

**New Approach:**
1. User specifies protocol (input mode)
2. Create protocol driver
3. Query protocol for transport preferences
4. Auto-select best available transport
5. Create and initialize transport
6. Initialize protocol with transport

## Example Usage

### XInput Protocol
- Prefers: USB, then Bluetooth
- Supports: USB, Bluetooth
- Does not support: GPIO, Serial

### SNES Protocol (Example)
- Prefers: GPIO only
- Supports: GPIO
- Does not support: USB, Bluetooth, Serial

### Future Protocol Examples
- PS3 Protocol: USB, Bluetooth
- Custom Serial Protocol: Serial, USB
- Arcade Protocol: GPIO

## Benefits

1. **Automatic Transport Selection**: Protocols specify what they need
2. **Better Hardware Utilization**: Uses most appropriate transport
3. **Cleaner Architecture**: Separation of concerns between protocol and transport
4. **Extensibility**: Easy to add new transport types
5. **Backward Compatibility**: Legacy drivers still work unchanged

## Implementation Notes

- Maintains full backward compatibility with existing legacy drivers
- New architecture is opt-in per protocol
- Transport availability is checked during auto-selection
- Failed transport initialization falls back gracefully
- Protocol switching can trigger transport changes when needed

## Files Modified

- `headers/interfaces/transportinterface.h`: Added GPIO transport type
- `headers/interfaces/protocoldriver.h`: Added transport preference methods
- `headers/interfaces/gpiotransport.h`: New GPIO transport interface
- `src/interfaces/gpiotransport.cpp`: GPIO transport implementation
- `headers/drivermanager_v2.h`: Updated setup method signature
- `src/drivermanager_v2.cpp`: Enhanced transport selection logic
- `headers/drivers/xinput/XInputProtocolDriver.h`: Added transport methods
- `src/drivers/xinput/XInputProtocolDriver.cpp`: Implemented transport preferences

## Usage Examples

```cpp
// Simple setup - auto-selects best transport for XInput
DriverManagerV2& dm = DriverManagerV2::getInstance();
dm.setup(INPUT_MODE_XINPUT); // Will prefer USB, fallback to Bluetooth

// With preferred transport
dm.setup(INPUT_MODE_XINPUT, TransportType::BLUETOOTH); // Prefer Bluetooth if supported

// Protocol switching with automatic transport adjustment
dm.switchProtocol(INPUT_MODE_SNES); // Will auto-switch to GPIO transport if needed
```
