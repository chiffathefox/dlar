
#include "Arduino.h"

#include "Application.hpp"

#include "Timer.hpp"


void Timer::onLoop()
{

    /* TODO: Implement a BST based algorithm */

    if (mStartTime != -1) {
        unsigned long time = millis();

        if (mStartTime > time) {
            if ((unsigned long) -1 - mStartTime + time >= timeout()) {
                expired()->emit();

                mStartTime = singleShot() || mStartTime == -1 ? -1 : millis();
            }
        } else if (time - mStartTime >= timeout()) {
            expired()->emit();

            mStartTime = singleShot() || mStartTime == -1 ? -1 : millis();
        }
    }
}


Timer::Timer(unsigned long timeout, bool singleShot)
    : EventObject(),
    mStartTime(-1)
{
    setTimeout(timeout);
    setSingleShot(singleShot);

    /* TODO: disconnect from loop when possible */
    EventObjectConnect(Application::instance(), loop, this, onLoop);
}
