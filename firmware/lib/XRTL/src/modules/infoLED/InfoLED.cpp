#include "InfoLED.h"

/**
 * @brief wrapper for the Adafruit NeoPixel family, providing simple functions for common display tasks
 * @param pixelCount number of LEDs on the device
 * @param pin pin the device is connected to
*/
InfoLED::InfoLED(uint8_t pixelCount, uint8_t pin) : led(pixelCount, pin, NEO_GRB + NEO_KHZ800)
{
    nextOperation = esp_timer_get_time();
};

/**
 * @brief current color, formated as uint32 as used in the NeoPixel library
*/
uint32_t InfoLED::currentColor()
{
    return led.gamma32(led.ColorHSV(hue, sat, val));
}

/**
 * @brief initialize the LED chain
*/
void InfoLED::begin()
{
    led.begin();
}


/**
 * @brief pause all current patterns and turn all LEDs off
*/
void InfoLED::stop()
{
    led.clear();
    led.show();
    updatesQueued = false;
}

/**
 * @brief set the current color to specified one in HSV color space
 * @param newHue hue of the new color (uint16)
 * @param newSat saturation of the new color (uint8)
 * @param newVal value (intensity) of the new color (uint8)
*/
void InfoLED::hsv(uint16_t newHue, uint8_t newSat, uint8_t newVal)
{
    hue = newHue;
    sat = newSat;
    val = newVal;

    fill();
}

/**
 * @brief change the holding characteristics after a pattern expires
 * @param newHold freeze pattern after expiration (true) or turn all LEDs off (false)
 * @note by default all LEDs will turn off after a pattern reached its end. Set hold to true to keep the LEDs on after the last step.
*/
void InfoLED::hold(bool newHold)
{
    isHolding = newHold;
    updatesQueued = true;
}

/**
 * @brief indefinetly turn all LEDs on with the last set color
*/
void InfoLED::constant()
{
    led.show();
    updatesQueued = false;
    mode = on;
}

/**
 * @brief turn all LEDs on with the last set color for a limited time
 * @param duration time after which the LEDs will turn off again (in ms)
 * @note if hold is activated the pattern will persist
*/
void InfoLED::constant(uint32_t duration)
{
    led.show();
    updatesQueued = true;
    nextOperation = esp_timer_get_time() + duration * 1000;
    mode = on;
}

/**
 * @brief fill the LED chain with the last set color
 * @param fillAll if set to true all LEDs will light up regardless of the currently stored active LED
 * @note if fillAll is false, the LEDs will only light up until the currently active LED is reached.
*/
void InfoLED::fill(bool fillAll)
{
    if (fillAll)
        currentLED = led.numPixels();

    for (int i = 0; i < currentLED; i++)
    {
        led.setPixelColor(i, currentColor());
    }
}

/**
 * @brief let the entire LED chain pulse in intensity
 * @param minVal minimum intensity the LEDs will reach during pulse
 * @param maxVal maximum intensity the LEDs will reach during pulse
 * @param pulseTime pulse duration in ms -> time between equal intensity of the LEDs
 * @param pulsingDuration duration of the pulsing sequence in ms
 * @note pulseTime controls the periodicity of the pulsing sequence, while pulsingDuration controls the overall length
*/
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

/**
 * @brief fill the entire chain with a constant color sequentially in either direction
 * @param cycleDuration time after which the entire chain is filled and will turn off again
 * @note only the absolute value of cycleDuration is used as duration. If cycleDuration is negative, the chain will fill from right to left (counter clockwise), if positive from left to right (clockwise)
*/
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

/**
 * @brief check if a timed pattern needs an update and set the LEDs accordingly
 * @note should be called as frequently as possible, or at least as fast as a pattern is supposed to change
*/
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