
#pragma once


#include "EventObject.hpp"
#include "Vector2f.hpp"


class MovementController : public EventObject
{

    EVENT_OBJECT_SLOT(MovementController, onLoop);


    Vector2f mDirection;
    Vector2f mTargetDirection;

    unsigned long mTargetTime;


    unsigned long targetTime() const;

    void updateDirection();


public:

    explicit MovementController();

    virtual Vector2f direction() const;
    virtual void setDirection(const Vector2f &value);
    virtual void lerpDirectionTo(const Vector2f &value, unsigned long time = 0);

};
