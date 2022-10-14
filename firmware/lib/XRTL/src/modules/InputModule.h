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

enum conversion_t {
    thermistor,
    resistance_voltage_divider,
    map_value,
    offset,
    multiplication
};

static const char* conversionName[5] = {
    "Thermistor",
    "Resistance by voltage divider",
    "Map",
    "Offset",
    "Multiplication"
};

// conversions may need variable values -- settings are managed similar to the XRTLmodules
class Converter {
    public:
    virtual void convert(double& value);

    // handle settings
    virtual void loadSettings(JsonObject& settings, bool debugMode);
    virtual void saveSettings(JsonArray& settings);
    virtual void setViaSerial();
};

class ResistanceDivider: public Converter {
    private:
    double refVoltage;
    double refResistor;

    public:

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = resistance_voltage_divider;
        saving["refVoltage"] = refVoltage;
        saving["refResistor"] = refResistor;
    }

    void loadSettings(JsonObject& settings, bool debugMode) {
        refVoltage = loadValue<double>("refVoltage", settings, (double) 3300);
        refResistor = loadValue<double>("refResistor", settings, (double) 3);

        if (!debugMode) return;
        Serial.println(centerString("resistance conversion", 39, ' '));
        Serial.printf("reference voltage: %f\n", refVoltage);
        Serial.printf("reference resistor: %f\n", refResistor);
    }

    void setViaSerial(){
        refVoltage = serialInput("reference Voltage (mV): ").toDouble();
        refResistor = serialInput("reference Resistor: ").toDouble();
    }

    void convert(double& value) {
        value = refResistor * value / (refVoltage - value);
    }
};

class Thermistor: public Converter {
    private:
    double tempNormal;
    double resNormal;
    double beta;

    public:

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = thermistor;
        saving["tempNormal"] = tempNormal;
        saving["resNormal"] = resNormal;
        saving["beta"] = beta;
    }

    void loadSettings(JsonObject& settings, bool debugMode) {
        tempNormal = loadValue<double>("tempNormal", settings, (double) 298.15);
        resNormal = loadValue<double>("resNormal", settings, (double) 10);
        beta = loadValue<double>("beta", settings, (double) 3750);

        if (!debugMode) return;
        Serial.println(centerString("thermistor conversion", 39, ' '));
        Serial.printf("normal temperature: %f\n", tempNormal);
        Serial.printf("normal resistance: %f\n", resNormal);
        Serial.printf("beta: %f\n", beta);
    }

    void setViaSerial() {
        tempNormal = serialInput("normal temperature (K): ").toDouble();
        resNormal = serialInput("normal resistance: ").toDouble();
        beta = serialInput("beta (K): ").toDouble();
    }

    void convert(double& value) {
        value = 1 / ( log( value / resNormal ) / beta + (1 / tempNormal) );
    }
};

class MapValue: public Converter {
    private:
    double inMin;
    double inMax;
    double outMin;
    double outMax;

    public:

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = map_value;
        saving["inMin"] = inMin;
        saving["inMax"] = inMax;
        saving["outMin"] = outMin;
        saving["outMax"] = outMax;
    }

    void loadSettings(JsonObject& settings, bool debugMode) {
        inMin = loadValue<double>("inMin", settings, (double) 0);
        inMax = loadValue<double>("inMax", settings, (double) 3300);
        outMin = loadValue<double>("outMin", settings, (double) 0);
        outMax = loadValue<double>("outMax", settings, (double) 5000);

        if (!debugMode) return;
        Serial.println(centerString("map conversion", 39, ' '));
        Serial.printf("input minimum: %f\n", inMin);
        Serial.printf("input maximum: %f\n", inMax);
        Serial.printf("output minimum: %f\n", outMin);
        Serial.printf("output maximum: %f\n", outMax);
    }

    void setViaSerial() {
        inMin = serialInput("input minimum: ").toDouble();
        inMax = serialInput("input maximum: ").toDouble();
        outMin = serialInput("output minimum: ").toDouble();
        outMax = serialInput("output maximum: ").toDouble();
    }

    void convert(double& value) {
        value = (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
    }
};

class Offset: public Converter {
    private:
    double offsetValue;

    public:

    void convert(double& value) {
        value = value + offsetValue;
    }

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = offset;
        saving["offsetValue"] = offsetValue;
    }

    void setViaSerial(){
        offsetValue = serialInput("offset: ").toDouble();
    }

    void loadSettings(JsonObject& settings, bool debugMode) {
        offsetValue = loadValue<double>("offsetValue", settings, (double) 0);

        if (!debugMode) return;
        Serial.println(centerString("offset conversion", 39, ' '));
        Serial.printf("offset: %f\n", offsetValue);
    }
};

class Multiplication: public Converter {
    private:
    double multiplicator;

    public:
    
    void loadSettings(JsonObject& settings, bool debugMode) {
        multiplicator = loadValue<double>("multiplicator", settings, (double) 1);

        if (!debugMode) return;
        Serial.println(centerString("multiplicator conversion", 39, ' '));
        Serial.printf("multiplicator: %f\n", multiplicator);
    }

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = multiplication;
        saving["multiplicator"] = multiplicator;
    }

    void setViaSerial() {
        multiplicator = serialInput("multiplicator: ").toDouble();
    }

    void convert(double& value) {
        value = value * multiplicator;
    }
};

class InputModule: public XRTLmodule {
    private:
    XRTLinput* input;
    uint8_t conversionCount = 0;
    conversion_t conversionType[8];
    Converter* conversion[8]; // store conversions here

    uint8_t pin;
    uint16_t averageTime;

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
    moduleType getType();

    void setup();
    void loop();
    void stop();

    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();

    void startStreaming();
    void stopStreaming();

    bool handleCommand(String& controlId, JsonObject& command);
    bool handleCommand(String& command);

    void handleInternal(internalEvent eventId, String& sourceId);

    void addConversion(conversion_t conversion);
};

#endif