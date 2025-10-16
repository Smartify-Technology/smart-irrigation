// WaterPump.hpp
#ifndef WATERPUMP_HPP
#define WATERPUMP_HPP
#include "Actuator.hpp"

class WaterPump : public Actuator {
public:
    WaterPump(int pin) : Actuator(pin) {}
};

#endif
