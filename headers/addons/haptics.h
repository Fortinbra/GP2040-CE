#include "DRV2605L.h"
#include "gpaddon.h"

class HapticFeedback : public GPAddon
{

public:
	virtual bool available();
	virtual void setup();		// Analog Setup
	virtual void process(); // Analog Process
	virtual void preprocess() {}
	virtual std::string name() { return "HapticFeedback"; }

private:
	DRV2605L drv2605l;
};
