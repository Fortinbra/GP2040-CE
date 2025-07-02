# Bluetooth Support for GP2040-CE

This guide explains how to use the new Bluetooth functionality in GP2040-CE, which allows your gamepad to connect wirelessly to compatible devices.

## Overview

GP2040-CE now supports Bluetooth gamepad connections on the Raspberry Pi Pico W, enabling wireless gameplay with the same button mappings and response times as USB connections.

## Requirements

- **Hardware**: Raspberry Pi Pico W (the wireless variant with CYW43 chip)
- **Firmware**: GP2040-CE with Bluetooth support enabled
- **Target Device**: Any device with Bluetooth gamepad support (PC, PlayStation, Nintendo Switch, etc.)

## Supported Bluetooth Modes

| Mode | Description | Compatible With |
|------|-------------|-----------------|
| Bluetooth PS4 | Emulates a DualShock 4 controller | PlayStation 4, PC, mobile devices |
| Bluetooth PS5 | Emulates a DualSense controller | PlayStation 5, PC, mobile devices |
| Bluetooth Switch | Emulates a Switch Pro Controller | Nintendo Switch, PC |
| Bluetooth Xbox | Emulates an Xbox controller | PC, mobile devices |

## Setup Instructions

### 1. Build and Flash the Bluetooth Firmware

```bash
# Configure for Pico W with Bluetooth
cmake -B build -DGPIO2040_BOARDCONFIG=PicoWBluetooth -DPICO_BOARD=pico_w

# Build the firmware
cmake --build build

# Flash the .uf2 file to your Pico W
```

### 2. Configure Input Mode

1. Connect to the web interface via USB initially
2. Go to **Settings** → **Input Mode**  
3. Select one of the Bluetooth modes:
   - **Bluetooth PS4** for PlayStation compatibility
   - **Bluetooth PS5** for latest PlayStation features
   - **Bluetooth Switch** for Nintendo Switch
   - **Bluetooth Xbox** for Xbox-style input
4. Save the configuration

### 3. Pairing Process

1. Restart your gamepad after changing to a Bluetooth mode
2. The gamepad will enter discoverable mode automatically
3. On your target device:
   - **PlayStation**: Go to Settings → Devices → Bluetooth Devices
   - **Nintendo Switch**: Go to System Settings → Controllers and Sensors → Change Grip/Order
   - **PC**: Go to Bluetooth settings and "Add device"
4. Look for "GP2040-CE Controller" in the device list
5. Select it to pair

### 4. Usage

- Once paired, the gamepad will automatically connect when powered on
- All button mappings remain the same as USB modes
- Battery indicator may show on some devices
- Typical range is 10-30 feet depending on environment

## Troubleshooting

### Connection Issues

**Problem**: Gamepad not discoverable
- **Solution**: Ensure you're using a Pico W (not regular Pico)
- **Solution**: Verify Bluetooth mode is selected in web config
- **Solution**: Try restarting the gamepad

**Problem**: Frequent disconnections
- **Solution**: Check power supply - low voltage can cause instability
- **Solution**: Reduce distance to target device
- **Solution**: Check for interference from other 2.4GHz devices

**Problem**: High input lag
- **Solution**: Ensure target device supports the selected controller type
- **Solution**: Try a different Bluetooth mode
- **Solution**: Check if target device has Bluetooth power saving enabled

### Compatibility Notes

- **PlayStation 4/5**: Use Bluetooth PS4 or PS5 modes respectively
- **Nintendo Switch**: Use Bluetooth Switch mode for best compatibility
- **PC Gaming**: Bluetooth PS4 mode often has the best support
- **Mobile Devices**: Try Bluetooth PS4 mode first, then PS5
- **Retro Gaming**: Bluetooth Xbox mode may work better with older emulators

## Technical Details

- **Latency**: Typically 3-8ms additional latency compared to USB
- **Range**: Up to 30 feet in open space, 10-15 feet through walls
- **Battery Life**: Approximately 8-12 hours depending on usage
- **Protocol**: Uses Bluetooth HID (Human Interface Device) profile
- **Frequency**: 2.4GHz ISM band

## Limitations

- Cannot use USB and Bluetooth simultaneously
- Requires Pico W hardware (not compatible with regular Pico)
- Some advanced features (like touchpad on PS4/PS5 modes) may not work on all devices
- Pairing process may need to be repeated if devices are reset

## Development Notes

This implementation uses the Pico SDK's BTstack integration and maintains the same driver architecture as USB modes. Each Bluetooth driver wraps the existing gamepad state processing with Bluetooth HID transport.

## Support

For issues with Bluetooth functionality:
1. Verify your hardware is Pico W
2. Check that Bluetooth is enabled in your target device
3. Try different Bluetooth input modes
4. Report bugs to the GP2040-CE project with logs and device details
