
#pragma once


#include "RangeSensor.hpp"


class DeltaHystRangeSensor : public RangeSensor
{

    EVENT_OBJECT_SLOT(DeltaHystRangeSensor, onRangeReady);


    RangeSensor *mSensor;
    float mLast;


public:

    explicit DeltaHystRangeSensor(RangeSensor *sensor);

    virtual void start() override;
    virtual void reinit() override;
    virtual float range() const override;
    virtual float delta() const override;
    virtual float maximum() const override;

};
