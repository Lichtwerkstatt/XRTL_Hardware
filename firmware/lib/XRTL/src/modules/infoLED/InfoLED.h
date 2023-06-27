#ifndef INFOLED_H
#define INFOLED_H

#include "common/XRTLfunctions.h"
#include "Adafruit_NeoPixel.h"

// operating mode of the led
enum ledOperation_t
{
    on, //constant pattern, turns off automatically if not in hold
    pulsing,
    cyclingCW,
    cyclingCCW
};

class InfoLED
{
private:
    Adafruit_NeoPixel led;

    bool updatesQueued = false; // only true if an update is expected in the next loop
    bool isHolding = false; // if true: light is kept on after last update
    ledOperation_t mode = on;
    uint8_t currentLED = 0;
    int64_t nextOperation;
    int64_t operationInterval;

    uint16_t hue;
    uint8_t sat = 255;
    uint8_t val = 255;

    bool pulseDirection = true;
    uint8_t minPulseVal = 0;
    uint8_t maxPulseVal = 255;
    int64_t pulseTimer = 0;

    uint32_t currentColor();

public:
    InfoLED(uint8_t pixelCount, uint8_t pin);

    void begin();
    void stop();

    void hold(bool newHold = true);
    void hsv(uint16_t newHue, uint8_t newSat, uint8_t newVal);
    void constant();
    void constant(uint32_t duration);

    void fill(bool fillAll = false);

    void pulse(uint8_t minVal = 40, uint8_t maxVal = 110, uint16_t pulseTime = 1000, uint32_t pulsingDuration = 2000);
    //void cycle(uint64_t cycleDuration);
    void cycle(int64_t cycleDuration);

    void loop();
};

#endif