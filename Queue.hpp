
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


public:

    inline explicit Queue(const T &sentinel = T())
        : mSentinel(sentinel)
    {
        mSentinel.next = &mSentinel;
        mSentinel.prev = &mSentinel;
    }


    inline explicit Queue(Queue<T> &another)
        : mSentinel(another.mSentinel)
    {
        mSentinel.next = &mSentinel;
        mSentinel.prev = &mSentinel;

        for (QueueNode<T> *node = another.head()->next;
                node != another.head();
                node = node->next) {

            insert(node->value);
        }
    }


    inline ~Queue()
    {
        QueueNode<T> *next;
        
        for (QueueNode<T> *node = head()->next; node != head(); node = next) {
            next = node->next;
            delete node;
        }
    }


    inline QueueNode<T> *head()
    {
        return &mSentinel;
    }


    inline void insert(const T &item)
    {
        QueueNode<T> *node = new QueueNode<T>(item);

        node->next = head();
        node->prev = head()->prev;
        node->prev->next = node;
        head()->prev = node;
    }


    inline void remove(const T &item, bool once = true)
    {
        for (QueueNode<T> *node = head()->next;
                node != head(); 
                node = node->next) {

            if (item == node->value) {
                QueueNode<T> *next = node->next;

                node->prev->next = next;
                next->prev = node->prev;

                delete node;

                if (once) {
                    return;
                }

                node = next;
            }
        }
    }


    inline bool has(const T &item)
    {
        for (QueueNode<T> *node = head()->next;
                node != head(); 
                node = node->next) {

            if (node->value == item) {
                return true;
            }
        }

        return false;
    }
        
};
