#ifndef _MAX17048DEVICE_H_
#define _MAX17048DEVICE_H_

#include <vector>

#include "i2cdevicebase.h"
#include "MAX17048.h"

class MAX17048Device : public MAX17048, public I2CDeviceBase {
    public:
        // Constructor
        MAX17048Device() {}
        MAX17048Device(PeripheralI2C *i2cController, uint8_t addr = MAX17048_I2CADDR_DEFAULT) 
            : MAX17048(i2cController, addr) {}

        std::vector<uint8_t> getDeviceAddresses() const override {
            return {MAX17048_I2CADDR_DEFAULT}; // MAX17048 only has one possible address
        }
};

#endif
