#ifndef _WIIExtensionAddon_H
#define _WIIExtensionAddon_H

#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <numeric>
#include <stdint.h>
#include <hardware/i2c.h>
#include "BoardConfig.h"
#include "gpaddon.h"
#include "gamepad.h"
#include "storagemanager.h"
#include "peripheralmanager.h"
#include "wiiextension_dev.h"

// WiiExtension Module Name
#define WiiExtensionName "WiiExtension"

#ifndef WII_EXTENSION_ENABLED
#define WII_EXTENSION_ENABLED 0
#endif

#ifndef WII_EXTENSION_I2C_SDA_PIN
#define WII_EXTENSION_I2C_SDA_PIN -1
#endif

#ifndef WII_EXTENSION_I2C_SCL_PIN
#define WII_EXTENSION_I2C_SCL_PIN -1
#endif

#ifndef WII_EXTENSION_I2C_BLOCK
#define WII_EXTENSION_I2C_BLOCK i2c0
#endif

#ifndef WII_EXTENSION_I2C_SPEED
#define WII_EXTENSION_I2C_SPEED 400000
#endif

#define WII_SET_MASK(bits, check, val) ((check) ? ((bits) |= (val)) : ((bits) &= ~(val)))

typedef enum {
    WII_ANALOG_TYPE_NONE,
    WII_ANALOG_TYPE_LEFT_STICK_X,
    WII_ANALOG_TYPE_LEFT_STICK_Y,
    WII_ANALOG_TYPE_RIGHT_STICK_X,
    WII_ANALOG_TYPE_RIGHT_STICK_Y,
    WII_ANALOG_TYPE_DPAD_X,
    WII_ANALOG_TYPE_DPAD_Y,
    WII_ANALOG_TYPE_LEFT_TRIGGER,
    WII_ANALOG_TYPE_RIGHT_TRIGGER,
    WII_ANALOG_TYPE_LEFT_STICK_X_PLUS,
    WII_ANALOG_TYPE_LEFT_STICK_X_MINUS,
    WII_ANALOG_TYPE_LEFT_STICK_Y_PLUS,
    WII_ANALOG_TYPE_LEFT_STICK_Y_MINUS,
    WII_ANALOG_TYPE_RIGHT_STICK_X_PLUS,
    WII_ANALOG_TYPE_RIGHT_STICK_X_MINUS,
    WII_ANALOG_TYPE_RIGHT_STICK_Y_PLUS,
    WII_ANALOG_TYPE_RIGHT_STICK_Y_MINUS,
    WII_ANALOG_TYPE_COUNT
} WiiAnalogType;

typedef struct {
    uint16_t axisType;
    uint16_t minRange;
    uint16_t maxRange;
} WiiAnalogAxis;

typedef struct {
    // button ID = gamepad mask value
    std::unordered_map<uint16_t, uint32_t> buttonMap;
    std::unordered_map<uint16_t, WiiAnalogAxis> analogMap;
} WiiExtensionConfig;

typedef struct {
    uint16_t analogInput;
    uint16_t analogValue;
} WiiAnalogChange;

class WiiExtensionInput : public GPAddon {
public:
    WiiExtensionInput();
    virtual bool available();
    virtual void setup();       // WiiExtension Setup
    virtual void process();     // WiiExtension Process
    virtual void preprocess() {}
    virtual void postprocess(bool sent) {}
    virtual void reinit() {}
    virtual std::string name() { return WiiExtensionName; }
private:
    WiiExtensionDevice * wii;
    uint32_t uIntervalMS;
    uint32_t nextTimer;

    std::unordered_map<uint16_t, WiiExtensionConfig> extensionConfigs;
    WiiExtensionConfig* currentConfig = NULL;

    bool buttonC = false;
    bool buttonZ = false;

    bool buttonA = false;
    bool buttonB = false;
    bool buttonX = false;
    bool buttonY = false;
    bool buttonL = false;
    bool buttonZL = false;
    bool buttonR = false;
    bool buttonZR = false;

    bool buttonSelect = false;
    bool buttonStart = false;
    bool buttonHome = false;

    bool dpadUp     = false;
    bool dpadDown   = false;
    bool dpadLeft   = false;
    bool dpadRight  = false;

    bool isAnalogTriggers = false;
    bool isGyroscope = false;
    bool isAccelerometer = false;
    bool isTouch = false;

    uint16_t triggerLeft  = 0;
    uint16_t triggerRight = 0;
    uint16_t lastTriggerLeft  = 0;
    uint16_t lastTriggerRight = 0;
    uint16_t whammyBar    = 0;

    uint16_t leftX = 0;
    uint16_t lastLeftX = 0;

    uint16_t leftY = 0;
    uint16_t lastLeftY = 0;

    uint16_t rightX = 0;
    uint16_t lastRightX = 0;

    uint16_t rightY = 0;
    uint16_t lastRightY = 0;

    uint16_t accelerometerX = 0;
    uint16_t accelerometerY = 0;
    uint16_t accelerometerZ = 0;

    uint16_t gyroscopeX = 0;
    uint16_t gyroscopeY = 0;
    uint16_t gyroscopeZ = 0;

    uint16_t touchX = 0;
    uint16_t touchY = 0;
    uint16_t touchZ = 0;
    bool touchPressed = false;

    std::map<uint16_t, std::vector<WiiAnalogChange>> analogChanges;

    uint16_t map(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max);
    uint16_t bounds(uint16_t x, uint16_t out_min, uint16_t out_max);

    void update();
    void setControllerButton(uint16_t controllerID, uint16_t buttonID, uint32_t buttonMask);
    void setControllerAnalog(uint16_t controllerID, uint16_t analogID, uint32_t axisType);
    void setControllerStickMode(uint16_t controllerID, uint16_t analogID, uint32_t axisType);
    void setButtonState(bool buttonState, uint16_t buttonMask);
    void queueAnalogChange(uint16_t analogInput, uint16_t analogValue, uint16_t lastAnalogValue);
    void updateAnalogState();
    void updateMotionState();
    void reloadConfig();

    uint16_t getAverage(std::vector<WiiAnalogChange> const& changes);
    uint16_t getDelta(std::vector<uint16_t> const& changes, uint16_t baseValue);
};

#endif  // _WIIExtensionAddon_H
