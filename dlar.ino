
#include <Wire.h>
#include <VL53L0X.h>

#include "Debug.hpp"
#include "Performance.hpp"
#include "Application.hpp"


VL53L0X sensor;


void
setup()
{
    Serial.begin(9600);

    static Performance performance;

    Application::instance()->started()->emit();

    /*
    Wire.begin();
    sensor.init();
    sensor.setTimeout(500);
    debugInfo() << sensor.setMeasurementTimingBudget(20000);
    sensor.startContinuous();

    debugLog() << "epilogue";
    */
}


void
loop()
{
    Application::instance()->loop()->emit();

    /*
    delay(25);
    unsigned long start = millis();
    sensor.readRangeContinuousMillimeters();
//    sensor.readRangeSingleMillimeters();

    debugLog() << millis() - start;
    */
}
