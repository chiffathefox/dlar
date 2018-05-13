
#pragma once


#include <stdio.h>


#define DEBUG 1
#define DEBUG_ASSERT_NO_CS 1


#if (DEBUG_ASSERT_NO_CS || !DEBUG)


#    define debugAssert(condition)                         \
    if (!(condition)) {                                    \
        Debug::panic();                                    \
    }


#else


#    define debugAssert(condition)                         \
    if (!(condition)) {                                    \
        Debug::panic(__FILE__, __LINE__, #condition);      \
    }


#endif


#if (DEBUG)


#    include "SerialLogger.hpp"


#    define debugLog() SerialLogger("DEBUG", __PRETTY_FUNCTION__)


#    define debugWarn()                                    \
    SerialLogger("\033[33mWARN", __PRETTY_FUNCTION__, "\033[0m\n")


#    define debugInfo()                                    \
    SerialLogger("\033[32mINFO", __PRETTY_FUNCTION__, "\033[0m\n")


#else


#    include "DummyLogger.hpp"


#    define debugLog() DummyLogger()


#    define debugWarn() DummyLogger()


#    define debugInfo() DummyLogger()


#endif


class Debug
{

    static void disableAll();


public:

    static void panic();
    static void panic(const char *file, int line, const char *message);

};
