# Pico-W Bluetooth Configuration

This configuration adds Bluetooth gamepad support to the GP2040-CE firmware for the Raspberry Pi Pico W. It allows your gamepad to connect wirelessly to devices that support Bluetooth gamepads.

## Features

- Full Bluetooth HID gamepad support
- Same button mappings as USB modes
- PS4, PS5, Switch, and Xbox controller emulation over Bluetooth
- Automatic pairing and reconnection
- Low latency wireless gameplay

## Supported Input Modes

- **INPUT_MODE_BT_PS4**: Bluetooth PS4 Controller
- **INPUT_MODE_BT_PS5**: Bluetooth PS5 Controller (DualSense)
- **INPUT_MODE_BT_SWITCH**: Bluetooth Nintendo Switch Pro Controller
- **INPUT_MODE_BT_XINPUT**: Bluetooth Xbox Controller

## Requirements

- Raspberry Pi Pico W (with wireless chip)
- Pico SDK 2.1.1 or later
- Device with Bluetooth support (PC, PlayStation, Nintendo Switch, etc.)

## Usage

1. Flash the firmware to your Pico W
2. Set the input mode to one of the Bluetooth modes via web config
3. Put your target device in pairing mode
4. The gamepad will automatically appear as a Bluetooth device
5. Pair and enjoy wireless gaming!

## Notes

- Bluetooth and USB modes are mutually exclusive
- Battery life will be reduced compared to USB operation
- First-time pairing may take a few seconds
- Range is typically 10-30 feet depending on environment
