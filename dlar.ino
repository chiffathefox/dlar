
#include <Wire.h>
#include <VL53L0X.h>

#include "Event.hpp"
#include "Debug.hpp"
#include "Performance.hpp"


VL53L0X sensor;


void
setup()
{
    Serial.begin(9600);

    new Performance();

    Event::emit(Event::Start);


    //Wire.begin();
    //sensor.init();
    //sensor.setTimeout(500);
    //sensor.startContinuous();
}


void
loop()
{
    Event::emit(Event::Loop);

    //debugInfo() << sensor.readRangeContinuousMillimeters();
}
