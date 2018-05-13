
#pragma once


#include "EventObject.hpp"
#include "Timer.hpp"


class Performance : public EventObject
{

    EVENT_OBJECT_SLOT(Performance, onLoop);
    EVENT_OBJECT_SLOT(Performance, onTimerExpired);


    static const unsigned char sTickersSize;


    Timer mTimer;
    unsigned long mCounter;
    unsigned char mLastTicker;


public:

    class Ticker
    {
        unsigned long mCounter;


        friend void Performance::onTimerExpired();

        
    public:

        inline explicit Ticker()
            : mCounter(0)
        {

        }

        inline void tick()
        {
            mCounter++;
        }


    };


    explicit Performance(unsigned long timeFrame = 10000);

    Ticker *createTicker();


private:

    Ticker *mTickers;

};
