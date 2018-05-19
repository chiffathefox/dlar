
#include "Debug.hpp"

#include "TWI.hpp"


TWI TWI::sInstance(1);


#ifdef STM32_MCU_SERIES


unsigned char TWI::process(unsigned char stop)
{
    char res = i2c_master_xfer(mDevice, &itc_msg, 1, mTimeout);

    if (res == I2C_ERROR_PROTOCOL) {
        if (mDevice->error_flags & I2C_SR1_AF) { /* NACK */
            res = (mDevice->error_flags & I2C_SR1_ADDR ? ENACKADDR : 
                                                          ENACKTRNS);
        } else if (mDevice->error_flags & I2C_SR1_OVR) { /* Over/Underrun */
            res = EDATA;
        } else { /* Bus or Arbitration error */
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
