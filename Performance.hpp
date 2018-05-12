
#pragma once


#include "EventEmitter.hpp"
#include "Timer.hpp"


class Performance : public EventEmitter
{

    EVENT_EMITTER_SLOT(Performance, onLoop);
    EVENT_EMITTER_SLOT(Performance, onTimerExpired);


    Timer mTimer;
    unsigned long mCounter;


public:

    explicit Performance(unsigned long timeFrame = 10000);

};
