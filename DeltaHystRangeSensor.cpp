
#include <math.h>

#include "DeltaHystRangeSensor.hpp"


void DeltaHystRangeSensor::onRangeReady()
{
    float value = mSensor->range();

    if (fabs(mLast - value) > mSensor->delta()) {
        mLast = value;
    }

    rangeReady()->post();
}


DeltaHystRangeSensor::DeltaHystRangeSensor(RangeSensor *sensor)
    : mSensor(sensor),
    mLast(INFINITY)
{
    EventObjectConnect(sensor, initFailed, initFailed(), emit);
    EventObjectConnect(sensor, initFinished, initFinished(), emit);
    EventObjectConnect(sensor, rangeError, rangeError(), emit);
    EventObjectConnect(sensor, rangeReady, this, onRangeReady);
}


void DeltaHystRangeSensor::start()
{
    mSensor->start();
}


void DeltaHystRangeSensor::reinit()
{
    mSensor->reinit();
}


float DeltaHystRangeSensor::range() const
{
    return mLast;
}


float DeltaHystRangeSensor::delta() const
{
    return mSensor->delta();
}


float DeltaHystRangeSensor::maximum() const
{
    return mSensor->maximum();
}
