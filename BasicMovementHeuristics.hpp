
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
    float mLastFrontLeft;
    float mLastFrontRight;
    unsigned char mCounter;

    
public:

    explicit BasicMovementHeuristics(BreadthSensors *sensors,
            MovementController *movementController, Performance *performance);


};
