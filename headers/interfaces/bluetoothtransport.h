/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _BLUETOOTHTRANSPORT_H_
#define _BLUETOOTHTRANSPORT_H_

#include "interfaces/transportinterface.h"

/**
 * @brief Bluetooth transport implementation for BTStack
 * 
 * This class provides a transport interface for Bluetooth communication,
 * isolating protocol drivers from BTStack dependencies.
 * 
 * Note: This is a placeholder implementation. BTStack integration
 * would require additional setup and configuration.
 */
class BluetoothTransport : public TransportInterface {
public:
    BluetoothTransport();
    virtual ~BluetoothTransport();

    // TransportInterface implementation
    bool initialize() override;
    void deinitialize() override;
    bool isReady() override;
    int send(const uint8_t* data, size_t length) override;
    int receive(uint8_t* buffer, size_t maxLength) override;
    void process() override;
    TransportType getType() const override { return TransportType::BLUETOOTH; }
    size_t getMTU() const override { return 512; } // Bluetooth L2CAP MTU

    // Bluetooth-specific methods
    
    /**
     * @brief Start advertising as a HID device
     * @param device_name Name to advertise
     * @return true if advertising started successfully
     */
    bool startAdvertising(const char* device_name);
    
    /**
     * @brief Stop advertising
     */
    void stopAdvertising();
    
    /**
     * @brief Check if a device is connected
     * @return true if connected
     */
    bool isConnected() const;
    
    /**
     * @brief Disconnect from the current device
     */
    void disconnect();
    
    /**
     * @brief Set the HID report map
     * @param report_map Pointer to HID report descriptor
     * @param length Length of the report descriptor
     * @return true if set successfully
     */
    bool setHIDReportMap(const uint8_t* report_map, size_t length);
    
    /**
     * @brief Send HID report
     * @param report_id Report ID
     * @param report_data Report data
     * @param length Length of report data
     * @return true if sent successfully
     */
    bool sendHIDReport(uint8_t report_id, const uint8_t* report_data, size_t length);
    
    /**
     * @brief Get the connected device's address
     * @param address Buffer to store address (6 bytes)
     * @return true if address retrieved successfully
     */
    bool getConnectedDeviceAddress(uint8_t* address);

protected:
    struct BluetoothState {
        bool initialized;
        bool advertising;
        bool connected;
        char device_name[32];
        uint8_t connected_address[6];
        const uint8_t* hid_report_map;
        size_t report_map_length;
    } state;

    // Receive buffer for incoming data
    uint8_t receive_buffer[512];
    size_t receive_length;
    bool data_available;

private:
    // BTStack event handlers (would be implemented with actual BTStack)
    void handleConnectionEvent(bool connected);
    void handleDataReceived(const uint8_t* data, size_t length);
    
    // Placeholder for BTStack context
    void* btstack_context;
};

#endif // _BLUETOOTHTRANSPORT_H_
