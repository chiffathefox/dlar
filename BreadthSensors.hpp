
#pragma once


#include <math.h>

#include "EventObject.hpp"
#include "RangeSensor.hpp"
#include "Debug.hpp"


#define BREADTH_SENSOR(name, camelPart, mask)                          \
    EVENT_OBJECT_SLOT(BreadthSensors, name##InitFailed);                       \
    EVENT_OBJECT_SLOT(BreadthSensors, name##InitFinished);                     \
    EVENT_OBJECT_SLOT(BreadthSensors, name##RangeError);                       \
    EVENT_OBJECT_SLOT(BreadthSensors, name##RangeReady);                       \
                                                                               \
                                                                               \
    private:                                                                   \
                                                                               \
        RangeSensor *m##camelPart;                                             \
                                                                               \
                                                                               \
    public:                                                                    \
                                                                               \
        inline void set##camelPart(RangeSensor *value) {                       \
            EventObjectConnect(value, initFailed, this, name##InitFailed);     \
            EventObjectConnect(value, initFinished, this, name##InitFinished); \
            EventObjectConnect(value, rangeError, this, name##RangeError);     \
            EventObjectConnect(value, rangeReady, this, name##RangeReady);     \
                                                                               \
            float delta = value->delta();                                      \
                                                                               \
            if (delta > mMaxDelta) {                                           \
                mMaxDelta = delta;                                             \
            }                                                                  \
                                                                               \
            float maximum = value->maximum();                                  \
                                                                               \
            if (maximum > mMaximum) {                                          \
                mMaximum = maximum;                                            \
            }                                                                  \
                                                                               \
            m##camelPart = value;                                              \
        }                                                                      \
                                                                               \
                                                                               \
        inline float name() const                                              \
        {                                                                      \
            debugAssert(mReadySensors & mSensors == mSensors);                 \
                                                                               \
            if (m##camelPart->range() == -1) {                                 \
                return INFINITY;                                               \
            }                                                                  \
                                                                               \
            return (float) m##camelPart->range();                              \
        }


#define BREADTH_SENSORS                         \
    BREADTH_SENSOR(front, Front, 1);            \
    BREADTH_SENSOR(frontLeft, FrontLeft, 2);    \
    BREADTH_SENSOR(frontRight, FrontRight, 4);  \
    BREADTH_SENSOR(rearLeft, RearLeft, 8);      \
    BREADTH_SENSOR(rearRight, RearRight, 16);


class BreadthSensors : public EventObject
{

    EVENT_OBJECT_SIGNAL(BreadthSensors, ready);

    BREADTH_SENSORS;


    const float mWidth;
    const float mLength;

    float mMaxDelta;
    float mMaximum;

    unsigned char mSensors;
    unsigned char mReadySensors;


public:

    explicit BreadthSensors(float width, float length);

    inline float width() const
    {
        return mWidth;
    }


    inline float length() const
    {
        return mLength;
    }


    inline float maxDelta() const
    {
        return mMaxDelta;
    }


    inline float maximum() const
    {
        return mMaximum;
    }


};


#undef BREADTH_SENSOR
