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
	PeripheralI2C *i2c = PeripheralManager::getInstance().getI2C(0);
drv2605l = new DRV2605L(i2c,0x5A)
drv2605l->begin();
}

void HapticFeedback::process()
{

}
