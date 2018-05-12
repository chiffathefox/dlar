
#include <Wire.h>
#include <VL53L0X.h>

#include "Debug.hpp"
#include "Performance.hpp"
#include "Application.hpp"


void
setup()
{
    Serial.begin(9600);

    new Performance;

    Application::instance()->started()->emit();
}


void
loop()
{
    Application::instance()->loop()->emit();
}
