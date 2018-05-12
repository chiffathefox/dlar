
#pragma once


#include "EventEmitter.hpp"


class Performance : public EventEmitter
{

    EVENT_EMITTER_SLOT(Performance, onLoop);


    unsigned long mTimeFrame;
    unsigned long mTimeStart;
    unsigned long mCounter;


public:

    explicit Performance(unsigned long timeFrame = 10000);

};
