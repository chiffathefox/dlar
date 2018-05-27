
#include "Debug.hpp"

#include "TWI.hpp"


TWI TWI::sInstance(1);


#ifdef STM32_MCU_SERIES


unsigned char TWI::process(unsigned char stop)
{
    int32_t res = i2c_master_xfer(mDevice, &itc_msg, 1, mTimeout);

    switch (res) {
    
       
    case I2C_ERROR_TIMEOUT:

        debugWarn() << "I2C_ERROR_TIMEOUT";

        i2c_disable(mDevice);
        i2c_master_enable(mDevice, mFlags);

        return StatusError;


    case I2C_ERROR_PROTOCOL:

        debugWarn() << "I2C_PROTOCOL_ERROR";

        if (mDevice->error_flags & I2C_SR1_AF) {
            res = (mDevice->error_flags & I2C_SR1_ADDR ? ENACKADDR : 
                                                          ENACKTRNS);
        } else if (mDevice->error_flags & I2C_SR1_OVR) {
            res = EDATA;
        } else {
            res = EOTHER;
        }

        i2c_disable(mDevice);
        i2c_master_enable(mDevice, mFlags);

        return StatusError;


    }

    return StatusSuccess;
}


unsigned char TWI::process()
{
    process(true);
}


TWI::TWI(unsigned char device, unsigned char flags)
    : TwoWire(device, flags),
    mTimeout(20)
{
    if (device == 1) {
        mDevice = I2C1;
    } else if (device == 2) {
        mDevice = I2C1;
    } else {
        debugAssert(false);
    }
}


#else


TWI::TWI(unsigned char device, unsigned char flags)
    : TwoWire(),
    mTimeout(0)
{

}


#endif
