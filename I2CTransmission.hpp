
#pragma once


#include "Buffer.hpp"
#include "EventObject.hpp"
#include "I2C.hpp"
#include "I2CTransmissionChain.hpp"


class I2CTransmission : public EventObject
{

    EVENT_OBJECT_SIGNAL(I2CTransmission, failed);
    EVENT_OBJECT_SIGNAL(I2CTransmission, finished);


    friend class I2C;
    friend class I2CTransmissionChain;


    I2CTransmission *mPrev;
    I2CTransmission *mNext;

    I2CTransmission *mChainPrev;
    I2CTransmission *mChainNext;


public:

    explicit I2CTransmission(unsigned char address, const Buffer &writeBuffer, 
            Buffer &readBuffer = nullptr);


    unsigned char address() const;

};
