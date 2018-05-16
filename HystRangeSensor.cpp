
#include <math.h>

#include "HystRangeSensor.hpp"


void HystRangeSensor::onRangeReady()
{
    mBuffer[mIndex] = mSensor->range();
    mIndex = (mIndex + 1) % size();

    if (mIndex != 0) {
        return;
    }

    int infs = 0;

    for (int i = 0; i < size(); i++) {
        infs += isinf(mBuffer[i]);
    }

    if (infs * 2 >= size()) {
        mLast = INFINITY;

        return;
    }

    mLast = 0;

    for (int i = 0; i < size(); i++) {
        if (isfinite(mBuffer[i])) {
            mLast += mBuffer[i];
        }
    }

    mLast /= size() - infs;
}


HystRangeSensor::HystRangeSensor(RangeSensor *sensor, int size)
    : mSensor(sensor),
    mSize(size),
    mIndex(0),
    mLast(INFINITY),
    mBuffer(new float[size])
{
    EventObjectConnect(sensor, initFailed, initFailed(), emit);
    EventObjectConnect(sensor, initFinished, initFinished(), emit);
    EventObjectConnect(sensor, rangeError, rangeError(), emit);
    EventObjectConnect(sensor, rangeReady, this, onRangeReady);
}


HystRangeSensor::~HystRangeSensor()
{
    delete[] mBuffer;
}


void HystRangeSensor::start()
{
    mSensor->start();
}


void HystRangeSensor::reinit()
{
    mSensor->reinit();
}


float HystRangeSensor::range() const
{
    return mLast;
}


float HystRangeSensor::delta() const
{
    return mSensor->delta();
}


float HystRangeSensor::maximum() const
{
    return mSensor->maximum();
}
