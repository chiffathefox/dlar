
#pragma once


template<typename T>
class ValueContainer
{

    T mValue;
    unsigned mOk:1;


public:

    inline explicit ValueContainer()
    {

    }


    inline explicit ValueContainer(T value, bool ok)
        : mValue(value),
        mOk(ok)
    {

    }


    inline T value() const
    {
        return mValue;
    }


    inline bool ok() const
    {
        return mOk;
    }


};
