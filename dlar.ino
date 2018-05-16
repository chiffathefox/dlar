
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

    debugAssert(sensor->setMeasurementTimingBudget(20000));
    sensor->setSignalRateLimit(0.1);
}


static void sensorOnInitFailed(EventObject *receiver)
{
    debugWarn();
}


static VL53L0XAsync *createSensor(unsigned char xshutPin,
        unsigned char address)
{
    VL53L0XAsync *sensor = new VL53L0XAsync(xshutPin, address);

    sensor->setVcselPeriodPreRange(18);
    sensor->setVcselPeriodFinalRange(14);
    sensor->initFinished()->connect(sensor, &sensorOnInitFinished);
    sensor->initFailed()->connect((EventEmitter *) 257, &sensorOnInitFailed);
    sensor->rangeError()->connect((EventEmitter *) 258, &sensorOnInitFailed);

    return sensor;
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
    BreadthSensors *sensors = new BreadthSensors(105, 300);

    sensors->setFront(createSensor(12, 44));
    sensors->setFrontLeft(createSensor(10, 46));
    sensors->setFrontRight(createSensor(11, 45));

    RickshawController *rickshawController = new RickshawController(3, 4, 5, 9);
    rickshawController->setMaxMotorDutyCycle(20);
    rickshawController->setServoAngles(180, 90, 0);

    BasicMovementHeuristics *heuristics = new BasicMovementHeuristics(sensors,
            rickshawController, performance);

    heuristics->setMaxDiff(800);
    heuristics->setBrakingDistance(200);


    Application::instance()->started()->emit();
}


void
loop()
{
    Application::instance()->loop()->emit();
    Application::instance()->loopPost()->emit();
}
