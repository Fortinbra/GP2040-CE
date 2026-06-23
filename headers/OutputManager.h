#ifndef OUTPUT_MANAGER_H
#define OUTPUT_MANAGER_H

class Gamepad;

class OutputManager {
public:
    // Dispatch a gamepad state update to the active transport-only output path.
    // BLE and HID over I2C are handled here; USB dispatch remains in gp2040.cpp
    // via DriverManager.
    static void dispatch(Gamepad* gamepad);
};

#endif // OUTPUT_MANAGER_H
