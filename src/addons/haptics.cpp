#include "addons/haptics.h"
#include "DRV2605L.h"

bool HapticFeedback::available()
{
	// Implement the available() method
	// For example, you might check if the DRV2605L device is connected and ready
	return true;
}

void HapticFeedback::setup()
{
	// Implement the setup() method
	// For example, you might initialize the DRV2605L device
	drv2605l.begin();
}

void HapticFeedback::process()
{
	drv2605l.go();
}
