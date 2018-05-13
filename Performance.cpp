
#include "Arduino.h"

#include "Debug.hpp"
#include "Application.hpp"

#include "Performance.hpp"


const unsigned char Performance::sTickersSize = 5;


void Performance::onLoop()
{
    mCounter++;
}


void Performance::onTimerExpired()
{
    unsigned long diff = millis() - mTimer.startTime();

    debugLog() << (mCounter * 1000 / diff) << "FPS";

    for (unsigned char i = 0; i < mLastTicker; i++) {
        Ticker &ticker = mTickers[i];

        debugLog() << "Ticker" << i << "is at"
                   << (ticker.mCounter * 1000 / diff)
                   << "FPS"; 
        ticker.mCounter = 0;
    }

    mCounter = 0;
}


Performance::Performance(unsigned long timeFrame)
    : EventObject(),
    mTimer(timeFrame),
    mCounter(0),
    mTickers(new Ticker[sTickersSize]),
    mLastTicker(0)
{
    EventObjectConnect(Application::instance(), loop, this, onLoop);
    EventObjectConnect(&mTimer, expired, this, onTimerExpired);

    mTimer.start();
}


Performance::Ticker *Performance::createTicker()
{
    debugAssert(mLastTicker < sTickersSize);

    return &mTickers[mLastTicker++];
}
