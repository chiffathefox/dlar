
#pragma once


#include "Queue.hpp"


#define EVENT_EMITTER_SIGNAL(clazz, name)                            \
    public:                                                          \
                                                                     \
        inline EventEmitter *name()                                  \
        {                                                            \
            return &mSignal_##name;                                  \
        }                                                            \
                                                                     \
                                                                     \
    private:                                                         \
        EventEmitter mSignal_##name;        
        


#define EVENT_EMITTER_SLOT(clazz, name)                              \
    void name();                                                     \
    static void name##Static(EventEmitter *emitter)                  \
    {                                                                \
        static_cast<clazz *> (emitter)->name();                      \
    }


#define EventEmitterConnect(emitter, signal, receiver, name)         \
    emitter->signal()->connect(receiver, &(receiver->name##Static))


class EventEmitter
{


public:

    typedef void (*Slot)(EventEmitter *emitter);


    inline explicit EventEmitter()
        : mLastEmitted(-1)
    {

    }


    void connect(EventEmitter *receiver, Slot slot);
    void emit();


    inline unsigned long lastEmitted() const
    {
        return mLastEmitted;
    }


private:

    struct ReceiverSlot
    {

        EventEmitter *receiver;
        Slot slot;

        
        inline explicit ReceiverSlot(EventEmitter *receiver = nullptr, 
                Slot slot = nullptr)
            : receiver(receiver),
            slot(slot)
        {

        }


    };


    Queue<ReceiverSlot> mReceivers;
    unsigned long mLastEmitted;


};
