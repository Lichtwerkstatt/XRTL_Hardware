#include "XRTLoutput.h"

XRTLoutput::XRTLoutput(bool isPWM)
{
    pwm = isPWM;
}

void XRTLoutput::attach(uint8_t controlPin)
{
    pin = controlPin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void XRTLoutput::attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency)
{
    pin = controlPin;
    channel = pwmChannel;
    frequency = pwmFrequency;
    ledcSetup(channel, frequency, 8); // 8 bit resolution -> steps in percentage: 1/255 = 0.39%
    pinMode(pin, OUTPUT);
    ledcAttachPin(pin, channel);
}

void XRTLoutput::toggle(bool targetState)
{
    // if (targetState == state) return; // prevents update of powerlevel only

    if (targetState)
    {
        if (pwm)
        {
            ledcWrite(channel, power);
        }
        else
        {
            digitalWrite(pin, HIGH);
        }
        state = true;
    }
    else
    {
        if (pwm)
        {
            ledcWrite(channel, 0);
        }
        else
        {
            digitalWrite(pin, LOW);
        }
        state = false;
    }
}

void XRTLoutput::write(uint8_t powerLvl)
{
    if (!pwm)
        return;
    power = powerLvl;
    toggle(state); // update powerlevel
}

uint8_t XRTLoutput::read()
{
    if (!pwm)
    {
        if (state)
            return 255;
        else
            return 0;
    }
    return power;
}

bool XRTLoutput::getState()
{
    return state;
}
