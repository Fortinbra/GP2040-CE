# GP2040 Configuration for Raspberry Pi Pico 2 W with Bluetooth

Configuration for Raspberry Pi Pico 2 W with integrated WiFi/Bluetooth chip (CYW43). This configuration includes Bluetooth GATT support for wireless connectivity and takes advantage of the enhanced performance of the RP2350 processor.

## Features

- **Bluetooth HID Support**: Connect wirelessly to devices supporting Bluetooth HID
- **GATT Services**: Custom GATT profile for gamepad functionality
- **Battery Level Reporting**: Bluetooth battery level indication
- **Device Information**: Standard device information over Bluetooth
- **Standard GPIO Layout**: Same pin mapping as standard Pico for compatibility
- **Enhanced Performance**: RP2350 processor with improved capabilities

## Bluetooth Configuration

When built with `PICO_BOARD=pico2_w`, this configuration automatically includes:
- CYW43 wireless chip drivers
- BTStack Bluetooth Low Energy support
- Custom GATT database for gamepad services
- Bluetooth device pairing support

## Build Instructions

To build with Bluetooth support:

```bash
# Set environment variables (Windows)
set PICO_BOARD=pico2_w
set GP2040_BOARDCONFIG=Pico2W

# Or for Linux/Mac
export PICO_BOARD=pico2_w
export GP2040_BOARDCONFIG=Pico2W

# Configure and build
cmake -B build -S .
cmake --build build
```

## Pin Mapping

The Pico 2 W uses the same GPIO pin mapping as the standard Pico configurations:

| GP2040 | Xinput | Switch | PS3/4/5 | Dinput | Arcade | Pin |
|--------|--------|--------|---------|--------|--------|-----|
| UP     | UP     | UP     | UP      | UP     | UP     | 2   |
| DOWN   | DOWN   | DOWN   | DOWN    | DOWN   | DOWN   | 3   |
| RIGHT  | RIGHT  | RIGHT  | RIGHT   | RIGHT  | RIGHT  | 4   |
| LEFT   | LEFT   | LEFT   | LEFT    | LEFT   | LEFT   | 5   |
| B1     | A      | B      | Cross   | 2      | K1     | 6   |
| B2     | B      | A      | Circle  | 3      | K2     | 7   |
| R2     | RT     | ZR     | R2      | 8      | K3     | 8   |
| L2     | LT     | ZL     | L2      | 7      | K4     | 9   |
| B3     | X      | Y      | Square  | 1      | P1     | 10  |
| B4     | Y      | X      | Triangle| 4      | P2     | 11  |
| R1     | RB     | R      | R1      | 6      | P3     | 12  |
| L1     | LB     | L      | L1      | 5      | P4     | 13  |
| S1     | Back   | Minus  | Select  | 9      | Coin   | 16  |
| S2     | Start  | Plus   | Start   | 10     | Start  | 17  |
| L3     | LS     | LS     | L3      | 11     | LS     | 18  |
| R3     | RS     | RS     | R3      | 12     | RS     | 19  |
| A1     | Guide  | Home   | PS      | 13     | ~      | 20  |
| A2     | ~      | Capture| ~       | 14     | ~      | 21  |

## Reserved Pins

⚠️ **IMPORTANT**: The following pins are reserved for the CYW43 wireless chip and should NOT be used:
- GPIO 23 (WL_GPIO0)
- GPIO 24 (WL_GPIO1) 
- GPIO 25 (WL_GPIO2)
- GPIO 29 (VSYS_MONITOR)

Using these pins will interfere with WiFi and Bluetooth functionality.

## Bluetooth Pairing

1. Put your device (PC, phone, tablet) in Bluetooth pairing mode
2. Power on the GP2040-CE device
3. Look for "GP2040-CE Pico2 Gamepad" in available Bluetooth devices
4. Pair with the device (PIN: 0000 if required)
5. The gamepad should appear as a standard HID gamepad

## Additional Notes

- The Pico 2 W requires Pico SDK 2.1.1 or later for full RP2350 support
- Bluetooth and WiFi share the same CYW43 chip
- This configuration is optimized for gamepad usage over Bluetooth HID
- Battery level reporting works when powered by battery (VSYS monitoring)
