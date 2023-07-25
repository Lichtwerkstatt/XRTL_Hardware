#include "XRTLinput.h"

/**
 * @brief initialize the pin as input
 * @param inputPin pin number of the pin to use as input
 * @return true if pin was attached successfully
 * @note not all pins are suitable, especially when using WiFi. Check your board pinout for available pins.
 */
void XRTLinput::attach(uint8_t inputPin) {
    pin = inputPin;
    pinMode(pin, INPUT);

    voltage = analogReadMilliVolts(pin); // make sure voltage is initialized with real value (avoiding trigger thresholds)

    next = esp_timer_get_time() + averageMicroSeconds;
    return;
}

/**
 * @brief set the averaging time of the buffer
 * @param measurementTime time in ms to average the voltage for
 * @note the buffer will average the value for this amount of time, the number of averaged values depends on processing load
 */
void XRTLinput::averageTime(int64_t measurementTime) {
    averageMicroSeconds = 1000 * measurementTime;
}

/**
 * @brief fill the buffer, average and deliver value
 * @note should be called as frequently as possible
 */
void XRTLinput::loop() {
    now = esp_timer_get_time();
    if (now < next) { // average time not reached, keep going
        buffer += analogReadMilliVolts(pin);
        sampleCount++;
        return;
    } else { // average time over, store averaged value
        next = now + averageMicroSeconds;
        if (sampleCount == 0) { // no measurment taken, fill buffer with single value
            buffer += analogReadMilliVolts(pin);
            sampleCount++;
        }
        voltage = ((double)buffer) / ((double)sampleCount);
        buffer = 0;
        sampleCount = 0;
    }
}

/**
 * @brief deliver the last averaged value
 * @returns voltage in mV as float
 */
double XRTLinput::readMilliVolts() {
    return voltage;
}