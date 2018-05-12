
#include "BreadthSensors.hpp"


BreadthSensors::BreadthSensors(float width, float length)
    : EventEmitter(),
    mWidth(width),
    mLength(length)
{
    
}


float BreadthSensors::width() const
{
    return mWidth;
}


float BreadthSensors::length() const
{
    return mLength;
}
