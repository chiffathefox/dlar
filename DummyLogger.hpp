
#pragma once


#include "Logger.hpp"


class DummyLogger : public Logger
{


public:

    inline virtual Logger & operator<<(char value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(unsigned char value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(int value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(unsigned int value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(long value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(unsigned long value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(double value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(bool value) override
    {
        return *this;
    }


    inline virtual Logger & operator<<(const char *value) override
    {
        return *this;
    }


};
