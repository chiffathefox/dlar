
#include <Wire.h>
#include <VL53L0X.h>

#include "Debug.hpp"
#include "Performance.hpp"
#include "Application.hpp"
#include "VL53L0XAsync.hpp"
#include "BreadthSensors.hpp"


static VL53L0XAsync *sensor;
static BreadthSensors *sensors;
static Performance *performance;
static Performance::Ticker *ticker;


static void initFinishedHandler(EventObject *receiver)
{
    debugAssert(sensor->setMeasurementTimingBudget(20000));
}


static void readyHandler(EventObject *receiver)
{
    debugLog() << sensors->front();
    ticker->tick();
}


void
setup()
{
    Wire.begin();
    Serial.begin(9600);

    sensor = new VL53L0XAsync(2, 43);
    sensor->initFinished()->connect(nullptr, &initFinishedHandler);

    sensors = new BreadthSensors(10000, 20000);
    sensors->ready()->connect(nullptr, &readyHandler);
    sensors->setFront(sensor);

    performance = new Performance;
    ticker = performance->createTicker();

    Application::instance()->started()->emit();
}


void
loop()
{
    Application::instance()->loop()->emit();
    Application::instance()->loopPost()->emit();
}
