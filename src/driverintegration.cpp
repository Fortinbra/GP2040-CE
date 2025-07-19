/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "driverintegration.h"
#include "storagemanager.h"

// Global driver configuration
DriverConfig g_driverConfig;

bool DriverIntegration::initialize(InputMode mode, bool useNewArchitecture, 
                                  TransportType transportType) {
    usingNewArchitecture = useNewArchitecture;

    if (usingNewArchitecture) {
        // Use new protocol/transport architecture
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.setup(mode, transportType);
    } else {
        // Use legacy architecture
        DriverManager& manager = DriverManager::getInstance();
        manager.setup(mode);
        return (manager.getDriver() != nullptr);
    }
}

bool DriverIntegration::process(Gamepad* gamepad) {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.process(gamepad);
    } else {
        DriverManager& manager = DriverManager::getInstance();
        GPDriver* driver = manager.getDriver();
        return driver ? driver->process(gamepad) : false;
    }
}

void DriverIntegration::processAux() {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        manager.processAux();
        manager.processTransport(); // Process transport-specific tasks
    } else {
        DriverManager& manager = DriverManager::getInstance();
        GPDriver* driver = manager.getDriver();
        if (driver) {
            driver->processAux();
        }
    }
}

InputMode DriverIntegration::getInputMode() const {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.getInputMode();
    } else {
        DriverManager& manager = DriverManager::getInstance();
        return manager.getInputMode();
    }
}

bool DriverIntegration::isConfigMode() const {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.isConfigMode();
    } else {
        DriverManager& manager = DriverManager::getInstance();
        return manager.isConfigMode();
    }
}

uint16_t DriverIntegration::getJoystickMidValue() {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.getJoystickMidValue();
    } else {
        DriverManager& manager = DriverManager::getInstance();
        GPDriver* driver = manager.getDriver();
        return driver ? driver->GetJoystickMidValue() : GAMEPAD_JOYSTICK_MID;
    }
}

GPDriver* DriverIntegration::getLegacyDriver() {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.getLegacyDriver();
    } else {
        DriverManager& manager = DriverManager::getInstance();
        return manager.getDriver();
    }
}

TransportType DriverIntegration::getTransportType() const {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.getTransportType();
    }
    return TransportType::USB; // Legacy always uses USB
}

bool DriverIntegration::switchTransport(TransportType newTransportType) {
    if (usingNewArchitecture) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        return manager.switchTransport(newTransportType);
    }
    return false; // Not supported in legacy mode
}

bool DriverIntegration::setNewArchitectureEnabled(bool enable) {
    if (enable == usingNewArchitecture) {
        return true; // Already in the desired state
    }

    // For now, we don't support runtime switching between architectures
    // This would require more complex state management
    return false;
}

// Global functions for easy integration

bool initializeDriverSystem(InputMode mode) {
    DriverIntegration& integration = DriverIntegration::getInstance();
    
    // Determine which architecture to use based on configuration and mode
    bool useNewArch = g_driverConfig.enableNewArchitecture;
    
    // Check if the mode supports new architecture
    if (useNewArch) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        // Use supportsNewArchitecture to check if this mode is available
        // For now, create a temporary instance to check
        auto tempManager = DriverManagerV2::getInstance();
        // We need to expose supportsNewArchitecture as public or add a public method
        // For now, we'll assume XInput supports new architecture
        if (mode != INPUT_MODE_XINPUT) {
            useNewArch = false; // Fall back to legacy for unsupported modes
        }
    }
    
    TransportType transport = g_driverConfig.defaultTransport;
    
    return integration.initialize(mode, useNewArch, transport);
}

void processDriverSystem(Gamepad* gamepad) {
    DriverIntegration& integration = DriverIntegration::getInstance();
    
    // Process gamepad input
    integration.process(gamepad);
    
    // Process auxiliary tasks
    integration.processAux();
}

void cleanupDriverSystem() {
    DriverIntegration& integration = DriverIntegration::getInstance();
    
    if (integration.isUsingNewArchitecture()) {
        DriverManagerV2& manager = DriverManagerV2::getInstance();
        manager.deinitialize();
    }
    // Legacy DriverManager doesn't have explicit cleanup
}
