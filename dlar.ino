
#include "TWI.hpp"
#include "Debug.hpp"
#include "Performance.hpp"
#include "Application.hpp"
#include "VL53L0XAsync.hpp"
#include "SerialLogger.hpp"
#include "BreadthSensors.hpp"
#include "HystRangeSensor.hpp"
#include "RickshawController.hpp"
#include "BasicMovementHeuristics.hpp"


static void sensorOnInitFinished(EventObject *receiver)
{
    VL53L0XAsync *sensor = static_cast<VL53L0XAsync *> (receiver);

    if (!sensor->setMeasurementTimingBudget(20000) ||
            !sensor->setSignalRateLimit(0.1)) {

        sensor->initFinished()->stopPropagation();
        sensor->reinit();
    }
}


static void frontSensorOnInitFailed(EventObject *receiver)
{
    debugWarn();
}


static void frontSensorOnRangeError(EventObject *receiver)
{
    debugWarn();
}


static void frontLeftSensorOnInitFailed(EventObject *receiver)
{
    debugWarn();
}


static void frontLeftSensorOnRangeError(EventObject *receiver)
{
    debugWarn();
}


static void frontRightSensorOnInitFailed(EventObject *receiver)
{
    debugWarn();
}


static void frontRightSensorOnRangeError(EventObject *receiver)
{
    debugWarn();
}


static void sensorOnInitFailed(EventObject *receiver)
{
    debugWarn();
}


static void sensorOnRangeError(EventObject *receiver)
{
    debugWarn();
}


static RangeSensor *createSensor(unsigned char xshutPin,
        unsigned char address)
{
    VL53L0XAsync *sensor = new VL53L0XAsync(xshutPin, address);

    sensor->setVcselPeriodPreRange(18);
    sensor->setVcselPeriodFinalRange(14);
    sensor->initFinished()->connect(sensor, &sensorOnInitFinished);

    RangeSensor *s = new HystRangeSensor(sensor, 3);

    s->initFailed()->connect(s, &sensorOnInitFailed);
    s->rangeError()->connect(s, &sensorOnRangeError);

    return s;
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
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    delay(6000);
    digitalWrite(LED_BUILTIN, LOW);

    TWI::instance()->begin();
    TWI::instance()->setClock(1000);

    Serial.begin(460800);
    SerialLogger::setSerial(&Serial);

    Performance *performance = new Performance;
    BreadthSensors *sensors = new BreadthSensors(105, 300);
    RangeSensor *sensor = createSensor(29, 44);

    sensor->initFailed()->connect(sensor, &frontSensorOnInitFailed);
    sensor->rangeError()->connect(sensor, &frontSensorOnRangeError);

    sensors->setFront(sensor);

    sensor = createSensor(30, 46);

    sensor->initFailed()->connect(sensor, &frontLeftSensorOnInitFailed);
    sensor->rangeError()->connect(sensor, &frontLeftSensorOnRangeError);

    sensors->setFrontLeft(sensor);

    sensor = createSensor(31, 45);

    sensor->initFailed()->connect(sensor, &frontRightSensorOnInitFailed);
    sensor->rangeError()->connect(sensor, &frontRightSensorOnRangeError);

    sensors->setFrontRight(sensor);

    RickshawController *rickshawController =
        new RickshawController(27, 21, 22, PA7);

    rickshawController->setMaxMotorDutyCycle(20);
    rickshawController->setServoAngles(180, 90, 0);

    BasicMovementHeuristics *heuristics = new BasicMovementHeuristics(sensors,
            rickshawController, performance);

    heuristics->setMaxDiff(800);
    heuristics->setBrakingDistance(200);

    debugInfo() << "started at" << millis();


    Application::instance()->started()->emit();
}


void
loop()
{
    Application::instance()->loop()->emit();
    Application::instance()->loopPost()->emit();
}
