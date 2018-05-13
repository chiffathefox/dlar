
#pragma once


#include "EventObject.hpp"
#include "BreadthSensors.hpp"
#include "MovementController.hpp"


class BasicMovementHeuristics : public EventObject
{

    EVENT_OBJECT_SLOT(BasicMovementHeuristics, eval);


    BreadthSensors * const mSensors;
    MovementController * const mMovementController;
    

public:

    explicit BasicMovementHeuristics(BreadthSensors *sensors,
            MovementController *movementController);


};
