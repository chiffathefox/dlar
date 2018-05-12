
#pragma once


#include "Event.hpp"
#include "Vector2f.hpp"


class MovementController : public Event
{

    Vector2f mDirection;
    Vector2f mTargetDirection;

    unsigned long mTargetTime;


    unsigned long targetTime() const;

    void updateDirection();


protected:

    virtual void loopEvent() override;


public:

    explicit MovementController();

    virtual Vector2f direction() const;
    virtual void setDirection(const Vector2f &value);
    virtual void lerpDirectionTo(const Vector2f &value, unsigned long time = 0);

};
