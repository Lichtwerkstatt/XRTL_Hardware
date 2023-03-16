#ifndef CONVERTER_H
#define CONVERTER_H

#include "modules/XRTLmodule.h"

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

#endif