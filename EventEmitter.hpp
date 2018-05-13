
#pragma once 


#include "Queue.hpp"
#include "EventObject.hpp"


class EventEmitter : public EventObject
{

    EVENT_OBJECT_SLOT(EventEmitter, emit);


public:

    typedef void (*Slot)(EventObject *receiver);


    explicit EventEmitter();

    void connect(EventObject *receiver, Slot slot);
    void disconnect(EventObject *receiver, Slot slot);
    void once(EventObject *receiver, Slot slot);
    void post();


    inline unsigned long lastEmitted() const
    {
        return mLastEmitted;
    }


    inline bool emitting() const
    {
        return mEmitting;
    }


private:

    struct ReceiverSlot
    {

        EventObject *receiver;
        Slot slot;

        unsigned once:1;

        
        inline explicit ReceiverSlot(EventObject *receiver = nullptr, 
                Slot slot = nullptr, bool once = false)
            : receiver(receiver),
            slot(slot),
            once(once)
        {

        }


        bool operator==(const ReceiverSlot &value) const
        {
            if (receiver == nullptr) {
                if (slot == nullptr) {
                    return value.receiver != nullptr && value.slot != nullptr;
                }

                return slot == value.slot;
            }

            if (slot == nullptr) {
                return receiver == value.receiver;
            }

            return slot == value.slot && receiver == value.receiver;
        }


    };


    Queue<ReceiverSlot> mReceivers;
    unsigned long mLastEmitted;

    unsigned mEmitting:1;


};
