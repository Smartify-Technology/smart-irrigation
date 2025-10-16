// Led.hpp
#ifndef LED_HPP
#define LED_HPP
#include "Actuator.hpp"

class Led : public Actuator {
public:
    Led(int pin) : Actuator(pin) {}
};

#endif
