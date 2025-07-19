/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _SNESPROTOCOLDRIVER_H_
#define _SNESPROTOCOLDRIVER_H_

#include "interfaces/protocoldriver.h"
#include "interfaces/gpiotransport.h"

/**
 * @brief SNES controller protocol driver
 * 
 * This driver implements the SNES controller protocol for communicating
 * with SNES consoles or devices that expect SNES controller data.
 * It demonstrates how custom protocols can be implemented without USB dependencies.
 */
class SNESProtocolDriver : public ProtocolDriver {
public:
    SNESProtocolDriver();
    virtual ~SNESProtocolDriver();

    // ProtocolDriver implementation
    bool initialize(TransportInterface* transport) override;
    void deinitialize() override;
    bool process(Gamepad* gamepad) override;
    void processAux() override;
    ProtocolType getProtocolType() const override { return ProtocolType::CUSTOM; }
    const char* getProtocolName() const override { return "SNES"; }
    uint16_t getJoystickMidValue() override { return 128; } // SNES has no analog sticks
    bool supportsAuthentication() const override { return false; }
    bool supportsForceFeedback() const override { return false; }
    bool handleIncomingData(const uint8_t* data, size_t length) override;
    
    // Transport determination methods
    size_t getPreferredTransports(TransportType* transports, size_t maxTransports) const override;
    bool supportsTransport(TransportType transportType) const override;

    // SNES-specific methods
    
    /**
     * @brief Set the controller ID (for multitap support)
     * @param id Controller ID (0-3)
     */
    void setControllerID(uint8_t id) { controllerID = id; }
    
    /**
     * @brief Get the current controller ID
     * @return Controller ID
     */
    uint8_t getControllerID() const { return controllerID; }
    
    /**
     * @brief Configure SNES timing parameters
     * @param clock_period_us Clock period in microseconds
     * @param latch_duration_us Latch pulse duration in microseconds
     */
    void setTimingParameters(uint32_t clock_period_us, uint32_t latch_duration_us);

protected:
    /**
     * @brief Convert gamepad state to SNES button format
     * @param gamepad Source gamepad state
     * @return 16-bit SNES button state
     */
    uint16_t convertGamepadToSNES(Gamepad* gamepad);
    
    /**
     * @brief Send SNES button data
     * @param buttons 16-bit button state
     * @return true if sent successfully
     */
    bool sendSNESData(uint16_t buttons);
    
    /**
     * @brief Handle SNES controller polling
     * @return true if polling handled successfully
     */
    bool handlePolling();

private:
    GPIOTransport* gpioTransport;
    uint8_t controllerID;
    uint16_t lastButtons;
    
    // SNES timing parameters
    uint32_t clockPeriod;
    uint32_t latchDuration;
    
    // SNES button bit positions
    static constexpr uint16_t SNES_B       = 0x8000;
    static constexpr uint16_t SNES_Y       = 0x4000;
    static constexpr uint16_t SNES_SELECT  = 0x2000;
    static constexpr uint16_t SNES_START   = 0x1000;
    static constexpr uint16_t SNES_UP      = 0x0800;
    static constexpr uint16_t SNES_DOWN    = 0x0400;
    static constexpr uint16_t SNES_LEFT    = 0x0200;
    static constexpr uint16_t SNES_RIGHT   = 0x0100;
    static constexpr uint16_t SNES_A       = 0x0080;
    static constexpr uint16_t SNES_X       = 0x0040;
    static constexpr uint16_t SNES_L       = 0x0020;
    static constexpr uint16_t SNES_R       = 0x0010;
    
    // State tracking
    bool pollingActive;
    uint32_t lastPollTime;
    static constexpr uint32_t POLL_TIMEOUT_MS = 100;
};

#endif // _SNESPROTOCOLDRIVER_H_
