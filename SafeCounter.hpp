
#pragma once


template<typename T>
class SafeCounter
{

    T mValue;
    T mDelta;


public:

    inline explicit SafeCounter(const T value, const T delta)
        : mValue(value),
        mDelta(delta)
    {

    }


    inline T value() const
    {
        return mValue;
    }


    inline void setValue(const T value)
    {
        mValue = value;
    }


    inline T delta() const
    {
        return mDelta;
    }


    inline void setDelta(const T value)
    {
        mDelta = value;
    }


    bool operator <=(const T value)
    {
        T result = mValue + mDelta;

        if (result < mValue) {
            return mValue >= value && result <= value;
        }

        return result <= value;
    }

};
