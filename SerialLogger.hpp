
#pragma once


#include "Logger.hpp"


class SerialLogger : public Logger
{

    const char *mSuffix;


public:

    explicit SerialLogger(const char *level, const char *tag,
            const char *suffix = "\n");
    ~SerialLogger();

    virtual Logger & operator<<(int value) override;
    virtual Logger & operator<<(float value) override;
    virtual Logger & operator<<(double value) override;
    virtual Logger & operator<<(bool value) override;
    virtual Logger & operator<<(unsigned long value) override;
    virtual Logger & operator<<(size_t value) override;
    virtual Logger & operator<<(const char *value) override;

};
