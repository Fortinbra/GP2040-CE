/*
 * SPDX-License-Identifier: MIT
 * SPDX-FileCopyrightText: Copyright (c) 2024 OpenStickCommunity (gp2040-ce.info)
 */

#include "interfaces/gpiotransport.h"
#include "pico/time.h"
#include "hardware/clocks.h"
#include <cstring>
#include <algorithm>

GPIOTransport::GPIOTransport() : 
    lastPinStates(0),
    initialized(false) {
    memset(debounceStates, 0, sizeof(debounceStates));
    memset(lastDebounceTime, 0, sizeof(lastDebounceTime));
}

GPIOTransport::~GPIOTransport() {
    deinitialize();
}

bool GPIOTransport::initialize() {
    if (initialized) {
        return true;
    }
    
    initializeGPIO();
    initialized = true;
    
    return true;
}

void GPIOTransport::deinitialize() {
    if (!initialized) {
        return;
    }
    
    cleanupGPIO();
    configuredPins.clear();
    stateChangeCallback = nullptr;
    initialized = false;
}

bool GPIOTransport::isReady() {
    return initialized;
}

int GPIOTransport::send(const uint8_t* data, size_t length) {
    if (!initialized || !data || length == 0) {
        return -1;
    }
    
    // For GPIO transport, "sending" means setting output pins based on data
    // This is protocol-specific, but we provide a generic implementation
    size_t bytesToProcess = std::min(length, size_t(4)); // Limit to 32 pins
    
    for (size_t i = 0; i < bytesToProcess; i++) {
        uint8_t byte = data[i];
        for (int bit = 0; bit < 8; bit++) {
            uint8_t pin = i * 8 + bit;
            if (isPinConfigured(pin)) {
                bool state = (byte & (1 << bit)) != 0;
                setPin(pin, state);
            }
        }
    }
    
    return bytesToProcess;
}

int GPIOTransport::receive(uint8_t* buffer, size_t maxLength) {
    if (!initialized || !buffer || maxLength == 0) {
        return -1;
    }
    
    // For GPIO transport, "receiving" means reading input pins
    size_t bytesToRead = std::min(maxLength, size_t(4)); // Limit to 32 pins
    
    for (size_t i = 0; i < bytesToRead; i++) {
        uint8_t byte = 0;
        for (int bit = 0; bit < 8; bit++) {
            uint8_t pin = i * 8 + bit;
            if (isPinConfigured(pin) && getPin(pin)) {
                byte |= (1 << bit);
            }
        }
        buffer[i] = byte;
    }
    
    return bytesToRead;
}

void GPIOTransport::process() {
    if (!initialized) {
        return;
    }
    
    processDebouncing();
    
    // Check for state changes and call callback if registered
    if (stateChangeCallback) {
        uint32_t currentStates = 0;
        for (const auto& pinConfig : configuredPins) {
            if (getPin(pinConfig.pin)) {
                currentStates |= (1 << pinConfig.pin);
            }
        }
        
        if (currentStates != lastPinStates) {
            uint32_t changedPins = currentStates ^ lastPinStates;
            stateChangeCallback(changedPins, currentStates);
            lastPinStates = currentStates;
        }
    }
}

bool GPIOTransport::configurePins(const std::vector<GPIOPinConfig>& pins) {
    configuredPins = pins;
    
    for (const auto& pinConfig : configuredPins) {
        gpio_init(pinConfig.pin);
        gpio_set_function(pinConfig.pin, GPIO_FUNC_SIO);  // Default to GPIO function
        
        // Configure as input or output based on usage
        gpio_set_dir(pinConfig.pin, false); // Default to input
        
        if (pinConfig.pullUp) {
            gpio_pull_up(pinConfig.pin);
        } else if (pinConfig.pullDown) {
            gpio_pull_down(pinConfig.pin);
        } else {
            gpio_disable_pulls(pinConfig.pin);
        }
    }
    
    return true;
}

bool GPIOTransport::setPin(uint8_t pin, bool state) {
    if (!isPinConfigured(pin)) {
        return false;
    }
    
    gpio_set_dir(pin, true); // Set as output
    gpio_put(pin, state);
    return true;
}

bool GPIOTransport::getPin(uint8_t pin) {
    if (!isPinConfigured(pin)) {
        return false;
    }
    
    return gpio_get(pin);
}

void GPIOTransport::setPins(uint32_t mask, uint32_t values) {
    for (int pin = 0; pin < 32; pin++) {
        if (mask & (1 << pin)) {
            bool state = (values & (1 << pin)) != 0;
            setPin(pin, state);
        }
    }
}

uint32_t GPIOTransport::getPins(uint32_t mask) {
    uint32_t result = 0;
    for (int pin = 0; pin < 32; pin++) {
        if (mask & (1 << pin)) {
            if (getPin(pin)) {
                result |= (1 << pin);
            }
        }
    }
    return result;
}

void GPIOTransport::registerCallback(std::function<void(uint32_t pins, uint32_t values)> callback) {
    stateChangeCallback = callback;
}

bool GPIOTransport::enablePWM(uint8_t pin, uint32_t frequency, float dutyCycle) {
    if (!isPinConfigured(pin)) {
        return false;
    }
    
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    
    // Calculate divisor and wrap values
    uint32_t clock_freq = clock_get_hz(clk_sys);
    uint32_t divider = clock_freq / (frequency * 65536);
    if (divider < 1) divider = 1;
    if (divider > 255) divider = 255;
    
    uint16_t wrap = (clock_freq / (frequency * divider)) - 1;
    uint16_t level = (uint16_t)(wrap * dutyCycle);
    
    pwm_set_clkdiv(slice, divider);
    pwm_set_wrap(slice, wrap);
    pwm_set_gpio_level(pin, level);
    pwm_set_enabled(slice, true);
    
    return true;
}

void GPIOTransport::disablePWM(uint8_t pin) {
    if (!isPinConfigured(pin)) {
        return;
    }
    
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_enabled(slice, false);
    gpio_set_function(pin, GPIO_FUNC_SIO);
}

void GPIOTransport::initializeGPIO() {
    // GPIO initialization is handled in configurePins()
}

void GPIOTransport::cleanupGPIO() {
    for (const auto& pinConfig : configuredPins) {
        gpio_deinit(pinConfig.pin);
    }
}

void GPIOTransport::processDebouncing() {
    uint32_t currentTime = time_us_32();
    
    for (const auto& pinConfig : configuredPins) {
        if (pinConfig.debounceMs == 0) {
            continue; // No debouncing for this pin
        }
        
        uint8_t pin = pinConfig.pin;
        bool currentState = gpio_get(pin);
        uint32_t stateChanged = (currentState != ((debounceStates[pin] & 1) != 0));
        
        if (stateChanged) {
            lastDebounceTime[pin] = currentTime;
        }
        
        if ((currentTime - lastDebounceTime[pin]) > (pinConfig.debounceMs * 1000)) {
            if (currentState != ((debounceStates[pin] & 2) != 0)) {
                debounceStates[pin] = (debounceStates[pin] & ~2) | (currentState ? 2 : 0);
            }
        }
        
        debounceStates[pin] = (debounceStates[pin] & ~1) | (currentState ? 1 : 0);
    }
}

bool GPIOTransport::isPinConfigured(uint8_t pin) const {
    return std::any_of(configuredPins.begin(), configuredPins.end(),
                      [pin](const GPIOPinConfig& config) { return config.pin == pin; });
}
