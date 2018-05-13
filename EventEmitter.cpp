
#include "Arduino.h"

#include "Debug.hpp"
#include "Application.hpp"

#include "EventEmitter.hpp"


EventEmitter::EventEmitter()
    : mLastEmitted(-1),
    mEmitting(false)
{
}


void EventEmitter::connect(EventObject *receiver, Slot slot)
{
    ReceiverSlot receiverSlot(receiver, slot);

    if (!mReceivers.has(receiverSlot)) {
        mReceivers.insert(receiverSlot);
    } else {
        debugWarn();
    }
}


void EventEmitter::disconnect(EventObject *receiver, Slot slot)
{
    mReceivers.remove(ReceiverSlot(receiver, slot), false);
}


void EventEmitter::once(EventObject *receiver, Slot slot)
{
    ReceiverSlot receiverSlot(receiver, slot, true);

    if (!mReceivers.has(receiverSlot)) {
        mReceivers.insert(receiverSlot);
    } else {
        debugWarn();
    }
}


void EventEmitter::emit()
{
    Queue<ReceiverSlot> receivers(mReceivers);
    QueueNode<ReceiverSlot> *head = receivers.head();

    mEmitting = true;

    for (QueueNode<ReceiverSlot> *node = head->next;
        node != head;
        node = node->next) {

        ReceiverSlot &receiverSlot = node->value;

        if (receiverSlot.once) {
            mReceivers.remove(receiverSlot);
        }

        receiverSlot.slot(receiverSlot.receiver);
    }

    mLastEmitted = millis();
    mEmitting = false;
}


void EventEmitter::post()
{
    EventObjectOnce(Application::instance(), loopPost, this, emit);
}
