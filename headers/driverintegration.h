/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _DRIVERINTEGRATION_H_
#define _DRIVERINTEGRATION_H_

#include "drivermanager.h"
#include "drivermanager_v2.h"
#include "gamepad.h"

/**
 * @brief Integration layer to smoothly transition between old and new driver architectures
 * 
 * This class provides a unified interface that can use either the legacy DriverManager
 * or the new DriverManagerV2, allowing for gradual migration and testing.
 */
class DriverIntegration {
public:
    static DriverIntegration& getInstance() {
        static DriverIntegration instance;
        return instance;
    }

    /**
     * @brief Initialize the driver system
     * @param mode Input mode
     * @param useNewArchitecture Whether to use the new protocol/transport architecture
     * @param transportType Transport type (only used with new architecture)
     * @return true if initialization was successful
     */
    bool initialize(InputMode mode, bool useNewArchitecture = false, 
                   TransportType transportType = TransportType::USB);

    /**
     * @brief Process gamepad input
     * @param gamepad Pointer to gamepad state
     * @return true if processing was successful
     */
    bool process(Gamepad* gamepad);

    /**
     * @brief Process auxiliary tasks
     */
    void processAux();

    /**
     * @brief Get current input mode
     * @return Current InputMode
     */
    InputMode getInputMode() const;

    /**
     * @brief Check if in config mode
     * @return true if in config mode
     */
    bool isConfigMode() const;

    /**
     * @brief Get joystick mid value
     * @return Joystick mid value
     */
    uint16_t getJoystickMidValue();

    /**
     * @brief Check if using new architecture
     * @return true if using new architecture
     */
    bool isUsingNewArchitecture() const { return usingNewArchitecture; }

    /**
     * @brief Get legacy driver (for compatibility)
     * @return Pointer to GPDriver or nullptr
     */
    GPDriver* getLegacyDriver();

    /**
     * @brief Get transport type (new architecture only)
     * @return Transport type
     */
    TransportType getTransportType() const;

    /**
     * @brief Switch transport (new architecture only)
     * @param newTransportType New transport type
     * @return true if switch was successful
     */
    bool switchTransport(TransportType newTransportType);

    /**
     * @brief Enable/disable new architecture features
     * @param enable Whether to enable new architecture
     * @return true if changed successfully
     */
    bool setNewArchitectureEnabled(bool enable);

private:
    DriverIntegration() : usingNewArchitecture(false) {}
    ~DriverIntegration() = default;

    bool usingNewArchitecture;
};

/**
 * @brief Configuration flags for driver architecture selection
 */
struct DriverConfig {
    bool enableNewArchitecture = false;         // Enable new protocol/transport architecture
    bool enableBluetoothTransport = false;     // Enable Bluetooth transport
    bool enableGPIOTransport = false;          // Enable GPIO transport for retro consoles
    bool enableRuntimeSwitching = false;       // Enable runtime transport switching
    TransportType defaultTransport = TransportType::USB;  // Default transport type
};

/**
 * @brief Global driver configuration
 */
extern DriverConfig g_driverConfig;

/**
 * @brief Initialize driver system based on configuration
 * @param mode Input mode
 * @return true if initialization was successful
 */
bool initializeDriverSystem(InputMode mode);

/**
 * @brief Process driver tasks (call from main loop)
 * @param gamepad Pointer to gamepad state
 */
void processDriverSystem(Gamepad* gamepad);

/**
 * @brief Cleanup driver system
 */
void cleanupDriverSystem();

#endif // _DRIVERINTEGRATION_H_
