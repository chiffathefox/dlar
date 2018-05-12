
#include "BreadthSensors.hpp"


BreadthSensors::BreadthSensors(float width, float length)
    : mWidth(width),
    mLength(length),
    mValueEventType(Event::add())
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


Event::type_t BreadthSensors::valueEventType() const
{
    return mValueEventType;
}
