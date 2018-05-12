
#include "Arduino.h"

#include "Debug.hpp"


void Debug::panic(const char *file, const char *func, int line,
        const char *message)
{
    bool state = LOW;

    analogWrite(3, 0);
    analogWrite(5, 0);
    analogWrite(6, 0);
    analogWrite(9, 0);
    analogWrite(10, 0);
    analogWrite(11, 0);

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
            Serial.write(func);
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
