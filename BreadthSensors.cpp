
#include "Application.hpp"

#include "BreadthSensors.hpp"


#define BREADTH_SENSOR(name, camelPart, mask)          \
    void BreadthSensors::name##InitFailed()            \
    {                                                  \
        m##camelPart->reinit();                        \
    }                                                  \
                                                       \
                                                       \
    void BreadthSensors::name##InitFinished()          \
    {                                                  \
        mSensors |= mask;                              \
        m##camelPart->start();                         \
    }                                                  \
                                                       \
                                                       \
    void BreadthSensors::name##RangeError()            \
    {                                                  \
        mSensors &= ~(mask);                           \
        m##camelPart->reinit();                        \
    }                                                  \
                                                       \
                                                       \
    void BreadthSensors::name##RangeReady()            \
    {                                                  \
        mReadySensors |= mask;                         \
                                                       \
        if (mReadySensors & mSensors == mSensors) {    \
            ready()->emit();                           \
            mReadySensors = 0;                         \
        }                                              \
    }


BREADTH_SENSORS;


BreadthSensors::BreadthSensors(float width, float length)
    : EventObject(),
    mWidth(width),
    mLength(length),
    mMaxDelta(0),
    mMaximum(0),
    mSensors(0),
    mReadySensors(0)
{

}
