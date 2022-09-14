#ifndef OUTPUTMODULE_H
#define OUTPUTMODULE_H

#include "XRTLmodule.h"

class XRTLoutput {
    private:
    bool state = false;
    bool pwm;
    uint8_t power;

    uint8_t pin;
    uint8_t channel;
    uint16_t frequency;

    public:

    void attach(uint8_t controlPin);
    void attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency);
    // turn on/off
    void toggle(bool targetState);
    // set powerlevel if PWM
    void set(uint8_t powerLvl);
    uint8_t read();
    bool getState();

};

class OutputModule: public XRTLmodule {
    private:
    String control;

    uint8_t pin;
    uint8_t channel;
    uint8_t frequency;

    int64_t switchTime = 300 * 1000 * 1000;



    XRTLoutput* out = new XRTLoutput; 

    public:

    OutputModule(String moduleName, XRTL* source);

    void pulse(uint16_t milliSeconds);

    void setup();
    void loop();
    void stop();

    void saveSettings(DynamicJsonDocument& settings);
    void loadSettings(DynamicJsonDocument& settings);
    void setViaSerial();
    void getStatus(JsonObject& payload, JsonObject& status);

    void handleInternal(internalEvent state);
    bool handleCommand(String& controlId, JsonObject& command);

    template<typename... Args>
    void debug(Args... args);
};

#endif