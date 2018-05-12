
#include "Arduino.h"

#include "Debug.hpp"
#include "Application.hpp"

#include "RickshawController.hpp"


void RickshawController::onStarted()
{
    pinMode(mPwmPin, OUTPUT);
    pinMode(mFwdPin, OUTPUT);
    pinMode(mBwdPin, OUTPUT);

    mServo.attach(mServoPin);

    writeIdle();
}


void RickshawController::writeIdle()
{
    analogWrite(mPwmPin, 0);
    digitalWrite(mFwdPin, LOW);
    digitalWrite(mBwdPin, LOW);

    mServo.write(servoMiddleAngle());
}


unsigned char RickshawController::servoAngle(float y) const
{
    return (float) servoMiddleAngle() + y * mServoFactors[y > 0];
}


RickshawController::RickshawController(unsigned char pwmPin, 
        unsigned char fwdPin, unsigned char bwdPin, unsigned char servoPin)
    : MovementController(),
    mPwmPin(pwmPin),
    mFwdPin(fwdPin),
    mBwdPin(bwdPin),
    mServoPin(servoPin)
{
    EventEmitterConnect(Application::instance(), started, this, onStarted);

    setServoAngles(0, 90, 180);
    setMaxMotorDutyCycle(30);
}


void RickshawController::setDirection(const Vector2f &value)
{
    MovementController::setDirection(value);

    bool isForward = value.x() > 0;

    digitalWrite(mFwdPin, isForward);
    digitalWrite(mBwdPin, !isForward);
    analogWrite(mPwmPin, (float) maxMotorDutyCycle() * value.x());

    mServo.write(servoAngle(value.y()));
}


void RickshawController::setServoAngles(unsigned char left,
        unsigned char middle, unsigned char right)
{
    debugAssert((middle < left && middle > right) ||
            (middle > left && middle < right));

    float middlef = middle;

    mServoMiddleAngle = middle;
    mServoFactors[0] = (float) left - middlef;
    mServoFactors[1] = (float) right - middlef;
}
