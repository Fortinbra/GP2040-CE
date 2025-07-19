/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "drivermanager_v2.h"

// Legacy drivers
#include "drivers/net/NetDriver.h"
#include "drivers/astro/AstroDriver.h"
#include "drivers/egret/EgretDriver.h"
#include "drivers/hid/HIDDriver.h"
#include "drivers/keyboard/KeyboardDriver.h"
#include "drivers/mdmini/MDMiniDriver.h"
#include "drivers/neogeo/NeoGeoDriver.h"
#include "drivers/pcengine/PCEngineDriver.h"
#include "drivers/psclassic/PSClassicDriver.h"
#include "drivers/ps3/PS3Driver.h"
#include "drivers/ps4/PS4Driver.h"
#include "drivers/switch/SwitchDriver.h"
#include "drivers/xbone/XBOneDriver.h"
#include "drivers/xboxog/XboxOriginalDriver.h"
#include "drivers/xinput/XInputDriver.h"

// New protocol drivers
#include "drivers/xinput/XInputProtocolDriver.h"

// Transport implementations
#include "interfaces/usbtransport.h"
#include "interfaces/bluetoothtransport.h"
#include "interfaces/gpiotransport.h"

#include <memory>

bool DriverManagerV2::setup(InputMode mode, TransportType preferredTransport) {
    // Clean up any existing setup
    deinitialize();

    inputMode = mode;

    // Check if this mode supports the new architecture
    if (supportsNewArchitecture(mode)) {
        // Use new protocol + transport architecture
        useNewArchitecture = true;

        // Create protocol driver first to determine transport requirements
        protocolDriver = createProtocolDriver(mode);
        if (!protocolDriver) {
            return false;
        }

        // Select the best transport for this protocol
        TransportType selectedTransport = selectBestTransport(mode, preferredTransport);

        // Create transport
        transport = createTransport(selectedTransport);
        if (!transport || !transport->initialize()) {
            protocolDriver.reset();
            return false;
        }

        // Initialize protocol driver with transport
        if (!protocolDriver->initialize(transport.get())) {
            transport.reset();
            protocolDriver.reset();
            return false;
        }

        return true;
    } else {
        // Fall back to legacy architecture
        useNewArchitecture = false;
        setupLegacy(mode);
        return (legacyDriver != nullptr);
    }
}

void DriverManagerV2::setupLegacy(InputMode mode) {
    legacyDriver = createLegacyDriver(mode);
    if (legacyDriver) {
        legacyDriver->initialize();
        inputMode = mode;
    }
}

bool DriverManagerV2::process(Gamepad* gamepad) {
    if (useNewArchitecture) {
        if (protocolDriver && transport) {
            return protocolDriver->process(gamepad);
        }
        return false;
    } else {
        if (legacyDriver) {
            return legacyDriver->process(gamepad);
        }
        return false;
    }
}

void DriverManagerV2::processAux() {
    if (useNewArchitecture) {
        if (protocolDriver) {
            protocolDriver->processAux();
        }
    } else {
        if (legacyDriver) {
            legacyDriver->processAux();
        }
    }
}

void DriverManagerV2::processTransport() {
    if (useNewArchitecture && transport) {
        transport->process();
    }
}

TransportType DriverManagerV2::selectBestTransport(InputMode mode, TransportType preferredTransport) {
    // Create a temporary protocol driver to query transport preferences
    auto tempProtocolDriver = createProtocolDriver(mode);
    if (!tempProtocolDriver) {
        return preferredTransport; // Fallback to preferred
    }

    // Get the protocol's preferred transports
    TransportType supportedTransports[8];
    size_t numSupported = tempProtocolDriver->getPreferredTransports(supportedTransports, 8);

    // If protocol supports the preferred transport, use it
    if (tempProtocolDriver->supportsTransport(preferredTransport)) {
        return preferredTransport;
    }

    // Otherwise, try to find an available transport from the protocol's preferences
    for (size_t i = 0; i < numSupported; i++) {
        TransportType transportType = supportedTransports[i];
        
        // Check if we can create this transport (availability check)
        auto testTransport = createTransport(transportType);
        if (testTransport && testTransport->initialize()) {
            testTransport->deinitialize(); // Clean up test transport
            return transportType;
        }
    }

    // Final fallback - return the first supported transport by the protocol
    if (numSupported > 0) {
        return supportedTransports[0];
    }

    // Ultimate fallback
    return TransportType::USB;
}

TransportType DriverManagerV2::getTransportType() const {
    if (useNewArchitecture && transport) {
        return transport->getType();
    }
    return TransportType::USB; // Default for legacy drivers
}

bool DriverManagerV2::switchTransport(TransportType newTransportType) {
    if (!useNewArchitecture || !protocolDriver) {
        return false;
    }

    // Check if the protocol supports the new transport type
    if (!protocolDriver->supportsTransport(newTransportType)) {
        return false;
    }

    // Create new transport
    auto newTransport = createTransport(newTransportType);
    if (!newTransport || !newTransport->initialize()) {
        return false;
    }

    // Deinitialize current protocol and transport
    protocolDriver->deinitialize();
    transport.reset();

    // Switch to new transport
    transport = std::move(newTransport);
    return protocolDriver->initialize(transport.get());
}

