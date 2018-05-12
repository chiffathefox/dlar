
#pragma once


#include "Event.hpp"


class BreadthSensors : public Event
{

    const float mWidth;
    const float mLength;
    const type_t mValueEventType;


public:

    explicit BreadthSensors(float width, float length);

    virtual float width() const;
    virtual float length() const;
    virtual type_t valueEventType() const;

    virtual float front() const = 0;
    virtual float frontLeft() const = 0;
    virtual float frontRight() const = 0;
    virtual float rearLeft() const = 0;
    virtual float rearRight() const = 0;

};
