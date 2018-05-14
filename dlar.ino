
#include <Wire.h>
#include <VL53L0X.h>

#include "Debug.hpp"
#include "Performance.hpp"
#include "Application.hpp"
#include "VL53L0XAsync.hpp"
#include "BreadthSensors.hpp"


static VL53L0XAsync *frontSensor;
static VL53L0XAsync *frontLeftSensor;
static VL53L0XAsync *frontRightSensor;
static VL53L0XAsync *rearLeftSensor;
static VL53L0XAsync *rearRightSensor;

//static BreadthSensors *sensors;
static Performance *performance;
static Performance::Ticker *ticker;


static void sensorOnInitFinished(EventObject *receiver)
{
    VL53L0XAsync *sensor = static_cast<VL53L0XAsync *> (receiver);

    debugAssert(sensor->setMeasurementTimingBudget(20000));

    sensor->start();
}


static void sensorOnInitFailed(EventObject *sensor)
{
    debugWarn();
}


static void rangeReady(EventObject *receiver)
{
    VL53L0XAsync *sensor = static_cast<VL53L0XAsync *> (receiver);

    unsigned char n;

    if (sensor == frontSensor) {
        n = 0;
    } else if (sensor == frontLeftSensor) {
        n = 1;
    } else if (sensor == frontRightSensor) {
        n = 2;
    }

    debugInfo() << n << sensor->range();
    ticker->tick();
}


static void readyHandler(EventObject *receiver)
{
//    debugLog() << sensors->front();
    ticker->tick();
}


void
setup()
{
    Wire.begin();
    Serial.begin(9600);

    debugWarn();

    frontSensor = new VL53L0XAsync(12, 44);
    frontSensor->initFinished()->connect(frontSensor, &sensorOnInitFinished);
    frontSensor->initFailed()->connect(frontSensor, &sensorOnInitFailed);
    frontSensor->rangeReady()->connect(frontSensor, &rangeReady);
    frontSensor->rangeError()->connect(frontSensor, &sensorOnInitFailed);

    frontLeftSensor = new VL53L0XAsync(10, 46);
    frontLeftSensor->initFinished()->connect(frontLeftSensor,
            &sensorOnInitFinished);
    frontLeftSensor->initFailed()->connect(frontLeftSensor, &sensorOnInitFailed);
    frontLeftSensor->rangeReady()->connect(frontLeftSensor, &rangeReady);
    frontLeftSensor->rangeError()->connect(frontLeftSensor, &sensorOnInitFailed);

    frontRightSensor = new VL53L0XAsync(11, 45);
    frontRightSensor->initFinished()->connect(frontRightSensor,
            &sensorOnInitFinished);
    frontRightSensor->initFailed()->connect(frontRightSensor, &sensorOnInitFailed);
    frontRightSensor->rangeReady()->connect(frontRightSensor, &rangeReady);
    frontRightSensor->rangeError()->connect(frontRightSensor, &sensorOnInitFailed);

//    sensors = new BreadthSensors(10000, 20000);
//    sensors->ready()->connect(nullptr, &readyHandler);

    performance = new Performance;
    ticker = performance->createTicker();

    Application::instance()->started()->emit();
}


void
loop()
{
    Application::instance()->loop()->emit();
    Application::instance()->loopPost()->emit();

    delay(1000);
}
