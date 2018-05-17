
#pragma once


#include "Vector2f.hpp"


class Logger
{


public:

    virtual Logger & operator<<(char value) = 0;
    virtual Logger & operator<<(unsigned char value) = 0;
    virtual Logger & operator<<(int value) = 0;
    virtual Logger & operator<<(unsigned int value) = 0;
    virtual Logger & operator<<(long value) = 0;
    virtual Logger & operator<<(unsigned long value) = 0;
    virtual Logger & operator<<(double value) = 0;
    virtual Logger & operator<<(bool value) = 0;
    virtual Logger & operator<<(const char *value) = 0;


};
