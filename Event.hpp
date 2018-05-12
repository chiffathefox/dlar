
#pragma once


#include <stdlib.h>
#include <inttypes.h>

#include "Queue.hpp"


class Event
{


public:

    typedef unsigned char type_t;

    enum Type : type_t {
        Start = 0,
        Loop
    };


    static void emit(type_t type);
    static unsigned long lastEmited(type_t type);
    static type_t add();


    Event();
    ~Event();


protected:

    virtual void handleEvent(type_t type);
    virtual void startEvent();
    virtual void loopEvent();

    virtual void subscribe(type_t type);
    virtual void unsubscribe(type_t type);
    bool subscribed(type_t type) const;


private:

    static const size_t MaxEventsCount;

    static Queue<Event *> sEvents[];
    static unsigned long sLastEmited[];
    static type_t sAdded;


    uint16_t mSubscribedLsb;
    uint16_t mSubscribedMsb;

};
