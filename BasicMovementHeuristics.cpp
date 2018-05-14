
#include <math.h>
#include <float.h>

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
            direction.setX(maxX * ((diff < 0) * -2 + 1));
        } else {
            direction.setX(maxX * diff / mSensors->maximum());
        }

        mCounter = 0;
        mLastFrontLeft = mSensors->frontLeft();
        mLastFrontRight = mSensors->frontRight();
    }

    float front = mSensors->front();
    float maxY = sqrt(1 - direction.x() * direction.x());

    direction.setY(isinf(front) ? maxY : maxY * front / mSensors->maximum());

    debugLog() << mSensors->frontLeft() << mSensors->front() 
               << mSensors->frontRight()
               << direction.x() << direction.y();

    mMovementController->setDirection(direction);

    mTicker->tick();
}


BasicMovementHeuristics::BasicMovementHeuristics(BreadthSensors *sensors,
        MovementController *movementController, Performance *performance)
    : mSensors(sensors),
    mMovementController(movementController),
    mTicker(performance->createTicker()),
    mLastFrontLeft(-1),
    mLastFrontRight(-1),
    mCounter(0)
{
    EventObjectConnect(sensors, ready, this, eval);
}
