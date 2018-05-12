
#include "Arduino.h"

#include "Debug.hpp"

#include "Performance.hpp"


void Performance::loopEvent()
{
    unsigned long time = millis();

    if (time < mTimeStart) {
        mTimeStart = time;
        mCounter = 0;

        return;
    }

    mCounter++;

    unsigned long diff = time - mTimeStart;

    if (diff >= mTimeFrame) {
        debugLog() << (mCounter * 1000 / diff) << "FPS";

        mTimeStart = time;
        mCounter = 0;
    }
}


Performance::Performance(unsigned long timeFrame)
    : mTimeFrame(timeFrame),
    mCounter(0),
    mTimeStart(millis())
{
    subscribe(Loop);
}
