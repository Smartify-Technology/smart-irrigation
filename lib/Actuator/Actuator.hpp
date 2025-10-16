// Actuator.hpp
#ifndef ACTUATOR_HPP
#define ACTUATOR_HPP

#include <Arduino.h>

class Actuator {
protected:
    int pin;
    bool state;

public:
    Actuator() : pin(-1), state(false) {}
    Actuator(int pin) : pin(pin), state(false) {
        pinMode(pin, OUTPUT);
        deactivate();
    }

    virtual int getPin() const { return pin; }

    virtual void activate() {
        digitalWrite(pin, HIGH);
        state = true;
    }

    virtual void deactivate() {
        digitalWrite(pin, LOW);
        state = false;
    }

    bool isActive() const { return state; }
};

#endif
