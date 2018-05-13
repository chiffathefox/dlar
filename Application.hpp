
#pragma once


#include "EventObject.hpp"


class Application : public EventObject
{

    EVENT_OBJECT_SIGNAL(Application, started);
    EVENT_OBJECT_SIGNAL(Application, loop);
    EVENT_OBJECT_SIGNAL(Application, loopPost);


    static Application sInstance;


public:

    inline static Application *instance()
    {
        return &sInstance;
    }

};
