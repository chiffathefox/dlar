
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


#if 0


#    include "SerialLogger.hpp"


#    define debugLog() SerialLogger("D", __PRETTY_FUNCTION__)


#    define debugWarn()                                    \
    SerialLogger("\033[33mW", __PRETTY_FUNCTION__, "\033[0m\n")


#    define debugInfo()                                    \
    SerialLogger("\033[32mI", __PRETTY_FUNCTION__, "\033[0m\n")


#elif (DEBUG)


#    include "SerialLogger.hpp"


#    define debugLog() SerialLogger("D", __func__)


#    define debugWarn()                                    \
    SerialLogger("\033[33mW", __func__, "\033[0m\n")


#    define debugInfo()                                    \
    SerialLogger("\033[32mI", __func__, "\033[0m\n")



#else


#    include "DummyLogger.hpp"


#    define debugLog() DummyLogger()


#    define debugWarn() DummyLogger()


#    define debugInfo() DummyLogger()


#endif


#if 0
#include "DummyLogger.hpp"
#undef debugWarn
#undef debugInfo
#define debugWarn() DummyLogger()
#define debugInfo() DummyLogger()
#endif



class Debug
{

    static void disableAll();


public:

    static void panic();
    static void panic(const char *file, int line, const char *message);

};
