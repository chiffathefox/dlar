
#include "Arduino.h"

#include "Debug.hpp"
#include "Application.hpp"

#include "MovementController.hpp"


void MovementController::onLoop()
{
    updateDirection();
}


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
    unsigned long lastEmitted = Application::instance()->loop()->lastEmitted();
    unsigned long timeDelta = lastEmitted > time ? 0 : lastEmitted - time;
    Vector2f diff = (mTargetDirection - direction) / (targetTime() - time);
    
    direction += diff * timeDelta;

    if ((direction - mTargetDirection).dot(diff) < 0) {
        direction = mTargetDirection;
    }

    setDirection(direction);
}


MovementController::MovementController()
    : EventObject()
{
    EventObjectConnect(Application::instance(), loop, this, onLoop);
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
