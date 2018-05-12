
#pragma once


#include <Servo.h>

#include "MovementController.hpp"


class RickshawController : public MovementController
{

    const unsigned char mPwmPin;
    const unsigned char mFwdPin;
    const unsigned char mBwdPin;
    const unsigned char mServoPin;

    unsigned char mMaxMotorDutyCycle;

    unsigned char mServoMiddleAngle;
    float mServoFactors[2];

    Servo mServo;


    void writeIdle();
    unsigned char servoAngle(float y) const;


protected:

    virtual void startEvent() override;


public:

    explicit RickshawController(unsigned char pwmPin, unsigned char fwdPin,
            unsigned char bwdPin, unsigned char servoPin);

    virtual void setDirection(const Vector2f &value) override;

    inline unsigned char maxMotorDutyCycle() const
    {
        return mMaxMotorDutyCycle;
    }


    inline unsigned char setMaxMotorDutyCycle(unsigned char value)
    {
        mMaxMotorDutyCycle = value;
    }


    inline unsigned char servoMiddleAngle() const
    {
        return mServoMiddleAngle;
    }


    void setServoAngles(unsigned char left, unsigned char middle,
            unsigned char right);

};
