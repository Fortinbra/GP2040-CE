#ifndef _Haptics_H
#define _Haptics_H

#include "DRV2605L.h"
#include "gpaddon.h"
#include "GamepadEnums.h"
#include "peripheralmanager.h"

class HapticFeedback : public GPAddon
{
public:
	virtual bool available();
	virtual void setup();
	virtual void process();
	virtual void preprocess() {}
	virtual std::string name() { return "HapticFeedback"; }

private:
	DRV2605L *drv2605l;
};
#endif