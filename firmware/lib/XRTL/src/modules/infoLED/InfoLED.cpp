#include "InfoLED.h"

InfoLED::InfoLED(uint8_t pixelCount, uint8_t pin) : led(pixelCount, pin, NEO_GRB + NEO_KHZ800)
{
    nextOperation = esp_timer_get_time();
};

uint32_t InfoLED::currentColor()
{
    return led.gamma32(led.ColorHSV(hue, sat, val));
}

void InfoLED::begin()
{
    led.begin();
}

void InfoLED::stop()
{
    led.clear();
    led.show();
    updatesQueued = false;
}

void InfoLED::hsv(uint16_t newHue, uint8_t newSat, uint8_t newVal)
{
    hue = newHue;
    sat = newSat;
    val = newVal;

    fill();
}

void InfoLED::hold(bool newHold)
{
    isHolding = newHold;
    updatesQueued = true;
}

void InfoLED::constant()
{
    led.show();
    updatesQueued = false;
    mode = on;
}

void InfoLED::constant(uint32_t duration)
{
    led.show();
    updatesQueued = true;
    nextOperation = esp_timer_get_time() + duration * 1000;
    mode = on;
}

void InfoLED::fill(bool fillAll)
{
    if (fillAll)
        currentLED = led.numPixels();

    for (int i = 0; i < currentLED; i++)
    {
        led.setPixelColor(i, currentColor());
    }
}

void InfoLED::pulse(uint8_t minVal, uint8_t maxVal, uint16_t pulseTime, uint32_t pulsingDuration)
{
    if (minVal == maxVal)
    {
        constant();
        return;
    }

    if (maxVal < minVal)
    {
        minPulseVal = maxVal;
        maxPulseVal = minVal;
    }
    else
    {
        minPulseVal = minVal;
        maxPulseVal = maxVal;
    }

    if (val < minPulseVal || val > maxPulseVal)
    {
        val = constrain(val, minVal, maxVal);
    }
    
    operationInterval = 500 * pulseTime; // from ms to µs factor 1000, but 2 * (maxVal - minVal) intervals => 500
    operationInterval /= maxVal - minVal;

    pulseTimer = pulsingDuration * 1000 + esp_timer_get_time();

    updatesQueued = true;
    mode = pulsing;
}

// void InfoLED::cycle(uint64_t cycleDuration)
// {
//     cycleDuration *= 1000;
//     cycleDuration /= led.numPixels();

//     operationInterval = cycleDuration;

//     led.clear();
//     currentLED = 0;
//     led.setPixelColor(currentLED++, currentColor());
//     led.show();

//     nextOperation = esp_timer_get_time() + operationInterval;
    
//     updatesQueued = true;
//     mode = cyclingCW;
// }

void InfoLED::cycle(int64_t cycleDuration)
{
    if (cycleDuration == 0)
    {
        return;
    }

    cycleDuration *= 1000;
    cycleDuration /= led.numPixels();

    operationInterval = abs(cycleDuration);

    led.clear();
    if (cycleDuration > 0)
    {
        currentLED = 0;
        led.setPixelColor(currentLED++, currentColor());
        mode = cyclingCW;
    }
    else
    {
        led.setPixelColor(0, currentColor());
        currentLED = led.numPixels() - 1;
        mode = cyclingCCW;
    }
    led.show();

    nextOperation = esp_timer_get_time() + operationInterval;  
    updatesQueued = true;
}

void InfoLED::loop()
{
    if (!updatesQueued)
        return;

    int64_t now = esp_timer_get_time();
    if (nextOperation >= now)
        return;


    switch(mode)
    {
        case on: // if this is reached a timed constant pattern has reached its life time; turn off.
        {
            if (isHolding)
            {
                updatesQueued = false;
            }
            else
            {
                led.clear();
            }

            break;
        }
        case pulsing:
        {
            if (pulseTimer > 0 && now >= pulseTimer)
            {
                led.clear();
                mode = on;
                break;
            }

            if (val >= maxPulseVal)
                pulseDirection = false;
            if (val <= minPulseVal)
                pulseDirection = true;

            if (pulseDirection)
                val++;
            else
                val--;

            fill();
            
            nextOperation = now + operationInterval;
            break;
        }
        case cyclingCW:
        {
            led.setPixelColor(currentLED++, currentColor());
            if (currentLED >= led.numPixels())
                mode = on;

            nextOperation = now + operationInterval;
            break;
        }
        case cyclingCCW:
        {
            led.setPixelColor(currentLED--, currentColor());
            if (currentLED == 0)
                mode = on;

            nextOperation = now + operationInterval;
            break;
        }
    }

    led.show(); // always push updates to LED
}