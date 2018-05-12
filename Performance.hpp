
#pragma once


#include "Event.hpp"


class Performance : public Event
{

    unsigned long mTimeFrame;
    unsigned long mTimeStart;
    unsigned long mCounter;


protected:

    virtual void loopEvent() override;


public:

    explicit Performance(unsigned long timeFrame = 10000);

};
