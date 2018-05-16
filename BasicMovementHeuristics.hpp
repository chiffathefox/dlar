
#pragma once


#include "EventObject.hpp"
#include "BreadthSensors.hpp"
#include "MovementController.hpp"
#include "Performance.hpp"


class BasicMovementHeuristics : public EventObject
{

    EVENT_OBJECT_SLOT(BasicMovementHeuristics, eval);


    BreadthSensors *mSensors;
    MovementController *mMovementController;
    Performance::Ticker *mTicker;
    float mMaxDiff;
    float mBrakingDistance;

    
public:

    explicit BasicMovementHeuristics(BreadthSensors *sensors,
            MovementController *movementController, Performance *performance);


    inline float maxDiff() const
    {
        return mMaxDiff;
    }


    inline void setMaxDiff(float value)
    {
        mMaxDiff = value;
    }


    inline float brakingDistance() const
    {
        return mBrakingDistance;
    }


    inline void setBrakingDistance(float value)
    {
        mBrakingDistance = value;
    }


};
