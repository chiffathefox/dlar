
#pragma once


class Logger
{


public:

    virtual Logger & operator<<(int value) = 0;
    virtual Logger & operator<<(float value) = 0;
    virtual Logger & operator<<(double value) = 0;
    virtual Logger & operator<<(bool value) = 0;
    virtual Logger & operator<<(unsigned long value) = 0;
    virtual Logger & operator<<(size_t value) = 0;
    virtual Logger & operator<<(const char *value) = 0;

};
