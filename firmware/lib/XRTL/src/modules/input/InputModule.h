#ifndef INPUTMODULE_H
#define INPUTMODULE_H

#include "conversions/Converter.h"
#include "conversions/resdiv/ResistanceDivider.h"
#include "conversions/thermistor/Thermistor.h"
#include "conversions/map/MapValue.h"
#include "conversions/offset/Offset.h"
#include "conversions/multiply/Multiplication.h"

// averaging and storing input value on an input pin
class XRTLinput {
    private:
    uint8_t pin = 35;

    int64_t averageMicroSeconds = 100000;
    int64_t now = 0;
    int64_t next = 0;

    uint16_t sampleCount = 0;
    //buffer used for averaging values, 64 bit should prevent overflows for roughly 60 years at normal clock speed
    uint64_t buffer = 0;
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
    // number of conversions currently applied to the input.
    // maximum number: 16
    uint8_t conversionCount = 0;
    conversion_t conversionType[16]; // array that holds the type of each conversion
    Converter* conversion[16]; // array that holds the pointers to the individual instances of the conversion class

    // number of the physical input pin
    // WARNING: ADC2 cannot be used when WiFi is active. Be aware of your board limitations.
    uint8_t pin;
    uint16_t averageTime; // time that the voltage value is averaged for in milli seconds

    bool isStreaming = false;
    int64_t next;
    uint32_t intervalMicroSeconds = 1000000; // TODO: add interface for streaming interval

    bool rangeChecking = false;
    bool relayViolations = false; // True: boundary violations will be reported to server as status; False: violations are errors
    double loBound = 0.0; // lowest ADC output: 142 mV, 0 will never get triggered
    double hiBound = 3300.0; // voltage reference, will never get triggered due to cut off typically around 3150 mV
    uint32_t deadMicroSeconds = 0;
    int64_t nextCheck;

    // TODO: add option to inform server of every status change
    bool lastState;

    public:

    InputModule(String moduleName, XRTL* source);
    moduleType getType();

    void setup();
    void loop();
    void stop();

    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
    bool getStatus(JsonObject& status);

    void startStreaming();
    void stopStreaming();

    void handleCommand(String& controlId, JsonObject& command);
    bool handleCommand(String& command);

    void handleInternal(internalEvent eventId, String& sourceId);

    // add a conversion that is applied to every value before it is delivered
    void addConversion(conversion_t conversion);
};

#endif