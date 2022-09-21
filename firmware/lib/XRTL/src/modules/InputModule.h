#ifndef INPUTMODULE_H
#define INPUTMODULE_H

#include "XRTLmodule.h"

// collecting and averaging 
class XRTLinput {
    private:
    uint8_t pin = 35;

    int64_t averageMicroSeconds = 100000;
    int64_t now = 0;
    int64_t next = 0;

    uint16_t sampleCount = 0;
    uint32_t buffer = 0;
    double voltage = 0.0;

    public:

    void attach(uint8_t inputPin);
    void averageTime(int64_t measurementTime);
    void loop();
    double readMilliVolts();
};

class InputModule: public XRTLmodule {
    private:
    XRTLinput* input;

    String control;
    String binaryLeadFrame;

    uint8_t pin;
    uint16_t time;

    bool isStreaming = false;
    int64_t next;
    uint32_t intervalMicroSeconds = 1000000; // TODO: add interface for streaming interval

    bool rangeChecking = false;
    double loBound = 0.0; // lowest ADC output: 142 mV, 0 will never get triggered
    double hiBound = 3300.0; // voltage reference, will never get triggered due to cut off
    uint32_t deadMicroSeconds = 0;
    int64_t nextCheck;

    public:

    InputModule(String moduleName, XRTL* source);

    void setup();
    void loop();
    void stop();

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