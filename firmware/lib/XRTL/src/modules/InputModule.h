#ifndef INPUTMODULE_H
#define INPUTMODULE_H

#include "XRTLmodule.h"

class XRTLinput {
    private:
    uint8_t pin = 35;

    int64_t averageMicroSeconds = 100000;
    int64_t now = 0;
    int64_t next = 0;

    uint16_t sampleCount = 0;
    uint32_t buffer = 0;
    double voltage = 0;

    public:

    void attach(uint8_t inputPin);
    void averageTime(int64_t measurementTime);
    void loop();
    double readMilliVolts();
};

class InputModule: public XRTLmodule {
    private:
    XRTLinput* input;
    uint8_t pin;
    uint16_t time;

    bool isStreaming = false;
    int64_t next;
    uint32_t intervalMicroSeconds;

    bool rangeChecking;
    double loBound = 142.0; // lowest output value of ADC
    double hiBound = 3300.0; // voltage reference

    public:

    InputModule(String moduleName, XRTL* source);

    void setup();
    void loop();

    void saveSettings(DynamicJsonDocument& settings);
    void loadSettings(DynamicJsonDocument& settings);
    void setViaSerial();

    void setStreamTimeCap(uint32_t milliSeconds);
    void startStreaming();
    void stopStreaming();

    bool handleCommand(String& controlId, JsonObject& command);
    bool handleCommand(String& command);

    void handleInternal(internalEvent event);

    template<typename... Args>
    void debug(Args... args);
};

#endif