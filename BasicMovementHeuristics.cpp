
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
        diff -= copysign(minDiff, diff);

        if (isinf(diff)) {
            direction.setX(maxX * ((diff < 0) * -2 + 1));
        } else if (fabs(diff) >= maxDiff()) {
            direction.setX(copysign(maxX, diff));
        } else {
            float k = log(2) / maxDiff();
            float x = maxX * (exp(k * fabs(diff)) - 1);

            direction.setX(copysign(x, diff));
        }
    }

    float front = mSensors->front();
    float maxY = sqrt(1 - direction.x() * direction.x());

    if (isinf(front)) {
        direction.setY(maxY);
    } else {
        float k = log(2) / brakingDistance() / 2;
        float y = exp(k * front) - 1;

        direction.setY(front <= brakingDistance() && y < maxY ? y : maxY);
    }

    //debugLog() << mSensors->frontLeft() << mSensors->front() 
    //           << mSensors->frontRight()
    //           << direction.x() << direction.y();

    debugInfo() << mSensors->frontLeft()
                << mSensors->front()
                << mSensors->frontRight();

    float magnitude = direction.magnitude();

    if (magnitude > 1) {
        direction.setX(direction.x() / magnitude - 0.01);
        direction.setY(direction.y() / magnitude - 0.01);
    }

	//debugLog() << "DIR_X: " << direction.x();

    mMovementController->setDirection(direction);

    mTicker->tick();
}


BasicMovementHeuristics::BasicMovementHeuristics(BreadthSensors *sensors,
        MovementController *movementController, Performance *performance)
    : mSensors(sensors),
    mMovementController(movementController),
    mTicker(performance->createTicker()),
    mMaxDiff(sensors->maximum())
{
    EventObjectConnect(sensors, ready, this, eval);
}
