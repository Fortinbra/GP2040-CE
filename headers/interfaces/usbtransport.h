/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _USBTRANSPORT_H_
#define _USBTRANSPORT_H_

#include "interfaces/transportinterface.h"
#include "tusb_config.h"
#include "tusb.h"
#include "class/hid/hid.h"
#include "device/usbd_pvt.h"

/**
 * @brief USB transport implementation using TinyUSB
 * 
 * This class wraps TinyUSB functionality to provide a transport interface
 * for USB communication, isolating protocol drivers from TinyUSB dependencies.
 */
class USBTransport : public TransportInterface {
public:
    USBTransport();
    virtual ~USBTransport();

    // TransportInterface implementation
    bool initialize() override;
    void deinitialize() override;
    bool isReady() override;
    int send(const uint8_t* data, size_t length) override;
    int receive(uint8_t* buffer, size_t maxLength) override;
    void process() override;
    TransportType getType() const override { return TransportType::USB; }
    size_t getMTU() const override { return 64; } // Standard USB packet size

    // USB-specific methods
    bool setDescriptors(const uint8_t* device_desc, 
                       const uint8_t* config_desc,
                       const uint8_t* hid_report_desc,
                       const uint16_t* string_desc[],
                       size_t string_count);

    bool registerClassDriver(const usbd_class_driver_t* driver);
    
    // USB control transfer handlers
    bool handleVendorControlTransfer(uint8_t rhport, uint8_t stage, 
                                   tusb_control_request_t const* request);
    
    bool handleGetReport(uint8_t report_id, hid_report_type_t report_type, 
                        uint8_t* buffer, uint16_t reqlen);
    
    bool handleSetReport(uint8_t report_id, hid_report_type_t report_type, 
                        const uint8_t* buffer, uint16_t bufsize);

    // Endpoint management
    bool openEndpointPair(uint8_t rhport, const uint8_t* desc, 
                         uint8_t num_endpoints, uint8_t xfer_type,
                         uint8_t* out_ep, uint8_t* in_ep);

    bool sendEndpointData(uint8_t endpoint, const uint8_t* data, size_t length);
    bool receiveEndpointData(uint8_t endpoint, uint8_t* buffer, size_t length);

protected:
    struct USBState {
        bool initialized;
        bool connected;
        bool configured;
        uint8_t in_endpoint;
        uint8_t out_endpoint;
        const usbd_class_driver_t* class_driver;
        
        // Descriptor storage
        const uint8_t* device_descriptor;
        const uint8_t* config_descriptor;
        const uint8_t* hid_report_descriptor;
        const uint16_t** string_descriptors;
        size_t string_count;
    } state;

    // Receive buffer for incoming data
    uint8_t receive_buffer[64];
    size_t receive_length;
    bool data_available;

private:
    static USBTransport* instance; // For static callbacks
    
    // Static TinyUSB callbacks
    static void tud_mount_cb();
    static void tud_umount_cb();
    static void tud_suspend_cb(bool remote_wakeup_en);
    static void tud_resume_cb();
    static bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, 
                                          tusb_control_request_t const* request);
};

#endif // _USBTRANSPORT_H_
