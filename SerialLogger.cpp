
#include "Arduino.h"

#include "SerialLogger.hpp"


Stream *SerialLogger::sSerial = &Serial;


SerialLogger::SerialLogger(const char *level, const char *tag,
        const char *suffix)
    : Logger(),
    mSuffix(suffix)
{
    sSerial->write(level);
    sSerial->write(":");
    sSerial->write(tag);
    sSerial->write(":");
}


SerialLogger::~SerialLogger()
{
    sSerial->write(mSuffix);
}


Logger & SerialLogger::operator<<(char value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(unsigned char value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(int value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(unsigned int value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(long value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(unsigned long value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(double value)
{
    sSerial->write(" ");
    sSerial->print(value);

    return *this;
}


Logger & SerialLogger::operator<<(bool value)
{
    sSerial->write(" ");
    sSerial->write(value ? "true" : "false");

    return *this;
}


Logger & SerialLogger::operator<<(const char *value)
{
    sSerial->write(" ");
    sSerial->write(value);

    return *this;
}
