/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#ifndef _GPIOTRANSPORT_H_
#define _GPIOTRANSPORT_H_

#include "interfaces/transportinterface.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/types.h"
#include <vector>
#include <functional>

/**
 * @brief GPIO pin configuration
 */
struct GPIOPinConfig {
    uint8_t pin;
    bool pullUp;
    bool pullDown;
    uint32_t debounceMs;
};

/**
 * @brief GPIO transport implementation for direct GPIO communication
 * 
 * This transport is used for protocols that communicate directly through GPIO pins,
 * such as original console protocols (SNES, Saturn, etc.) or arcade systems.
 */
class GPIOTransport : public TransportInterface {
public:
    GPIOTransport();
    virtual ~GPIOTransport();

    // TransportInterface implementation
    bool initialize() override;
    void deinitialize() override;
    bool isReady() override;
    int send(const uint8_t* data, size_t length) override;
    int receive(uint8_t* buffer, size_t maxLength) override;
    void process() override;
    TransportType getType() const override { return TransportType::GPIO; }
    size_t getMTU() const override { return 32; } // GPIO packet size

    // GPIO-specific methods
    
    /**
     * @brief Configure GPIO pins for this transport
     * @param pins Vector of pin configurations
     * @return true if configuration was successful
     */
    bool configurePins(const std::vector<GPIOPinConfig>& pins);
    
    /**
     * @brief Set a GPIO pin state
     * @param pin GPIO pin number
     * @param state Pin state (true = high, false = low)
     * @return true if successful
     */
    bool setPin(uint8_t pin, bool state);
    
    /**
     * @brief Get a GPIO pin state
     * @param pin GPIO pin number
     * @return Pin state (true = high, false = low)
     */
    bool getPin(uint8_t pin);
    
    /**
     * @brief Set multiple pins at once using a bitmask
     * @param mask Bitmask of pins to set
     * @param values Values for the pins (bit 0 = pin 0, etc.)
     */
    void setPins(uint32_t mask, uint32_t values);
    
    /**
     * @brief Get multiple pin states as a bitmask
     * @param mask Bitmask of pins to read
     * @return Pin states as bitmask
     */
    uint32_t getPins(uint32_t mask);
    
    /**
     * @brief Register a callback for GPIO state changes
     * @param callback Function to call when GPIO state changes
     */
    void registerCallback(std::function<void(uint32_t pins, uint32_t values)> callback);
    
    /**
     * @brief Enable PWM on a specific pin
     * @param pin GPIO pin number
     * @param frequency PWM frequency in Hz
     * @param dutyCycle Duty cycle (0.0 to 1.0)
     * @return true if successful
     */
    bool enablePWM(uint8_t pin, uint32_t frequency, float dutyCycle);
    
    /**
     * @brief Disable PWM on a specific pin
     * @param pin GPIO pin number
     */
    void disablePWM(uint8_t pin);

private:
    std::vector<GPIOPinConfig> configuredPins;
    std::function<void(uint32_t, uint32_t)> stateChangeCallback;
    uint32_t lastPinStates;
    uint32_t debounceStates[32];
    uint32_t lastDebounceTime[32];
    bool initialized;
    
    /**
     * @brief Initialize GPIO hardware
     */
    void initializeGPIO();
    
    /**
     * @brief Cleanup GPIO configuration
     */
    void cleanupGPIO();
    
    /**
     * @brief Process pin debouncing
     */
    void processDebouncing();
    
    /**
     * @brief Check if a pin is configured
     * @param pin GPIO pin number
     * @return true if configured
     */
    bool isPinConfigured(uint8_t pin) const;
};

#endif // _GPIOTRANSPORT_H_
