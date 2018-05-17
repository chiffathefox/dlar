
#include "Arduino.h"

#include "SerialLogger.hpp"


SerialLogger::SerialLogger(const char *level, const char *tag,
        const char *suffix)
    : Logger(),
    mSuffix(suffix)
{
    Serial1.write(level);
    Serial1.write(":");
    Serial1.write(tag);
    Serial1.write(":");
}


SerialLogger::~SerialLogger()
{
    Serial1.write(mSuffix);
}


Logger & SerialLogger::operator<<(int value)
{
    Serial1.write(" ");
    Serial1.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(float value)
{
    Serial1.write(" ");
    Serial1.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(double value)
{
    Serial1.write(" ");
    Serial1.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(bool value)
{
    Serial1.write(" ");
    Serial1.print(value ? "true" : "false");

    return *this;
}


Logger & SerialLogger::operator<<(unsigned long value)
{
    Serial1.write(" ");
    Serial1.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(size_t value)
{
    Serial1.write(" ");
    Serial1.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(const char *value)
{
    Serial1.write(" ");
    Serial1.print(value);

    return *this;
}
