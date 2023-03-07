#ifndef MAPVALUECONV_H
#define MAPVALUECONV_H

#include "modules/input/conversions/Converter.h"

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

#endif