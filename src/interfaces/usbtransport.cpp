/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "interfaces/usbtransport.h"
#include <cstring>

USBTransport* USBTransport::instance = nullptr;

USBTransport::USBTransport() {
    instance = this;
    memset(&state, 0, sizeof(state));
    receive_length = 0;
    data_available = false;
}

USBTransport::~USBTransport() {
    deinitialize();
    if (instance == this) {
        instance = nullptr;
    }
}

bool USBTransport::initialize() {
    if (state.initialized) {
        return true;
    }

    // Initialize TinyUSB
    bool result = tusb_init();
    if (result) {
        state.initialized = true;
        state.connected = false;
        state.configured = false;
    }

    return result;
}

void USBTransport::deinitialize() {
    if (state.initialized) {
        // TinyUSB doesn't have a formal deinit, but we can reset our state
        state.initialized = false;
        state.connected = false;
        state.configured = false;
        state.class_driver = nullptr;
    }
}

bool USBTransport::isReady() {
    return state.initialized && state.connected && state.configured && tud_ready();
}

int USBTransport::send(const uint8_t* data, size_t length) {
    if (!isReady() || !data || length == 0) {
        return -1;
    }

    // Use the configured in endpoint
    if (state.in_endpoint != 0) {
        return sendEndpointData(state.in_endpoint, data, length) ? length : -1;
    }

    return -1;
}

int USBTransport::receive(uint8_t* buffer, size_t maxLength) {
    if (!isReady() || !buffer || maxLength == 0) {
        return -1;
    }

    if (!data_available) {
        return 0;
    }

    size_t copyLength = (receive_length < maxLength) ? receive_length : maxLength;
    memcpy(buffer, receive_buffer, copyLength);
    
    data_available = false;
    receive_length = 0;

    return copyLength;
}

void USBTransport::process() {
    if (state.initialized) {
        tud_task();
    }
}

bool USBTransport::setDescriptors(const uint8_t* device_desc,
                                 const uint8_t* config_desc,
                                 const uint8_t* hid_report_desc,
                                 const uint16_t* string_desc[],
                                 size_t string_count) {
    state.device_descriptor = device_desc;
    state.config_descriptor = config_desc;
    state.hid_report_descriptor = hid_report_desc;
    state.string_descriptors = string_desc;
    state.string_count = string_count;
    return true;
}

bool USBTransport::registerClassDriver(const usbd_class_driver_t* driver) {
    state.class_driver = driver;
    return true;
}

bool USBTransport::handleVendorControlTransfer(uint8_t rhport, uint8_t stage,
                                              tusb_control_request_t const* request) {
    // Default implementation - can be overridden by derived classes
    return false;
}

bool USBTransport::handleGetReport(uint8_t report_id, hid_report_type_t report_type,
                                  uint8_t* buffer, uint16_t reqlen) {
    // Default implementation - can be overridden by derived classes
    return false;
}

bool USBTransport::handleSetReport(uint8_t report_id, hid_report_type_t report_type,
                                  const uint8_t* buffer, uint16_t bufsize) {
    // Default implementation - can be overridden by derived classes
    return false;
}

bool USBTransport::openEndpointPair(uint8_t rhport, const uint8_t* desc,
                                   uint8_t num_endpoints, uint8_t xfer_type,
                                   uint8_t* out_ep, uint8_t* in_ep) {
    bool result = usbd_open_edpt_pair(rhport, desc, num_endpoints, xfer_type, out_ep, in_ep);
    if (result) {
        state.out_endpoint = *out_ep;
        state.in_endpoint = *in_ep;
    }
    return result;
}

bool USBTransport::sendEndpointData(uint8_t endpoint, const uint8_t* data, size_t length) {
    return usbd_edpt_xfer(0, endpoint, (uint8_t*)data, length);
}

bool USBTransport::receiveEndpointData(uint8_t endpoint, uint8_t* buffer, size_t length) {
    if (length > sizeof(receive_buffer)) {
        length = sizeof(receive_buffer);
    }

    bool result = usbd_edpt_xfer(0, endpoint, buffer, length);
    if (result) {
        memcpy(receive_buffer, buffer, length);
        receive_length = length;
        data_available = true;
    }
    return result;
}

// Static TinyUSB callbacks
void USBTransport::tud_mount_cb() {
    if (instance) {
        instance->state.connected = true;
    }
}

void USBTransport::tud_umount_cb() {
    if (instance) {
        instance->state.connected = false;
        instance->state.configured = false;
    }
}

void USBTransport::tud_suspend_cb(bool remote_wakeup_en) {
    (void)remote_wakeup_en;
    if (instance) {
        instance->state.configured = false;
    }
}

void USBTransport::tud_resume_cb() {
    if (instance) {
        instance->state.configured = true;
    }
}

bool USBTransport::tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage,
                                              tusb_control_request_t const* request) {
    if (instance) {
        return instance->handleVendorControlTransfer(rhport, stage, request);
    }
    return false;
}

// TinyUSB callback implementations - safe since legacy USB driver is excluded
extern "C" {
    void tud_mount_cb(void) {
        USBTransport::tud_mount_cb();
    }

    void tud_umount_cb(void) {
        USBTransport::tud_umount_cb();
    }

    void tud_suspend_cb(bool remote_wakeup_en) {
        USBTransport::tud_suspend_cb(remote_wakeup_en);
    }

    void tud_resume_cb(void) {
        USBTransport::tud_resume_cb();
    }

    bool tud_vendor_control_xfer_cb(uint8_t rhport, uint8_t stage, tusb_control_request_t const* request) {
        return USBTransport::tud_vendor_control_xfer_cb(rhport, stage, request);
    }
}
