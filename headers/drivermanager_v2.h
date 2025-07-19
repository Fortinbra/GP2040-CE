/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _DRIVERMANAGER_V2_H_
#define _DRIVERMANAGER_V2_H_

#include "enums.pb.h"
#include "gpdriver.h"
#include "interfaces/protocoldriver.h"
#include "interfaces/transportinterface.h"
#include <memory>

/**
 * @brief Enhanced driver manager supporting both legacy and new architecture
 * 
 * This manager supports:
 * - Legacy GPDriver interface for backward compatibility
 * - New ProtocolDriver + TransportInterface architecture for modularity
 * - Multiple transport types (USB, Bluetooth, etc.)
 * - Runtime switching between drivers and transports
 */
class DriverManagerV2 {
public:
    DriverManagerV2(DriverManagerV2 const&) = delete;
    void operator=(DriverManagerV2 const&) = delete;
    
    static DriverManagerV2& getInstance() {
        static DriverManagerV2 instance;
        return instance;
    }

    /**
     * @brief Initialize the driver manager with a specific input mode
     * Protocol drivers will determine their preferred transport types
     * @param mode Input mode (protocol type)
     * @param preferredTransport Optional preferred transport type
     * @return true if initialization was successful
     */
    bool setup(InputMode mode, TransportType preferredTransport = TransportType::USB);

    /**
     * @brief Legacy setup method for backward compatibility
     * @param mode Input mode
     */
    void setupLegacy(InputMode mode);

    /**
     * @brief Process gamepad input through the active driver
     * @param gamepad Pointer to gamepad state
     * @return true if processing was successful
     */
    bool process(class Gamepad* gamepad);

    /**
     * @brief Process auxiliary tasks
     */
    void processAux();

    /**
     * @brief Process transport tasks
     */
    void processTransport();

    /**
     * @brief Get the current input mode
     * @return Current InputMode
     */
    InputMode getInputMode() const { return inputMode; }

    /**
     * @brief Check if in config mode
     * @return true if in config mode
     */
    bool isConfigMode() const { return (inputMode == INPUT_MODE_CONFIG); }

    /**
     * @brief Get the current transport type
     * @return Current TransportType
     */
    TransportType getTransportType() const;

    /**
     * @brief Auto-select the best available transport for a protocol
     * @param mode Input mode (protocol type)
     * @param preferredTransport Optional preferred transport type
     * @return Selected transport type, or TransportType::USB as fallback
     */
    TransportType selectBestTransport(InputMode mode, TransportType preferredTransport = TransportType::USB);

    /**
     * @brief Get the legacy driver (for compatibility)
     * @return Pointer to GPDriver or nullptr
     */
    GPDriver* getLegacyDriver() { return legacyDriver; }

    /**
     * @brief Get the protocol driver
     * @return Pointer to ProtocolDriver or nullptr
     */
    ProtocolDriver* getProtocolDriver() { return protocolDriver.get(); }

    /**
     * @brief Get the transport interface
     * @return Pointer to TransportInterface or nullptr
     */
    TransportInterface* getTransport() { return transport.get(); }

    /**
     * @brief Switch to a different transport while keeping the same protocol
     * @param newTransportType New transport type
     * @return true if switch was successful
     */
    bool switchTransport(TransportType newTransportType);

    /**
     * @brief Switch to a different protocol while keeping the same transport
     * @param newMode New input mode (protocol)
     * @return true if switch was successful
     */
    bool switchProtocol(InputMode newMode);

    /**
     * @brief Check if using new architecture (protocol + transport)
     * @return true if using new architecture, false if legacy
     */
    bool isUsingNewArchitecture() const { return useNewArchitecture; }

    /**
     * @brief Get joystick mid value from active driver
     * @return Joystick mid value
     */
    uint16_t getJoystickMidValue();

    /**
     * @brief Deinitialize and cleanup
     */
    void deinitialize();

private:
    DriverManagerV2() : 
        inputMode(INPUT_MODE_XINPUT),
        legacyDriver(nullptr),
        useNewArchitecture(false) {}

    ~DriverManagerV2() {
        deinitialize();
    }

    /**
     * @brief Create a transport interface for the specified type
     * @param type Transport type
     * @return Unique pointer to transport interface
     */
    std::unique_ptr<TransportInterface> createTransport(TransportType type);

    /**
     * @brief Create a protocol driver for the specified input mode
     * @param mode Input mode
     * @return Unique pointer to protocol driver
     */
    std::unique_ptr<ProtocolDriver> createProtocolDriver(InputMode mode);

    /**
     * @brief Create a legacy driver for the specified input mode
     * @param mode Input mode
     * @return Pointer to GPDriver
     */
    GPDriver* createLegacyDriver(InputMode mode);

    /**
     * @brief Check if an input mode supports the new architecture
     * @param mode Input mode to check
     * @return true if supported
     */
    bool supportsNewArchitecture(InputMode mode) const;

    /**
     * @brief Map InputMode to ProtocolType
     * @param mode Input mode
     * @return Corresponding protocol type
     */
    ProtocolType mapInputModeToProtocol(InputMode mode) const;

    InputMode inputMode;
    
    // Legacy architecture
    GPDriver* legacyDriver;
    
    // New architecture
    std::unique_ptr<ProtocolDriver> protocolDriver;
    std::unique_ptr<TransportInterface> transport;
    bool useNewArchitecture;
};

#endif // _DRIVERMANAGER_V2_H_
