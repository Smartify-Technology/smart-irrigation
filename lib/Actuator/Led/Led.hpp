// Led.hpp
#ifndef LED_HPP
#define LED_HPP
#include "Actuator.hpp"

class Led : public Actuator {
public:
    Led(int pin) : Actuator(pin) {}

    void blink(int duration, int period) {
        unsigned long startTime = millis();
        while (millis() - startTime < duration) {
            digitalWrite(pin, HIGH);
            delay(period / 2);
            digitalWrite(pin, LOW);
            delay(period / 2);
        }
    }

    void blinkForTask(int duration, int period) {
        unsigned long startTime = millis();
        while (millis() - startTime < duration) {
            digitalWrite(pin, HIGH);
            vTaskDelay(period / 2 / portTICK_PERIOD_MS);
            digitalWrite(pin, LOW);
            vTaskDelay(period / 2 / portTICK_PERIOD_MS);
        }
    }
};

#endif
