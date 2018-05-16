
#pragma once


#include "EventObject.hpp"


class RangeSensor : public EventObject
{

    EVENT_OBJECT_SIGNAL(VL53L0XAsync, initFailed);
    EVENT_OBJECT_SIGNAL(VL53L0XAsync, initFinished);
    EVENT_OBJECT_SIGNAL(RangeSensor, rangeError);
    EVENT_OBJECT_SIGNAL(RangeSensor, rangeReady);


public:

    virtual void start() = 0;
    virtual void reinit() = 0;
    virtual float range() const = 0;
    virtual float delta() const = 0;
    virtual float maximum() const = 0;

};
