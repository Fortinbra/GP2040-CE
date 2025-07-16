# GP2040 Configuration for Raspberry Pi Pico W with Bluetooth

![Pin Mapping](assets/PinMapping.png)

Configuration for Raspberry Pi Pico W with integrated WiFi/Bluetooth chip (CYW43). This configuration includes Bluetooth GATT support for wireless connectivity.

## Features

- **Bluetooth HID Support**: Connect wirelessly to devices supporting Bluetooth HID
- **GATT Services**: Custom GATT profile for gamepad functionality
- **Battery Level Reporting**: Bluetooth battery level indication
- **Device Information**: Standard device information over Bluetooth
- **Standard GPIO Layout**: Same pin mapping as standard Pico for compatibility

## Bluetooth Configuration

When built with `PICO_BOARD=pico_w`, this configuration automatically includes:
- CYW43 wireless chip drivers
- BTStack Bluetooth Low Energy support
- Custom GATT database for gamepad services
- Bluetooth device pairing support

## Build Instructions

To build with Bluetooth support:

```bash
# Set environment variables
export PICO_BOARD=pico_w
export GP2040_BOARDCONFIG=PicoW

# Configure and build
cmake -B build
cmake --build build
```

Or using VS Code tasks with the pre-configured environment.

## Main Pin Mapping Configuration

| RP2040 Pin | Action                        | GP2040 | Xinput | Switch | PS3/4/5  | Dinput | Arcade |
|------------|-------------------------------|--------|--------|--------|----------|--------|--------|
| GPIO_PIN_02| GpioAction::BUTTON_PRESS_UP   | UP     | UP     | UP      | UP      | UP     | UP     |
| GPIO_PIN_03| GpioAction::BUTTON_PRESS_DOWN | DOWN   | DOWN   | DOWN    | DOWN    | DOWN   | DOWN   |
| GPIO_PIN_04| GpioAction::BUTTON_PRESS_RIGHT| RIGHT  | RIGHT  | RIGHT   | RIGHT   | RIGHT  | RIGHT  |
| GPIO_PIN_05| GpioAction::BUTTON_PRESS_LEFT | LEFT   | LEFT   | LEFT    | LEFT    | LEFT   | LEFT   |
| GPIO_PIN_06| GpioAction::BUTTON_PRESS_B1   | B1     | A      | B       | Cross   | 2      | K1     |
| GPIO_PIN_07| GpioAction::BUTTON_PRESS_B2   | B2     | B      | A       | Circle  | 3      | K2     |
| GPIO_PIN_08| GpioAction::BUTTON_PRESS_R2   | R2     | RT     | ZR      | R2      | 8      | K3     |
| GPIO_PIN_09| GpioAction::BUTTON_PRESS_L2   | L2     | LT     | ZL      | L2      | 7      | K4     |
| GPIO_PIN_10| GpioAction::BUTTON_PRESS_B3   | B3     | X      | Y       | Square  | 1      | P1     |
| GPIO_PIN_11| GpioAction::BUTTON_PRESS_B4   | B4     | Y      | X       | Triangle| 4      | P2     |
| GPIO_PIN_12| GpioAction::BUTTON_PRESS_R1   | R1     | RB     | R       | R1      | 6      | P3     |
| GPIO_PIN_13| GpioAction::BUTTON_PRESS_L1   | L1     | LB     | L       | L1      | 5      | P4     |
| GPIO_PIN_16| GpioAction::BUTTON_PRESS_S1   | S1     | Back   | Minus   | Select  | 9      | Coin   |
| GPIO_PIN_17| GpioAction::BUTTON_PRESS_S2   | S2     | Start  | Plus    | Start   | 10     | Start  |
| GPIO_PIN_18| GpioAction::BUTTON_PRESS_L3   | L3     | LS     | LS      | L3      | 11     | LS     |
| GPIO_PIN_19| GpioAction::BUTTON_PRESS_R3   | R3     | RS     | RS      | R3      | 12     | RS     |
| GPIO_PIN_20| GpioAction::BUTTON_PRESS_A1   | A1     | Guide  | Home    | PS      | 13     | ~      |
| GPIO_PIN_21| GpioAction::BUTTON_PRESS_A2   | A2     | ~      | Capture | ~       | 14     | ~      |

## Reserved Pins - DO NOT USE

**IMPORTANT**: The following GPIO pins are reserved for the CYW43 wireless chipset and MUST NOT be used for buttons or other GPIO functions:

| GPIO Pin | Function | Description |
|----------|----------|-------------|
| **GPIO 23** | WL_GPIO0 | CYW43 WiFi/Bluetooth communication |
| **GPIO 24** | WL_GPIO1 | CYW43 WiFi/Bluetooth communication |
| **GPIO 25** | WL_GPIO2 | CYW43 WiFi/Bluetooth communication |
| **GPIO 29** | VSYS_MONITOR | System voltage monitoring |

These pins are internally connected to the CYW43 wireless module and using them for other purposes will interfere with Bluetooth and WiFi functionality.