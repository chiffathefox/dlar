
#pragma once


#include "RangeSensor.hpp"


class HystRangeSensor : public RangeSensor
{

    EVENT_OBJECT_SLOT(HystRangeSensor, onRangeReady);


    RangeSensor *mSensor;
    int mSize;
    int mIndex;
    float mLast;
    float *mBuffer;


public:

    explicit HystRangeSensor(RangeSensor *sensor, int size);
    ~HystRangeSensor();

    virtual void start() override;
    virtual void reinit() override;
    virtual float range() const override;
    virtual float delta() const override;
    virtual float maximum() const override;


    inline int size() const
    {
        return mSize;
    }

};
