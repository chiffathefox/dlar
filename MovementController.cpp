
#include "Arduino.h"

#include "Debug.hpp"
#include "MovementController.hpp"


unsigned long MovementController::targetTime() const
{
    unsigned long time = millis();

    return mTargetTime <= time ? time + 1 : mTargetTime;
}


void MovementController::updateDirection()
{
    Vector2f direction = this->direction();

    if (direction == mTargetDirection) {
        return;
    }

    unsigned long time = millis();
    unsigned long lastEmited = Event::lastEmited(Loop);
    unsigned long timeDelta = lastEmited > time ? 0 : lastEmited - time;
    Vector2f diff = (mTargetDirection - direction) / (targetTime() - time);
    
    direction += diff * timeDelta;

    if ((direction - mTargetDirection).dot(diff) < 0) {
        direction = mTargetDirection;
    }

    setDirection(direction);
}


void MovementController::loopEvent()
{
    Event::loopEvent();

    updateDirection();
}


MovementController::MovementController()
    : Event()
{
    subscribe(Loop);
}


Vector2f MovementController::direction() const
{
    return mDirection;
}


void MovementController::setDirection(const Vector2f &value)
{
    debugAssert(value.sqrMagnitude() <= 1);

    mDirection = value;
    mTargetDirection = value;
    mTargetTime = millis();
}


void MovementController::lerpDirectionTo(const Vector2f &value,
        unsigned long time)
{
    debugAssert(value.sqrMagnitude() <= 1);

    mTargetDirection = value;
    mTargetTime = millis() + time;

    updateDirection();
}
