#ifndef XRTLOUTPUT_H
#define XRTLOUTPUT_H

#include "common/XRTLfunctions.h"

class XRTLoutput {
private:
    bool state = false;
    uint8_t power = 0;

    uint8_t pin;
    bool pwm;
    uint8_t channel;
    uint16_t frequency;

public:
    XRTLoutput(bool isPWM);

    void attach(uint8_t controlPin);
    bool attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency);

    void toggle(bool targetState);
    void write(uint8_t powerLvl);
    
    uint8_t read();
    bool getState();
};

#endif