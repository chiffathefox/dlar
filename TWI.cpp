
#include "Debug.hpp"

#include "TWI.hpp"


TWI TWI::sInstance(1);


#ifdef STM32_MCU_SERIES


unsigned char TWI::process(unsigned char stop)
{
    //debugLog() << (mDevice->state == I2C_STATE_IDLE);
    int32_t res = i2c_master_xfer(mDevice, &itc_msg, 1, mTimeout   );
    //debugLog() << (mDevice->state == I2C_STATE_IDLE) <<  res;


    if(res==I2C_ERROR_TIMEOUT)
    {
        debugInfo()<<"TIMEOUT&&Try reset i2c";
        i2c_disable(mDevice);
        i2c_master_enable(mDevice, mFlags);
    }

    
    if (res == -1) {
        //debugInfo()<<"ERROR -1";
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
    }

    return res;
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
