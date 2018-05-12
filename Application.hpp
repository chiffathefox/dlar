
#pragma once


#include "EventEmitter.hpp"


class Application : public EventEmitter
{

    EVENT_EMITTER_SIGNAL(Application, started);
    EVENT_EMITTER_SIGNAL(Application, loop);


    static Application sInstance;


public:

    inline static Application *instance()
    {
        return &sInstance;
    }

};
