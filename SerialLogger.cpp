
#include "Arduino.h"

#include "SerialLogger.hpp"


SerialLogger::SerialLogger(const char *level, const char *tag,
        const char *suffix)
    : Logger(),
    mSuffix(suffix)
{
    Serial.write(level);
    Serial.write(": ");
    Serial.write(tag);
    Serial.write(":");
}


SerialLogger::~SerialLogger()
{
    Serial.write(mSuffix);
}


Logger & SerialLogger::operator<<(int value)
{
    Serial.write(" ");
    Serial.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(float value)
{
    Serial.write(" ");
    Serial.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(double value)
{
    Serial.write(" ");
    Serial.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(bool value)
{
    Serial.write(" ");
    Serial.print(value ? "true" : "false");

    return *this;
}


Logger & SerialLogger::operator<<(unsigned long value)
{
    Serial.write(" ");
    Serial.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(size_t value)
{
    Serial.write(" ");
    Serial.print(value);

    return *this;
}


Logger & SerialLogger::operator<<(const char *value)
{
    Serial.write(" ");
    Serial.print(value);

    return *this;
}
