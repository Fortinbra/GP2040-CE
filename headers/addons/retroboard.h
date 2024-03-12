#ifndef _RETROBOARD_H
#define _RETROBOARD_H

#include "gpaddon.h"
#include "peripheralmanager.h"
#include "gamepadstate.h"

#ifndef RETROBOARD_ENABLED
#define RETROBOARD_ENABLED 0
#endif

#ifndef RETROBOARD_SDA_PIN
#define RETROBOARD_SDA_PIN -1
#endif

#ifndef RETROBOARD_SCL_PIN
#define RETROBOARD_SCL_PIN -1
#endif

#ifndef RETROBOARD_ADDRESS
#define RETROBOARD_ADDRESS 0x17
#endif

#define RetroBoardName "RetroBoard";

class RetroBoard : public GPAddon
{
public:
	virtual bool available();
	virtual void setup();
	virtual void process();
	virtual void preprocess() {}
	virtual std::string name() { return RetroBoardName; }

private:
	GamepadState *state;
	PeripheralI2C *i2cController;
}
#endif
