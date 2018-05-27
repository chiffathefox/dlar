
#pragma once


#include "EventObject.hpp"
#include "I2CTransmission.hpp"


class I2C : public EventObject
{

    static I2C sInstance;
    static I2CTransmission sFree;


    I2CTransmission &mTransmission;


public:

    static I2C &instance();
    static I2CTransmission &createTransmission(unsigned char address,
            const Buffer &writeBuffer, Buffer &readBuffer = nullptr);
    static void releaseTransmission(I2CTransmission &transmission);


    void enqueue(I2CTransmission &transmission);

};
