
#pragma once


#include "Arduino.h"

#include "EventObject.hpp"


class Timer : public EventObject
{

    EVENT_OBJECT_SIGNAL(Timer, expired);
    EVENT_OBJECT_SLOT(Timer, onLoop);


    unsigned long mTimeout;
    unsigned long mStartTime;
    bool mSingleShot;


public:

    explicit Timer(unsigned long timeout = 0, bool singleShot = false);


    inline void start()
    {
        mStartTime = millis();
    }


    inline void stop()
    {
        mStartTime = -1;
    }


    inline unsigned long timeout() const
    {
        return mTimeout;
    }


    inline void setTimeout(unsigned long value)
    {
        mTimeout = value;
    }


    inline bool singleShot() const
    {
        return mSingleShot;
    }


    inline void setSingleShot(bool value)
    {
        mSingleShot = value;
    }


    inline unsigned long startTime() const
    {
        return mStartTime;
    }


    inline bool running() const
    {
        return mStartTime != -1;
    }


};
