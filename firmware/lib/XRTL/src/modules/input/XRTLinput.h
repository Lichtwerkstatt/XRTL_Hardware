#ifndef XRTLINPUT_H
#define XRTLINPUT_H

#include "common/XRTLfunctions.h"

// averaging and storing input value on an input pin
class XRTLinput {
private:
    uint8_t pin = 35;

    int64_t averageMicroSeconds = 100000;
    int64_t now = 0;
    int64_t next = 0;

    uint16_t sampleCount = 0;
    // buffer used for averaging values, 64 bit should prevent overflows for roughly 60 years at normal clock speed
    uint64_t buffer = 0;
    double voltage = 0.0;

public:
    void attach(uint8_t inputPin);
    void averageTime(int64_t measurementTime);
    void loop();
    double readMilliVolts();
};

#endif