bool DriverManagerV2::switchProtocol(InputMode newMode) {
    if (!useNewArchitecture || !transport) {
        return false;
    }

    if (!supportsNewArchitecture(newMode)) {
        return false;
    }

    // Create new protocol driver
    auto newProtocolDriver = createProtocolDriver(newMode);
    if (!newProtocolDriver) {
        return false;
    }

    // Check if the new protocol supports the current transport
    if (!newProtocolDriver->supportsTransport(transport->getType())) {
        // Try to find a transport that works for the new protocol
        TransportType selectedTransport = selectBestTransport(newMode, transport->getType());
        
        // Create new transport if needed
        if (selectedTransport != transport->getType()) {
            auto newTransport = createTransport(selectedTransport);
            if (!newTransport || !newTransport->initialize()) {
                return false;
            }
            
            // Switch transport
            transport->deinitialize();
            transport = std::move(newTransport);
        }
    }

    // Initialize new protocol driver with (possibly new) transport
    if (!newProtocolDriver->initialize(transport.get())) {
        return false;
    }

    // Switch to new protocol
    if (protocolDriver) {
        protocolDriver->deinitialize();
    }
    protocolDriver = std::move(newProtocolDriver);
    inputMode = newMode;

    return true;
}

uint16_t DriverManagerV2::getJoystickMidValue() {
    if (useNewArchitecture) {
        if (protocolDriver) {
            return protocolDriver->getJoystickMidValue();
        }
    } else {
        if (legacyDriver) {
            return legacyDriver->GetJoystickMidValue();
        }
    }
    return GAMEPAD_JOYSTICK_MID; // Default value
}

void DriverManagerV2::deinitialize() {
    if (useNewArchitecture) {
        if (protocolDriver) {
            protocolDriver->deinitialize();
            protocolDriver.reset();
        }
        if (transport) {
            transport->deinitialize();
            transport.reset();
        }
    } else {
        if (legacyDriver) {
            delete legacyDriver;
            legacyDriver = nullptr;
        }
    }
    useNewArchitecture = false;
}

std::unique_ptr<TransportInterface> DriverManagerV2::createTransport(TransportType type) {
    switch (type) {
        case TransportType::USB:
            return std::make_unique<USBTransport>();
        case TransportType::BLUETOOTH:
            return std::make_unique<BluetoothTransport>();
        case TransportType::GPIO:
            return std::make_unique<GPIOTransport>();
    }
    return nullptr; // Should never reach here with the enum
}

std::unique_ptr<ProtocolDriver> DriverManagerV2::createProtocolDriver(InputMode mode) {
    switch (mode) {
        case INPUT_MODE_XINPUT:
            return std::make_unique<XInputProtocolDriver>();
        // TODO: Add other protocol drivers as they are implemented
        default:
            return nullptr;
    }
}

GPDriver* DriverManagerV2::createLegacyDriver(InputMode mode) {
    switch (mode) {
        case INPUT_MODE_CONFIG:
            return new NetDriver();
        case INPUT_MODE_ASTRO:
            return new AstroDriver();
        case INPUT_MODE_EGRET:
            return new EgretDriver();
        case INPUT_MODE_KEYBOARD:
            return new KeyboardDriver();
        case INPUT_MODE_GENERIC:
            return new HIDDriver();
        case INPUT_MODE_MDMINI:
            return new MDMiniDriver();
        case INPUT_MODE_NEOGEO:
            return new NeoGeoDriver();
        case INPUT_MODE_PSCLASSIC:
            return new PSClassicDriver();
        case INPUT_MODE_PCEMINI:
            return new PCEngineDriver();
        case INPUT_MODE_PS3:
            return new PS3Driver();
        case INPUT_MODE_PS4:
            return new PS4Driver(PS4_CONTROLLER);
        case INPUT_MODE_PS5:
            return new PS4Driver(PS4_ARCADESTICK);
        case INPUT_MODE_SWITCH:
            return new SwitchDriver();
        case INPUT_MODE_XBONE:
            return new XBOneDriver();
        case INPUT_MODE_XBOXORIGINAL:
            return new XboxOriginalDriver();
        case INPUT_MODE_XINPUT:
            return new XInputDriver();
        default:
            return nullptr;
    }
}

bool DriverManagerV2::supportsNewArchitecture(InputMode mode) const {
    // Currently only XInput is implemented with the new architecture
    // Add other modes as they are implemented
    switch (mode) {
        case INPUT_MODE_XINPUT:
            return true;
        default:
            return false;
    }
}

ProtocolType DriverManagerV2::mapInputModeToProtocol(InputMode mode) const {
    switch (mode) {
        case INPUT_MODE_XINPUT:
            return ProtocolType::XINPUT;
        case INPUT_MODE_PS3:
            return ProtocolType::PS3;
        case INPUT_MODE_PS4:
            return ProtocolType::PS4;
        case INPUT_MODE_PS5:
            return ProtocolType::PS5;
        case INPUT_MODE_SWITCH:
            return ProtocolType::SWITCH;
        case INPUT_MODE_XBONE:
            return ProtocolType::XBONE;
        case INPUT_MODE_XBOXORIGINAL:
            return ProtocolType::XBOX_ORIGINAL;
        case INPUT_MODE_KEYBOARD:
            return ProtocolType::KEYBOARD;
        case INPUT_MODE_GENERIC:
            return ProtocolType::HID_GENERIC;
        default:
            return ProtocolType::CUSTOM;
    }
}
