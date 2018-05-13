
#pragma once


#include "Logger.hpp"


class DummyLogger : public Logger
{


public:

    inline Logger & operator<<(int value) override
    {

    }


    inline Logger & operator<<(float value) override
    {

    }


    inline Logger & operator<<(double value) override
    {

    }


    inline Logger & operator<<(bool value) override
    {

    }


    inline Logger & operator<<(unsigned long value) override
    {

    }


    inline Logger & operator<<(size_t value) override
    {

    }


    inline Logger & operator<<(const char *value) override
    {

    }


};
