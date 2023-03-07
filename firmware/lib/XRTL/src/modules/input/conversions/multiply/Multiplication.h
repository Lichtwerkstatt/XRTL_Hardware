#ifndef MULTIPLICATIONCONV_H
#define MULTIPLICATIONCONV_H

#include "modules/input/conversions/Converter.h"

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

#endif