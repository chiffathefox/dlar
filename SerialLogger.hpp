
#pragma once


#include "Arduino.h"

#include "Logger.hpp"


class SerialLogger : public Logger
{

    static Stream *sSerial;


    const char *mSuffix;


public:

    static inline void setSerial(HardwareSerial *value)
    {
        sSerial = value;
    }


    explicit SerialLogger(const char *level, const char *tag,
            const char *suffix = "\n");
    ~SerialLogger();

    virtual Logger & operator<<(char value) override;
    virtual Logger & operator<<(unsigned char value) override;
    virtual Logger & operator<<(int value) override;
    virtual Logger & operator<<(unsigned int value) override;
    virtual Logger & operator<<(long value) override;
    virtual Logger & operator<<(unsigned long value) override;
    virtual Logger & operator<<(double value) override;
    virtual Logger & operator<<(bool value) override;
    virtual Logger & operator<<(const char *value) override;


};
