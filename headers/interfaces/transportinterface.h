/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _TRANSPORTINTERFACE_H_
#define _TRANSPORTINTERFACE_H_

#include <cstdint>
#include <cstddef>

/**
 * @brief Transport types supported by the driver system
 */
enum class TransportType {
    USB,
    BLUETOOTH,
    GPIO
};

/**
 * @brief Abstract transport interface for communication between drivers and hardware
 * 
 * This interface abstracts the underlying transport mechanism (USB, Bluetooth, etc.)
 * from the protocol-specific driver logic, allowing drivers to work with different
 * transport layers without being tightly coupled to TinyUSB or BTStack.
 */
class TransportInterface {
public:
    virtual ~TransportInterface() = default;

    /**
     * @brief Initialize the transport layer
     * @return true if initialization was successful
     */
    virtual bool initialize() = 0;

    /**
     * @brief Deinitialize the transport layer
     */
    virtual void deinitialize() = 0;

    /**
     * @brief Check if the transport is ready for communication
     * @return true if ready
     */
    virtual bool isReady() = 0;

    /**
     * @brief Send data through the transport
     * @param data Pointer to data buffer
     * @param length Length of data to send
     * @return Number of bytes actually sent, or -1 on error
     */
    virtual int send(const uint8_t* data, size_t length) = 0;

    /**
     * @brief Receive data from the transport
     * @param buffer Buffer to store received data
     * @param maxLength Maximum length of data to receive
     * @return Number of bytes actually received, or -1 on error
     */
    virtual int receive(uint8_t* buffer, size_t maxLength) = 0;

    /**
     * @brief Process transport-specific tasks (called from main loop)
     */
    virtual void process() = 0;

    /**
     * @brief Get the transport type
     * @return TransportType enum value
     */
    virtual TransportType getType() const = 0;

    /**
     * @brief Get maximum transmission unit (MTU) for this transport
     * @return Maximum packet size in bytes
     */
    virtual size_t getMTU() const = 0;

    /**
     * @brief Check if transport supports bidirectional communication
     * @return true if bidirectional
     */
    virtual bool isBidirectional() const { return true; }

    /**
     * @brief Get transport-specific configuration data
     * @param key Configuration key
     * @param value Pointer to store configuration value
     * @param maxLength Maximum length of value buffer
     * @return true if configuration was retrieved successfully
     */
    virtual bool getConfig(const char* key, void* value, size_t maxLength) { return false; }

    /**
     * @brief Set transport-specific configuration data
     * @param key Configuration key
     * @param value Pointer to configuration value
     * @param length Length of configuration value
     * @return true if configuration was set successfully
     */
    virtual bool setConfig(const char* key, const void* value, size_t length) { return false; }
};

#endif // _TRANSPORTINTERFACE_H_
