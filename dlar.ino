
#include <Wire.h>

#include "Debug.hpp"
#include "Performance.hpp"
#include "Application.hpp"
#include "VL53L0XAsync.hpp"
#include "BreadthSensors.hpp"
#include "RickshawController.hpp"
#include "BasicMovementHeuristics.hpp"


static void sensorOnInitFinished(EventObject *receiver)
{
    VL53L0XAsync *sensor = static_cast<VL53L0XAsync *> (receiver);
    debugAssert(sensor->setMeasurementTimingBudget(80000));
    debugAssert(sensor->setSignalRateLimit(0.1));
}


int freeRam()
{
    extern int __heap_start, *__brkval;
    int v;

    return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}


void
setup()
{
    Wire.begin();
    Serial.begin(9600);

    Performance *performance = new Performance;
    BreadthSensors *sensors = new BreadthSensors(170, 300);
    VL53L0XAsync *sensor;

    sensor = new VL53L0XAsync(12, 44);
    sensor->initFinished()->connect(sensor, &sensorOnInitFinished);
    sensors->setFront(sensor);

    sensor = new VL53L0XAsync(10, 46);
    sensor->initFinished()->connect(sensor, &sensorOnInitFinished);
    sensors->setFrontLeft(sensor);

    sensor = new VL53L0XAsync(11, 45);
    sensor->initFinished()->connect(sensor, &sensorOnInitFinished);
    sensors->setFrontRight(sensor);

    RickshawController *rickshawController = new RickshawController(3, 4, 5, 9);
    rickshawController->setMaxMotorDutyCycle(25);
    rickshawController->setServoAngles(180, 90, 0);

    BasicMovementHeuristics *heuristics = new BasicMovementHeuristics(sensors,
            rickshawController, performance);

    debugInfo() << sensors->maxDelta() << sensors->maximum();


    Application::instance()->started()->emit();
}


void
loop()
{
//    debugLog() << freeRam();
    Application::instance()->loop()->emit();
    Application::instance()->loopPost()->emit();
}
