
#include "Arduino.h"

#include "Debug.hpp"
#include "Application.hpp"

#include "Performance.hpp"


void Performance::onLoop()
{
    mCounter++;
}


void Performance::onTimerExpired()
{
    debugLog() << (mCounter * 1000 / (millis() - mTimer.startTime()))
               << "FPS";

    mCounter = 0;
}


Performance::Performance(unsigned long timeFrame)
    : EventEmitter(),
    mTimer(timeFrame),
    mCounter(0)
{
    EventEmitterConnect(Application::instance(), loop, this, onLoop);
    EventEmitterConnect(&mTimer, expired, this, onTimerExpired);

    mTimer.start();
}
