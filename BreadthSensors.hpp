
#pragma once


#include "EventEmitter.hpp"


class BreadthSensors : public EventEmitter
{

    EVENT_EMITTER_SIGNAL(BreadthSensors, ready);


    const float mWidth;
    const float mLength;


public:

    explicit BreadthSensors(float width, float length);

    virtual float width() const;
    virtual float length() const;

    virtual float front() const = 0;
    virtual float frontLeft() const = 0;
    virtual float frontRight() const = 0;
    virtual float rearLeft() const = 0;
    virtual float rearRight() const = 0;

};
