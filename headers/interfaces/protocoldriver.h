/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _PROTOCOLDRIVER_H_
#define _PROTOCOLDRIVER_H_

#include "gamepad.h"
#include "interfaces/transportinterface.h"
#include <cstdint>

// Forward declarations
class Gamepad;
class TransportInterface;

/**
 * @brief Protocol types supported by the driver system
 */
enum class ProtocolType {
    XINPUT,
    DINPUT,
    PS3,
    PS4,
    PS5,
    SWITCH,
    XBONE,
    XBOX_ORIGINAL,
    KEYBOARD,
    HID_GENERIC,
    MDMINI,
    NEOGEO,
    PCEMINI,
    EGRET,
    ASTRO,
    PSCLASSIC,
    CUSTOM
};

/**
 * @brief Abstract base class for protocol-specific drivers
 * 
 * This class abstracts the protocol logic from the transport mechanism,
 * allowing the same protocol to work over different transports (USB, Bluetooth, etc.)
 */
class ProtocolDriver {
public:
    virtual ~ProtocolDriver() = default;

    /**
     * @brief Initialize the protocol driver
     * @param transport Transport interface to use for communication
     * @return true if initialization was successful
     */
    virtual bool initialize(TransportInterface* transport) = 0;

    /**
     * @brief Deinitialize the protocol driver
     */
    virtual void deinitialize() = 0;

    /**
     * @brief Process gamepad input and send it via the transport
     * @param gamepad Pointer to gamepad state
     * @return true if processing was successful
     */
    virtual bool process(Gamepad* gamepad) = 0;

    /**
     * @brief Process auxiliary tasks (e.g., authentication, LEDs)
     */
    virtual void processAux() = 0;

    /**
     * @brief Get the protocol type
     * @return ProtocolType enum value
     */
    virtual ProtocolType getProtocolType() const = 0;

    /**
     * @brief Get preferred transport types for this protocol
     * @param transports Array to fill with preferred transport types
     * @param maxTransports Maximum number of transports to return
     * @return Number of transports returned
     */
    virtual size_t getPreferredTransports(TransportType* transports, size_t maxTransports) const = 0;

    /**
     * @brief Check if this protocol supports a specific transport type
     * @param transportType Transport type to check
     * @return true if supported
     */
    virtual bool supportsTransport(TransportType transportType) const = 0;

    /**
     * @brief Get the name of the protocol
     * @return Protocol name as a string
     */
    virtual const char* getProtocolName() const = 0;

    /**
     * @brief Get joystick mid value for this protocol
     * @return Mid value for analog sticks
     */
    virtual uint16_t getJoystickMidValue() = 0;

    /**
     * @brief Check if protocol supports authentication
     * @return true if authentication is supported
     */
    virtual bool supportsAuthentication() const { return false; }

    /**
     * @brief Check if protocol supports force feedback
     * @return true if force feedback is supported
     */
    virtual bool supportsForceFeedback() const { return false; }

    /**
     * @brief Handle incoming data from the transport
     * @param data Pointer to received data
     * @param length Length of received data
     * @return true if data was handled successfully
     */
    virtual bool handleIncomingData(const uint8_t* data, size_t length) { return false; }

    /**
     * @brief Get protocol-specific configuration
     * @param key Configuration key
     * @param value Pointer to store configuration value
     * @param maxLength Maximum length of value buffer
     * @return true if configuration was retrieved successfully
     */
    virtual bool getConfig(const char* key, void* value, size_t maxLength) { return false; }

    /**
     * @brief Set protocol-specific configuration
     * @param key Configuration key
     * @param value Pointer to configuration value
     * @param length Length of configuration value
     * @return true if configuration was set successfully
     */
    virtual bool setConfig(const char* key, const void* value, size_t length) { return false; }

protected:
    TransportInterface* transport = nullptr;

    /**
     * @brief Send data through the transport
     * @param data Pointer to data to send
     * @param length Length of data
     * @return Number of bytes sent, or -1 on error
     */
    int sendData(const uint8_t* data, size_t length) {
        if (!transport) return -1;
        return transport->send(data, length);
    }

    /**
     * @brief Receive data from the transport
     * @param buffer Buffer to store received data
     * @param maxLength Maximum length to receive
     * @return Number of bytes received, or -1 on error
     */
    int receiveData(uint8_t* buffer, size_t maxLength) {
        if (!transport) return -1;
        return transport->receive(buffer, maxLength);
    }

    /**
     * @brief Check if transport is ready
     * @return true if ready
     */
    bool isTransportReady() {
        return transport && transport->isReady();
    }
};

#endif // _PROTOCOLDRIVER_H_
