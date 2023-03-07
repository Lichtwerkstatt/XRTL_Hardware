#ifndef OFFSETCONV_H
#define OFFSETCONV_H

#include "modules/input/conversions/Converter.h"

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

#endif