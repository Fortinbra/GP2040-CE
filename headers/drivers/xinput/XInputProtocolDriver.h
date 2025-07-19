/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _XINPUTPROTOCOLDRIVER_H_
#define _XINPUTPROTOCOLDRIVER_H_

#include "interfaces/protocoldriver.h"
#include "drivers/xinput/XInputDescriptors.h"
#include <cstdint>

/**
 * @brief XInput protocol driver using the new architecture
 * 
 * This driver implements the XInput protocol without direct TinyUSB dependencies,
 * allowing it to work over different transports (USB, Bluetooth, etc.)
 */
class XInputProtocolDriver : public ProtocolDriver {
public:
    XInputProtocolDriver();
    virtual ~XInputProtocolDriver();

    // ProtocolDriver implementation
    bool initialize(TransportInterface* transport) override;
    void deinitialize() override;
    bool process(Gamepad* gamepad) override;
    void processAux() override;
    ProtocolType getProtocolType() const override { return ProtocolType::XINPUT; }
    const char* getProtocolName() const override { return "XInput"; }
    uint16_t getJoystickMidValue() override { return GAMEPAD_JOYSTICK_MID; }
    bool supportsAuthentication() const override { return true; }
    bool supportsForceFeedback() const override { return true; }
    bool handleIncomingData(const uint8_t* data, size_t length) override;
    
    // Transport determination methods
    size_t getPreferredTransports(TransportType* transports, size_t maxTransports) const override;
    bool supportsTransport(TransportType transportType) const override;

    // XInput-specific methods
    
    /**
     * @brief Get the current XInput report
     * @return Pointer to current report
     */
    const XInputReport* getCurrentReport() const { return &xinputReport; }
    
    /**
     * @brief Set player LED pattern
     * @param pattern LED pattern to set
     */
    void setPlayerLED(uint8_t pattern);
    
    /**
     * @brief Get authentication status
     * @return true if authenticated
     */
    bool isAuthenticated() const { return authenticated; }
    
    /**
     * @brief Handle XInput vendor control requests
     * @param stage Control transfer stage
     * @param request Control request
     * @return true if handled
     */
    bool handleVendorControlRequest(uint8_t stage, const void* request);

protected:
    /**
     * @brief Convert gamepad state to XInput report
     * @param gamepad Source gamepad state
     */
    void convertGamepadToXInput(Gamepad* gamepad);
    
    /**
     * @brief Process XInput-specific features
     * @param data Feature data
     * @param length Data length
     */
    void processFeatureReport(const uint8_t* data, size_t length);
    
    /**
     * @brief Handle rumble/vibration commands
     * @param data Rumble data
     * @param length Data length
     */
    void handleRumble(const uint8_t* data, size_t length);

private:
    XInputReport xinputReport;
    uint8_t playerLED;
    bool authenticated;
    uint8_t lastReport[32];
    
    // Feature and rumble buffers
    uint8_t featureBuffer[32];
    uint8_t rumbleBuffer[8];
    
    // Authentication state
    bool authenticationInProgress;
    uint32_t authTimer;
    
    // Constants
    static constexpr uint8_t XINPUT_PLED_OFF = 0x00;
    static constexpr uint8_t XINPUT_PLED_ON1 = 0x06;
    static constexpr uint8_t XINPUT_PLED_ON2 = 0x07;
    static constexpr uint8_t XINPUT_PLED_ON3 = 0x08;
    static constexpr uint8_t XINPUT_PLED_ON4 = 0x09;
};

#endif // _XINPUTPROTOCOLDRIVER_H_
