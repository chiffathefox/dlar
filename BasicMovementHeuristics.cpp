
#include <math.h>

#include "BasicMovementHeuristics.hpp"


void BasicMovementHeuristics::eval()
{
    const float maxX = 0.5;
    float diff = mSensors->frontRight() - mSensors->frontLeft();
    float minDiff = mSensors->maxDelta() * 2 + mSensors->width();
    Vector2f direction;

    if (fabs(diff) > minDiff && !isnan(diff)) {
        diff -= minDiff;

        if (isinf(diff)) {
            direction.setX(maxX * (signbit(diff) * -2 + 1));
        } else {
            direction.setX(maxX * diff / mSensors->maximum());
        }
    }

    float front = mSensors->front();
    float maxY = sqrt(1 - direction.x() * direction.x());

    direction.setY(isinf(front) ? maxY : front * maxY / mSensors->maximum());

    mMovementController->setDirection(direction);
}


BasicMovementHeuristics::BasicMovementHeuristics(BreadthSensors *sensors,
        MovementController *movementController)
    : mSensors(sensors),
    mMovementController(movementController)
{
    EventObjectConnect(sensors, ready, this, eval);
}
