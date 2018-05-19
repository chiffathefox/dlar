
#include <math.h>

#include "HystRangeSensor.hpp"


void HystRangeSensor::onRangeReady()
{
    mBuffer[mIndex] = mSensor->range();
    mBufferFilled = mBufferFilled || (mIndex >= size() - 1);
    mIndex = (mIndex + 1) % size();

    if (!mBufferFilled) {
        return;
    }

    int infs = 0;

    for (int i = 0; i < size(); i++) {
        infs += isinf(mBuffer[i]);
    }

    if (infs * 2 >= size()) {
        mLast = INFINITY;
        rangeReady()->post();

        return;
    }

    float value = 0;

    for (int i = 0; i < size(); i++) {
        if (isfinite(mBuffer[i])) {
            value += mBuffer[i];
        }
    }

    value /= size() - infs;

    if (fabs(value - mLast) > mSensor->delta()) {
        mLast = value;
    }

    rangeReady()->post();
}


HystRangeSensor::HystRangeSensor(RangeSensor *sensor, int size)
    : mSensor(sensor),
    mSize(size),
    mIndex(0),
    mLast(INFINITY),
    mBuffer(new float[size]),
    mBufferFilled(false)
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
