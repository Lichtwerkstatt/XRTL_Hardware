#include "InfoLED.h"

/* void InfoLED::begin()
{
    led.begin();
}

void InfoLED::cycle(uint32_t t, bool b = false)
{ // t: time before stepping to the next LED; b: true -> LEDs stay on after full cycle
    interval = t;
    continueCycle = b;
    isCycling = true;
    isPulsing = false;
}

void InfoLED::pulse(uint32_t t, uint8_t minimum, uint8_t maximum)
{ // t: time before next intensity value; minimum: minimum intensity; maximum: maximum intensity
    interval = t;
    minVal = minimum;
    maxVal = maximum;
    isCycling = false;
    isPulsing = true;
}

void InfoLED::rainbow(uint16_t h)
{ // h: hue is incremented by this amount per step
    hueIncrement = h;
    isHueing = true;
    if (isCycling)
    {
        isPulsing = false;
    }
    if (isPulsing)
    {
        isCycling = false;
    }
}

void InfoLED::start()
{
    isOn = true;
}

void InfoLED::stop()
{
    isOn = false;
    led.clear();
    led.show();
}

void InfoLED::hsv(uint16_t h, uint8_t s, uint8_t v)
{
    isCycling = false;
    isHueing = false;
    isPulsing = false;
    hue = h;
    sat = s;
    val = v;
}

void InfoLED::constant()
{
    isCycling = false;
    isHueing = false;
    isPulsing = false;
}

void InfoLED::loop()
{
    if (!isOn)
    {
        led.clear();
        return;
    }
    
    now = millis() - last;
    if ((isCycling) && (now > interval))
    {
        if (currentLED == led.numPixels())
        {
            currentLED = 0;
            if (!continueCycle)
            {
                led.clear();
            }
        }
        led.setPixelColor(currentLED, led.gamma32(led.ColorHSV(hue, sat, val)));
        currentLED++;
        last = millis();
    }

    if (isHueing)
    {
        hue = hue + hueIncrement;
    }

    if ((isPulsing) && (now > interval))
    {
        if (val == minVal)
        {
            deltaVal = 1;
        }
        if (val == maxVal)
        {
            deltaVal = -1;
        }
        val += deltaVal;
        last = millis();
    }

    if (!isCycling)
    {
        for (int i = 0; i < led.numPixels(); i++)
        {
            led.setPixelColor(i, led.gamma32(led.ColorHSV(hue, sat, val)));
        }
    }
    
    led.show();
} */

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

void InfoLED::constant(uint16_t duration)
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

void InfoLED::pulse(uint8_t minVal, uint8_t maxVal, uint16_t pulseTime, uint16_t pulseCount)
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

    pulseTimer = pulseTime * pulseCount * 1000 + esp_timer_get_time();

    updatesQueued = true;
    mode = pulsing;
}

void InfoLED::cycle(uint64_t cycleDuration)
{
    cycleDuration *= 1000;
    cycleDuration /= led.numPixels();

    operationInterval = cycleDuration;

    led.clear();
    currentLED = 0;
    led.setPixelColor(currentLED++, currentColor());
    led.show();

    nextOperation = esp_timer_get_time() + operationInterval;
    
    updatesQueued = true;
    mode = cycling;
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
        case cycling:
        {
            led.setPixelColor(currentLED++, currentColor());
            if (currentLED >= led.numPixels())
                mode = on;

            nextOperation = now + operationInterval;
            break;
        }
    }

    led.show();
}