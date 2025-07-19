/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "drivers/xinput/XInputProtocolDriver.h"
#include "drivers/shared/driverhelper.h"
#include "gamepad.h"
#include <cstring>

XInputProtocolDriver::XInputProtocolDriver() :
    playerLED(XINPUT_PLED_OFF),
    authenticated(false),
    authenticationInProgress(false),
    authTimer(0) {
    
    memset(&xinputReport, 0, sizeof(xinputReport));
    memset(lastReport, 0, sizeof(lastReport));
    memset(featureBuffer, 0, sizeof(featureBuffer));
    memset(rumbleBuffer, 0, sizeof(rumbleBuffer));
}

XInputProtocolDriver::~XInputProtocolDriver() {
    deinitialize();
}

bool XInputProtocolDriver::initialize(TransportInterface* transport) {
    if (!transport) {
        return false;
    }

    this->transport = transport;

    // Initialize XInput report structure
    xinputReport.report_id = 0;
    xinputReport.report_size = XINPUT_ENDPOINT_SIZE;
    xinputReport.buttons1 = 0;
    xinputReport.buttons2 = 0;
    xinputReport.lt = 0;
    xinputReport.rt = 0;
    xinputReport.lx = GAMEPAD_JOYSTICK_MID;
    xinputReport.ly = GAMEPAD_JOYSTICK_MID;
    xinputReport.rx = GAMEPAD_JOYSTICK_MID;
    xinputReport.ry = GAMEPAD_JOYSTICK_MID;
    memset(xinputReport._reserved, 0, sizeof(xinputReport._reserved));

    // Reset authentication state
    authenticated = false;
    authenticationInProgress = false;
    playerLED = XINPUT_PLED_OFF;

    return true;
}

void XInputProtocolDriver::deinitialize() {
    transport = nullptr;
    authenticated = false;
    authenticationInProgress = false;
}

bool XInputProtocolDriver::process(Gamepad* gamepad) {
    if (!transport || !gamepad) {
        return false;
    }

    // Convert gamepad state to XInput format
    convertGamepadToXInput(gamepad);

    // Only send if report has changed
    if (memcmp(&xinputReport, lastReport, sizeof(xinputReport)) != 0) {
        int result = sendData((const uint8_t*)&xinputReport, sizeof(xinputReport));
        if (result > 0) {
            memcpy(lastReport, &xinputReport, sizeof(xinputReport));
            return true;
        }
        return false;
    }

    return true;
}

void XInputProtocolDriver::processAux() {
    if (!transport) {
        return;
    }

    // Handle authentication if needed
    if (authenticationInProgress) {
        // Simple authentication simulation
        // In a real implementation, this would handle the XInput authentication protocol
        authTimer++;
        if (authTimer > 1000) { // Simulate auth completion after some time
            authenticated = true;
            authenticationInProgress = false;
            setPlayerLED(XINPUT_PLED_ON1); // Set player 1 LED
        }
    }

    // Check for incoming data
    uint8_t buffer[64];
    int received = receiveData(buffer, sizeof(buffer));
    if (received > 0) {
        handleIncomingData(buffer, received);
    }
}

bool XInputProtocolDriver::handleIncomingData(const uint8_t* data, size_t length) {
    if (!data || length == 0) {
        return false;
    }

    // Handle different types of incoming data
    if (length >= 2) {
        uint8_t reportType = data[0];
        
        switch (reportType) {
            case 0x00: // Input report (rumble)
                if (length >= 8) {
                    handleRumble(data, length);
                }
                break;
                
            case 0x01: // Feature report
                processFeatureReport(data, length);
                break;
                
            default:
                // Unknown report type
                break;
        }
    }

    return true;
}

