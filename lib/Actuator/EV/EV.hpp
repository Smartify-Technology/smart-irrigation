// EV.hpp
#ifndef EV_HPP
#define EV_HPP
#include "Actuator.hpp"

class EV : public Actuator {
    int relay;
public:
    EV() : Actuator(-1), relay(-1){ }
    EV(int pin, int relay) : Actuator(pin) {
        relay = relay;
    }

    int getRelay() {
        return relay;
    }
};

#endif
