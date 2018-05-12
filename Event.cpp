
#include "Arduino.h"

#include "Debug.hpp"

#include "Event.hpp"


const size_t Event::MaxEventsCount = 8;

Queue<Event *> Event::sEvents[Event::MaxEventsCount];
unsigned long Event::sLastEmited[Event::MaxEventsCount] = {(unsigned long) -1};
Event::type_t Event::sAdded = Loop + 1;


void Event::emit(type_t type)
{
    if (type == Start) {
        if (sLastEmited[Start] != -1) {
            return;
        }
    }

    QueueNode<Event *> *head = sEvents[type].head();

    for (QueueNode<Event *> *node = head->next;
        node != head;
        node = node->next) {

        node->value->handleEvent(type);
    }

    sLastEmited[type] = millis();
}


unsigned long Event::lastEmited(type_t type)
{
    return sLastEmited[type];
}


Event::type_t Event::add()
{
    debugAssert(sAdded < MaxEventsCount);

    return sAdded++;
}


Event::Event()
    : mSubscribedLsb(0),
    mSubscribedMsb(0)
{

}


Event::~Event()
{
    for (size_t i = 0; i < MaxEventsCount; i++) {
        unsubscribe(i);
    }
}


void Event::handleEvent(type_t type)
{
    switch (type) {


    case Start:

        startEvent();

        break;


    case Loop:

        loopEvent();

        break;


    default:

        debugAssert(type == Start && type == Loop);

        break;


    }
}


void Event::startEvent()
{
    static bool called = false;

    if (!called) {
        called = true;

        for (size_t i = 1; i < MaxEventsCount; i++) {
            sLastEmited[i] = -1;
        }
    }
}


void Event::loopEvent()
{

}


void Event::subscribe(type_t type)
{
    if (subscribed(type)) {
        return;
    }

    if (type >= 64) {
        mSubscribedMsb |= 1 << (type % 64);
    } else {
        mSubscribedLsb |= 1 << type;
    }

    sEvents[type].insert(this);
}


void Event::unsubscribe(type_t type)
{
    if (!subscribed(type)) {
        return;
    }

    if (type >= 64) {
        mSubscribedMsb &= ~(1 << (type % 64));
    } else {
        mSubscribedLsb &= ~(1 << type);
    }

    sEvents[type].remove(this);
}


bool Event::subscribed(type_t type) const
{
    if (type >= 64) {
        return mSubscribedMsb & (1 << (type % 64));
    }

    return mSubscribedLsb & (1 << type);
}