void XInputProtocolDriver::convertGamepadToXInput(Gamepad* gamepad) {
    // Convert buttons
    xinputReport.buttons1 = 0;
    xinputReport.buttons2 = 0;

    if (gamepad->pressedDpad(GAMEPAD_MASK_UP))    xinputReport.buttons1 |= XBOX_MASK_UP;
    if (gamepad->pressedDpad(GAMEPAD_MASK_DOWN))  xinputReport.buttons1 |= XBOX_MASK_DOWN;
    if (gamepad->pressedDpad(GAMEPAD_MASK_LEFT))  xinputReport.buttons1 |= XBOX_MASK_LEFT;
    if (gamepad->pressedDpad(GAMEPAD_MASK_RIGHT)) xinputReport.buttons1 |= XBOX_MASK_RIGHT;
    
    if (gamepad->pressedButton(GAMEPAD_MASK_B1)) xinputReport.buttons2 |= XBOX_MASK_A;
    if (gamepad->pressedButton(GAMEPAD_MASK_B2)) xinputReport.buttons2 |= XBOX_MASK_B;
    if (gamepad->pressedButton(GAMEPAD_MASK_B3)) xinputReport.buttons2 |= XBOX_MASK_X;
    if (gamepad->pressedButton(GAMEPAD_MASK_B4)) xinputReport.buttons2 |= XBOX_MASK_Y;
    
    if (gamepad->pressedButton(GAMEPAD_MASK_L1)) xinputReport.buttons2 |= XBOX_MASK_LB;
    if (gamepad->pressedButton(GAMEPAD_MASK_R1)) xinputReport.buttons2 |= XBOX_MASK_RB;
    
    if (gamepad->pressedButton(GAMEPAD_MASK_S1)) xinputReport.buttons1 |= XBOX_MASK_BACK;
    if (gamepad->pressedButton(GAMEPAD_MASK_S2)) xinputReport.buttons1 |= XBOX_MASK_START;
    
    if (gamepad->pressedButton(GAMEPAD_MASK_L3)) xinputReport.buttons1 |= XBOX_MASK_LS;
    if (gamepad->pressedButton(GAMEPAD_MASK_R3)) xinputReport.buttons1 |= XBOX_MASK_RS;
    
    if (gamepad->pressedButton(GAMEPAD_MASK_A1)) xinputReport.buttons2 |= XBOX_MASK_HOME;

    // Convert triggers
    xinputReport.lt = gamepad->pressedButton(GAMEPAD_MASK_L2) ? 0xFF : 0;
    xinputReport.rt = gamepad->pressedButton(GAMEPAD_MASK_R2) ? 0xFF : 0;

    // Convert analog sticks
    xinputReport.lx = gamepad->state.lx;
    xinputReport.ly = gamepad->state.ly;
    xinputReport.rx = gamepad->state.rx;
    xinputReport.ry = gamepad->state.ry;
}

void XInputProtocolDriver::processFeatureReport(const uint8_t* data, size_t length) {
    if (length < 2) return;

    uint8_t featureId = data[1];
    
    switch (featureId) {
        case 0x01: // LED pattern
            if (length >= 3) {
                setPlayerLED(data[2]);
            }
            break;
            
        case 0x02: // Authentication request
            authenticationInProgress = true;
            authTimer = 0;
            break;
            
        default:
            break;
    }
}

void XInputProtocolDriver::handleRumble(const uint8_t* data, size_t length) {
    if (length >= 8) {
        // XInput rumble format: [report_id, size, motor_left, motor_right, ...]
        uint8_t leftMotor = data[2];
        uint8_t rightMotor = data[3];
        
        // Store rumble data for potential use by gamepad addons
        rumbleBuffer[0] = leftMotor;
        rumbleBuffer[1] = rightMotor;
        
        // TODO: Forward rumble data to gamepad or addon system
        // This would depend on how the main system handles force feedback
    }
}

void XInputProtocolDriver::setPlayerLED(uint8_t pattern) {
    playerLED = pattern;
    
    // TODO: Forward LED pattern to LED addon or system
    // This would depend on how the main system handles player LEDs
}

bool XInputProtocolDriver::handleVendorControlRequest(uint8_t stage, const void* request) {
    // Handle XInput-specific vendor control requests
    // This would be called by a USB transport that receives vendor control transfers
    
    // TODO: Implement XInput vendor control handling
    // This includes things like authentication challenges, security responses, etc.
    
    return false;
}

size_t XInputProtocolDriver::getPreferredTransports(TransportType* transports, size_t maxTransports) const {
    if (!transports || maxTransports == 0) {
        return 0;
    }
    
    // XInput prefers USB first, then Bluetooth
    size_t count = 0;
    
    if (count < maxTransports) {
        transports[count++] = TransportType::USB;
    }
    
    if (count < maxTransports) {
        transports[count++] = TransportType::BLUETOOTH;
    }
    
    return count;
}

bool XInputProtocolDriver::supportsTransport(TransportType transportType) const {
    switch (transportType) {
        case TransportType::USB:
        case TransportType::BLUETOOTH:
            return true;
        case TransportType::GPIO:
            return false;
    }
    return false; // Should never reach here with the enum
}
