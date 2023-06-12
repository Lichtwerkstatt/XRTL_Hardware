#ifndef INFOLED_H
#define INFOLED_H

#include "common/XRTLfunctions.h"
#include "Adafruit_NeoPixel.h"

/* class InfoLED
{
public:
    InfoLED(uint8_t n, uint8_t pin) : led(n, pin, NEO_GRB + NEO_KHZ800) {}
    Adafruit_NeoPixel led;
    uint8_t currentLED = 12;
    uint32_t interval = 200;
    uint32_t now;
    uint32_t last;

    uint16_t hue;
    uint8_t sat = 255;
    uint8_t val = 255;

    uint16_t hueIncrement = 10;

    uint8_t deltaVal = 1;
    uint8_t maxVal = 255;
    uint8_t minVal = 0;

    bool isOn = false;
    bool isPulsing = false;
    bool isCycling = false;
    bool isHueing = false;
    bool continueCycle = false;

    void begin();

    // let a value "run" over the LED chain
    void cycle(uint32_t t, bool b);

    // pulse brightness between minimum and maximum
    void pulse(uint32_t t, uint8_t minimum, uint8_t maximum);

    // gradually change hue, can be called with cycle() or pulse() but not both simultaneously
    void rainbow(uint16_t h);

    // enables the LEDs, also continues patterns after stop() is called
    void start();

    // turns off all LEDs until start is called
    void stop();

    // set a constant HSV to be shown
    void hsv(uint16_t h, uint8_t s, uint8_t v);

    // disables all dynamics, whatever HSV was shown last remains
    void constant();

    // check the selected pattern and change the corresponding values if necessary
    // must be called frequently whenever a dynamic light pattern is to be displayed
    void loop();
}; */

enum ledOperation_t
{
    on,
    pulsing,
    cycling
};

class InfoLED
{
private:
    Adafruit_NeoPixel led;

    bool updatesQueued = false;
    bool isHolding = false;
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
    void constant(uint16_t duration);

    void fill(bool fillAll = false);

    void pulse(uint8_t minVal = 40, uint8_t maxVal = 110, uint16_t pulseTime = 1000, uint16_t pulsingDuration = 2000);
    void cycle(uint64_t cycleDuration);

    void loop();
};

#endif