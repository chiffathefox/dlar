
#include "Event.hpp"
#include "Debug.hpp"
#include "Performance.hpp"


void
setup()
{
    Serial.begin(9600);

    new Performance();

    Event::emit(Event::Start);
}


void
loop()
{
    Event::emit(Event::Loop);
}
