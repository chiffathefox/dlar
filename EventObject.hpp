
#pragma once


#define EVENT_OBJECT_SIGNAL(clazz, name)                                    \
    public:                                                                 \
                                                                            \
        inline EventEmitter *name()                                         \
        {                                                                   \
            return &mSignal_##name;                                         \
        }                                                                   \
                                                                            \
                                                                            \
    private:                                                                \
        EventEmitter mSignal_##name;        
        


#define EVENT_OBJECT_SLOT(clazz, name)                                      \
    public:                                                                 \
                                                                            \
        virtual void name();                                                \
        static void name##Static(EventObject *receiver)                     \
        {                                                                   \
            static_cast<clazz *> (receiver)->name();                        \
        }


#define EventObjectConnect(emitter, signal, receiver, name)                 \
    (emitter)->signal()->connect(receiver, &((receiver)->name##Static))


#define EventObjectDisconnect(emitter, signal, receiver, name)              \
    (emitter)->signal()->disconnect(receiver, &((receiver)->name##Static))


#define EventObjectOnce(emitter, signal, receiver, name)                    \
    (emitter)->signal()->once(receiver, &((receiver)->name##Static))


class EventObject
{


public:

    ~EventObject();

};


#include "EventEmitter.hpp"
