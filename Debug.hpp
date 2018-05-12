
#pragma once


#include <stdio.h>

#include "SerialLogger.hpp"


#define debugAssert(condition)                         \
    if (!(condition)) {                                \
        Debug::panic(__FILE__, __func__, __LINE__,     \
                "assertion `" #condition "' failed");  \
    }


#define debugLog() SerialLogger("DEBUG", __PRETTY_FUNCTION__)


#define debugWarn()                                    \
    SerialLogger("\033[33mWARN", __PRETTY_FUNCTION__, "\033[0m\n")


#define debugInfo()                                    \
    SerialLogger("\033[32mINFO", __PRETTY_FUNCTION__, "\033[0m\n")


class Debug {


public:

    static void panic(const char *file, const char *func, int line,
            const char *message);

};
