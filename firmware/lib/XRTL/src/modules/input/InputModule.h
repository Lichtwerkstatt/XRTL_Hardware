#ifndef INPUTMODULE_H
#define INPUTMODULE_H

#include "XRTLinput.h"
#include "conversions/InputConverter.h"
#include "conversions/map/MapValue.h"
#include "conversions/multiply/Multiplication.h"
#include "conversions/offset/Offset.h"
#include "conversions/resdiv/ResistanceDivider.h"
#include "conversions/thermistor/Thermistor.h"
#include "conversions/voltdiv/VoltageDivider.h"

class InputModule : public XRTLmodule {
private:
    XRTLinput *input = NULL;
    // number of conversions currently applied to the input.
    // maximum number: 16
    uint8_t conversionCount = 0;
    conversion_t conversionType[16]; // array that holds the type of each conversion
    InputConverter *conversion[16];  // array that holds the pointers to the individual instances of the conversion class

    // number of the physical input pin
    // WARNING: ADC2 cannot be used when WiFi is active. Be aware of your board limitations.
    uint8_t pin = 35;
    uint16_t averageTime = 0; // time that the voltage value is averaged for in milli seconds

    bool isStreaming = false;
    int64_t next;
    uint32_t intervalMicroSeconds = 1000000; // TODO: add interface for streaming interval

    bool rangeChecking = false;
    bool isBinary = false;
    double loBound = 0.0;    // lowest ADC output: 142 mV, 0 will never get triggered
    double hiBound = 3300.0; // voltage reference, will never get triggered due to cut off typically around 3150 mV
    uint32_t deadMicroSeconds = 0;
    int64_t nextCheck;

    double value;
    bool lastState;

public:
    ParameterPack convParameters;

    InputModule(String moduleName);
    ~InputModule();
    moduleType type = xrtl_input;

    void setup();
    void loop();
    void stop();

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    bool conversionDialog();
    bool dialog();
    void setViaSerial();
    bool getStatus(JsonObject &status);

    void startStreaming();
    void stopStreaming();

    void handleCommand(String &controlId, JsonObject &command);

    void handleInternal(internalEvent eventId, String &sourceId);

    void addConversion(conversion_t conversion);
};

#endif