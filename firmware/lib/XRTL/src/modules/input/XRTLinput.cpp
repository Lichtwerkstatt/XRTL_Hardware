#include "XRTLinput.h"

void XRTLinput::attach(uint8_t inputPin) {
    pin = inputPin;
    pinMode(pin, INPUT);

    voltage = analogReadMilliVolts(pin); // make sure voltage is initialized with real value (avoiding trigger thresholds)

    next = esp_timer_get_time() + averageMicroSeconds;
}

void XRTLinput::averageTime(int64_t measurementTime) {
    averageMicroSeconds = 1000 * measurementTime;
}

void XRTLinput::loop() {
    now = esp_timer_get_time();
    if (now < next) {// average time not reached, keep going
        buffer += analogReadMilliVolts(pin);
        sampleCount++;
        return;
    }
    else {// average time over, store averaged value
        next = now + averageMicroSeconds;
        if (sampleCount == 0) {// no measurment taken, fill buffer with single
            buffer += analogReadMilliVolts(pin);
            sampleCount++;
        }
        voltage = ((double) buffer) / ((double)sampleCount);
        buffer = 0;
        sampleCount = 0;
    }
}

double XRTLinput::readMilliVolts() {
    return voltage;
}