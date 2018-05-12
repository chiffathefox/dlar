
#include "Arduino.h"

#include "EventEmitter.hpp"


void EventEmitter::connect(EventEmitter *receiver, Slot slot)
{
    mReceivers.insert(ReceiverSlot(receiver, slot));
}


void EventEmitter::emit()
{
    QueueNode<ReceiverSlot> *head = mReceivers.head();

    for (QueueNode<ReceiverSlot> *node = head->next;
        node != head;
        node = node->next) {

        node->value.slot(node->value.receiver);
    }

    mLastEmitted = millis();
}
