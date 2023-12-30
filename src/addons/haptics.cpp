#include "addons/haptics.h"
#include "DRV2605L.h"
#include "storagemanager.h"

bool HapticFeedback::available()
{
	// Implement the available() method
	// For example, you might check if the DRV2605L device is connected and ready
	return (true && PeripheralManager::getInstance().isI2CEnabled(0));
}

void HapticFeedback::setup()
{
	PeripheralI2C *i2c = PeripheralManager::getInstance().getI2C(0);
	drv2605l = new DRV2605L(i2c, 0x5A);
	printf("haptic setup\r\n");
	drv2605l->begin();
}

void HapticFeedback::process()
{
	Gamepad *gamepad = Storage::getInstance().GetGamepad();
	InputMode inputMode = static_cast<InputMode>(gamepad->getOptions().inputMode);
	if (inputMode == INPUT_MODE_XINPUT)
	{
		uint8_t *featureData = Storage::getInstance().GetFeatureData();
		if (featureData[0] == 0x00)
		{
			drv2605l->setRealtimeValue(featureData[3]);
		}
	}
}
