
#pragma once


#include "EventObject.hpp"
#include "RangeSensor.hpp"
#include "Debug.hpp"


#define BREADTH_SENSOR(name, camelPart, factor, mask)                          \
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
            m##camelPart = value;                                              \
        }                                                                      \
                                                                               \
                                                                               \
        inline float name() const                                              \
        {                                                                      \
            debugAssert(mReadySensors & mSensors == mSensors);                 \
                                                                               \
            return (float) m##camelPart->range() * factor;                     \
        }


#define BREADTH_SENSORS                            \
    BREADTH_SENSOR(front, Front, 1, 1);            \
    BREADTH_SENSOR(frontLeft, FrontLeft, -1, 2);   \
    BREADTH_SENSOR(frontRight, FrontRight, 1, 4);  \
    BREADTH_SENSOR(rearLeft, RearLeft, -1, 8);     \
    BREADTH_SENSOR(rearRight, RearRight, 1, 16);


class BreadthSensors : public EventObject
{

    EVENT_OBJECT_SIGNAL(BreadthSensors, ready);

    BREADTH_SENSORS;


    const float mWidth;
    const float mLength;

    unsigned char mSensors;
    unsigned char mReadySensors;


public:

    explicit BreadthSensors(float width, float length);

    float width() const;
    float length() const;

};


#undef BREADTH_SENSOR
