
#pragma once


template<typename T>
struct QueueNode
{
    T value;
    QueueNode<T> *next;
    QueueNode<T> *prev;


    explicit QueueNode(const T &value)
        : value(value)
    {

    }
};


template<typename T>
class Queue
{

    QueueNode<T> mSentinel;
    QueueNode<T> *mHead;


public:

    inline Queue(const T &sentinel = T())
        : mSentinel(sentinel)
    {
        mSentinel.next = &mSentinel;
        mSentinel.prev = &mSentinel;
        mHead = &mSentinel;
    }


    inline ~Queue()
    {
        QueueNode<T> *next;
        
        for (QueueNode<T> *node = mHead->next; node != mHead; node = next) {
            next = node->next;
            delete node;
        }
    }


    inline QueueNode<T> *head() const
    {
        return mHead;
    }


    inline void insert(const T &item)
    {
        QueueNode<T> *node = new QueueNode<T>(item);

        node->next = mHead;
        node->prev = mHead->prev;
        node->prev->next = node;
        mHead->prev = node;
    }


    inline void remove(const T &item)
    {
        for (QueueNode<T> *node = mHead->next;
            node != mHead; 
            node = node->next) {

            if (node->value == item) {
                node->prev->next = node->next;
                node->next->prev = node->prev;

                delete node;

                return;
            }
        }
    }
        
};
