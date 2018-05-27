
#pragma once


/* XXX: Don't use both `Wire.h' and `TWI.hpp' bad things will happen */


#include <Wire.h>
#include <stdint.h>


class TWI : public TwoWire
{

    static TWI sInstance;


#ifdef STM32_MCU_SERIES


    i2c_dev *mDevice;
    unsigned char mFlags;
    uint32_t mTimeout;


#endif


protected:

    unsigned char process(unsigned char stop) override;
    unsigned char process() override;


public:


    enum StatusCode : unsigned char
    {
        StatusSuccess = 0,
        StatusError = 4
    };


    inline static TWI *instance()
    {
        return &sInstance;
    }


    explicit TWI(unsigned char device, unsigned char flags = 0);


    inline void setTimeout(uint32_t value)
    {
        mTimeout = value;
    }


    inline uint32_t timeout() const
    {
        return mTimeout;
    }


};
