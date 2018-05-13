
#include "Arduino.h"

#include "Debug.hpp"


void Debug::disableAll()
{
    analogWrite(3, 0);
    analogWrite(5, 0);
    analogWrite(6, 0);
    analogWrite(9, 0);
    analogWrite(10, 0);
    analogWrite(11, 0);
}


void Debug::panic()
{
    disableAll();
    pinMode(LED_BUILTIN, OUTPUT);

    for (bool state = LOW ;; state = !state) {
        digitalWrite(LED_BUILTIN, state);
        delay(500);
    }
}


void Debug::panic(const char *file, int line, const char *message)
{
    bool state = LOW;

    disableAll();

    if (Serial) {
        Serial.end();
    }

    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);

    for (unsigned char n = 0 ;; n++) {
        if (n % 2 == 0) {
            Serial.write("PANIC: ");
            Serial.write(file);
            Serial.write(":");
            Serial.print(line, DEC);
            Serial.write(": ");
            Serial.println(message);
        }

        digitalWrite(LED_BUILTIN, state);
        state = !state;

        delay(500);
    }
}
