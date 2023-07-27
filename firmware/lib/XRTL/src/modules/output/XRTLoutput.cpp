#include "XRTLoutput.h"

XRTLoutput::XRTLoutput(bool isPWM) {
    pwm = isPWM;
}

/**
 * @brief initialize pin as digital output pin
 * @param controlPin pin number of output
 * @note for PWM control use different attach() methode
 */
void XRTLoutput::attach(uint8_t controlPin) {
    pin = controlPin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

/**
 * @brief initialize pin as PWM output pin
 * @param controlPin pin number of output
 * @param pwmChannel channel of the PWM controler to use
 * @param pwmFrequency frequency of the PWM signal in Hz
 * @returns true if the 
 * @note not all pins support PWM; for digital output use different attach() methode
 */
bool XRTLoutput::attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency) {
    pin = controlPin;
    channel = pwmChannel;
    frequency = pwmFrequency;
    uint32_t readFrequency = ledcSetup(channel, frequency, 8); // 8 bit resolution -> steps in percentage: 1/255 = 0.39%
    pinMode(pin, OUTPUT);
    ledcAttachPin(pin, channel);

    return (readFrequency != 0);
}

/**
 * @brief turn the output completely on or off
 * @param targetState boolean; turn output on if true and off if false
 * @note output might still be effectively off after setting to true if powerLvl == 0
 */
void XRTLoutput::toggle(bool targetState) {
    // if (targetState == state) return; // prevents update of powerlevel only

    if (targetState) {
        if (pwm) {
            ledcWrite(channel, power);
        } else {
            digitalWrite(pin, HIGH);
        }
        state = true;
    } else {
        if (pwm) {
            ledcWrite(channel, 0);
        } else {
            digitalWrite(pin, LOW);
        }
        state = false;
    }
}

/**
 * @brief set the powerlevel of the pwm output
 * @param powerLvl new power level
 * @note output might still be off after setting a nonzero powerlevel if toggle() is set to false
 */
void XRTLoutput::write(uint8_t powerLvl) {
    if (!pwm)
        return;
    power = powerLvl;
    toggle(state); // update powerlevel
}

/**
 * @brief get the current pwm powerlevel
 * @returns powerlevel as 8-bit integer
 */
uint8_t XRTLoutput::read() {
    if (!pwm) {
        if (state)
            return 255;
        else
            return 0;
    }
    return power;
}

/**
 * @brief get the current output state as boolean
 * @returns true if output is on; false if output is off
 */
bool XRTLoutput::getState() {
    return state;
}
