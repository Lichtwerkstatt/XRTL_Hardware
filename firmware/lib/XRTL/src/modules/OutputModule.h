#ifndef OUTPUTMODULE_H
#define OUTPUTMODULE_H

#include "XRTLmodule.h"

class XRTLoutput {
    private:
    bool state = false;
    uint8_t power = 0;

    uint8_t pin;
    bool pwm;
    uint8_t channel;
    uint16_t frequency;

    public:

    XRTLoutput(bool isPWM);

    void attach(uint8_t controlPin);
    void attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency);
    // turn on/off
    void toggle(bool targetState);
    // set powerlevel for PWM
    void write(uint8_t powerLvl);
    uint8_t read();
    bool getState();

};

// XRTLmodule for controling output signals, either relays or MOSFETs operated via PWM
class OutputModule: public XRTLmodule {
    private:

    uint8_t pin;

    // controls the output behavior
    // true: output is 8 bit pwm
    // false: output is a relay
    bool pwm = false;
    // pwm channel of the ESP used
    // WARNING: do NOT use channel 1 or 2 with ESP32CAM
    uint8_t channel;
    // pwm frequency in Hz, permitted values: 1000, 5000, 8000, 10000 
    uint16_t frequency;

    // esp_timer value (µs) at which the module is switched off again
    int64_t switchTime = 0;

    // ID (string) of the guarded module
    // trigger events of this module will cause immediate powerdown of the output
    String guardedModule = "";

    XRTLoutput* out = NULL; 

    public:

    OutputModule(String moduleName, XRTL* source);
    moduleType getType();

    void pulse(uint16_t milliSeconds);

    void setup();
    void loop();
    void stop();

    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
    bool getStatus(JsonObject& status);

    void handleInternal(internalEvent eventId, String& sourceId);
    void handleCommand(String& controlId, JsonObject& command);
};

#endif