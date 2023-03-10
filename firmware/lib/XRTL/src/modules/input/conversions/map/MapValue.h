#ifndef MAPVALUECONV_H
#define MAPVALUECONV_H

#include "modules/input/conversions/Converter.h"

// @brief linearly convert a value from a specified input range to a specified output range
class MapValue: public Converter {
    private:
    // @brief minimal value of the input range.
    // @note input value must never subceed this value.
    // unit of measurement must be identical with input and inMax.
    double inMin;
    // @brief maximal value of the input range.
    // @note input value must never exceed this value.
    // unit of measurement must be identical with input and inMin.
    double inMax;
    // @brief minimal value of the output range.
    // @note output value will never subceed this value.
    // unit of measurement must be identical with outMax.
    double outMin;
    // @brief maximal value of the output range.
    // @note output value will never exceed this value.
    // unit of measurement must be identical with inMax.
    double outMax;

    public:

    void saveSettings(JsonArray& settings);
    void loadSettings(JsonObject& settings, bool debugMode);
    void setViaSerial();

    void convert(double& value);
};

#endif