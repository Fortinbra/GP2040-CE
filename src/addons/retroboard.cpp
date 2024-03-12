#include "addons/retroboard.h"
#include "storagemanager.h"
#include "config.pb.h"


bool RetroBoard::available()
{
	const RetroBoardOptions &options = Storage::getInstance().getAddonOptions().retroBoardOptions;
	return (options.enabled && PeripheralManager::getInstance().isI2CEnabled(options.i2cBlock));
}
bool RetroBoard::setup()
{
	const RetroBoardOptions &options = Storage::getInstance().getAddonOptions().retroBoardOptions;
	PeripheralI2C *i2c = PeripheralManager::getInstance().getI2C(options.i2cBlock);

}
bool RetroBoard::process()
{
	Gamepad * gamepad = Storage::getInstance().GetGamepad();
	i2c
}
