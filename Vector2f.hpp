
#pragma once


#include <math.h>

#include "Logger.hpp"


class Vector2f
{

    float mX;
    float mY;


public:

    inline Vector2f(float x = 0, float y = 0)
        : mX(x), mY(y)
    {
        
    }


    inline float x() const
    {
        return mX;
    }


    inline void setX(float value)
    {
        mX = value;
    }
    

    inline float y() const
    {
        return mY;
    }


    inline void setY(float value)
    {
        mY = value;
    }


    inline void set(float x, float y)
    {
        setX(x);
        setY(y);
    }


    inline float dot(const Vector2f &value)
    {
        return x() * value.x() + y() * value.y();
    }


    inline float sqrMagnitude() const
    {
        return x() * x() + y() * y();
    }


    inline float magnitude() const
    {
        return sqrt(sqrMagnitude());
    }


    inline Vector2f operator+(const Vector2f &value) const
    {
        return Vector2f(x() + value.x(), y() + value.y());
    }


    inline Vector2f operator-(const Vector2f &value) const
    {
        return Vector2f(x() - value.x(), y() - value.y());
    }


    inline Vector2f operator/(const Vector2f &value) const
    {
        return Vector2f(x() / value.x(), y() / value.y());
    }


    inline Vector2f operator/(float value) const
    {
        return Vector2f(x() / value, y() / value);
    }


    inline Vector2f operator*(const Vector2f &value) const
    {
        return Vector2f(x() * value.x(), y() * value.y());
    }


    inline Vector2f operator*(float value) const
    {
        return Vector2f(x() * value, y() * value);
    }


    inline Vector2f & operator+=(const Vector2f &value)
    {
        set(x() + value.x(), y() + value.y());

        return *this;
    }


    inline Vector2f & operator-=(const Vector2f &value)
    {
        set(x() - value.x(), y() - value.y());

        return *this;
    }


    inline Vector2f & operator/=(const Vector2f &value)
    {
        set(x() / value.x(), y() / value.y());

        return *this;
    }


    inline Vector2f & operator/=(float value)
    {
        set(x() / value, y() / value);

        return *this;
    }


    inline Vector2f & operator*=(const Vector2f &value)
    {
        set(x() * value.x(), y() * value.y());

        return *this;
    }


    inline Vector2f & operator*=(float value)
    {
        set(x() * value, y() * value);

        return *this;
    }


    inline bool operator==(const Vector2f &value) const
    {
        return x() == value.x() && y() == value.y();
    }


};
