#ifndef INPUTMODULE_H
#define INPUTMODULE_H

#include "XRTLmodule.h"

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

// base class of conversions used to transform the input voltage
// class must contain convert function from double to double
// class must contain methods for handling the conversion settings
// multiple variables might be needed for a single conversion
class Converter {
    public:
    virtual void convert(double& value);

    // handle settings
    virtual void loadSettings(JsonObject& settings, bool debugMode);
    virtual void saveSettings(JsonArray& settings);
    virtual void setViaSerial();
};

// convert a voltage into a resistance by use of a simple voltage divider
// ┌---o    refVoltage 
// |
// █        refResistor
// |
// ├---o    measured voltage
// |
// █        measured resistor
// |
// └---o    Ground
// measured voltage
class ResistanceDivider: public Converter {
    private:
    // Reference voltage of the voltage divider,
    // unit of measurement must be identical to the input.
    // Without prior conversion: use mV
    double refVoltage;
    // Reference resistor used in your circuit.
    // Unit of measurement determines the output unit.
    // Example 1: 10 kΩ --> measured resistance is in kΩ
    // Example 2: 10000 Ω --> measured resitance is in Ω
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


// convert a resistance into temperature by use of an NTC
class Thermistor: public Converter {
    private:
    // Temperature at which the normal resistance was measured.
    // Unit of measurement must be identical with beta and relative to absolute zero.
    // Output unit of measurement is determined by this.
    // Strong suggestion: use Kelvin, not Rankine.
    double tempNormal;
    // Normal resistance of the NTC at normal temperature.
    // Unit of measurement must be identical to input resistance.
    double resNormal;
    // Beta value of the NTC.
    // Units must match with tempNormal.
    // Example: tempNormal = 298.15 K, beta = 3750 K
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

// linearly convert a value from a specified input range to a specified output range
class MapValue: public Converter {
    private:
    // minimal value of the input range.
    // input value must never subceed this value.
    // unit of measurement must be identical with input and inMax.
    double inMin;
    // maximal value of the input range.
    // input value must never exceed this value.
    // unit of measurement must be identical with input and inMin.
    double inMax;
    // minimal value of the output range.
    // output value will never subceed this value.
    // unit of measurement must be identical with outMax.
    double outMin;
    // maximal value of the output range.
    // output value will never exceed this value.
    // unit of measurement must be identical with inMax.
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


// add a specified (positive or negative) offset to the input
class Offset: public Converter {
    private:
    // value that is added to the input.
    // positive and negative values possible.
    // unit of measurement must be identical to input.
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

// multiply the input with a specified constant value
class Multiplication: public Converter {
    private:
    // value that the input is multiplied with.
    // keep in mind: unit of measurment of the output will be different from input unless this value is unitless
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
    double loBound = 0.0; // lowest ADC output: 142 mV, 0 will never get triggered
    double hiBound = 3300.0; // voltage reference, will never get triggered due to cut off typically around 3150 mV
    uint32_t deadMicroSeconds = 0;
    int64_t nextCheck;

    // TODO: add option to inform server of every status change
    bool autoBinaryUpdate = false;
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
    void getStatus(JsonObject& payload, JsonObject& status);

    void startStreaming();
    void stopStreaming();

    bool handleCommand(String& controlId, JsonObject& command);
    bool handleCommand(String& command);

    void handleInternal(internalEvent eventId, String& sourceId);

    // add a conversion that is applied to every value before it is delivered
    void addConversion(conversion_t conversion);
};

#endif