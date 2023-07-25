#ifndef MULTIPLICATIONCONV_H
#define MULTIPLICATIONCONV_H

#include "modules/input/conversions/InputConverter.h"

// @brief multiply the input with a specified constant value
// @note unit of measurement will be differen after conversion unless the multiplicator is unitless
class Multiplication : public InputConverter {
private:
    // @brief value that the input is multiplied with.
    // @note keep in mind: unit of measurment of the output will be different from input unless this value is unitless
    double multiplicator;

public:
    Multiplication();

    void loadSettings(JsonObject &settings, bool debugMode);
    void saveSettings(JsonObject &settings);
    void setViaSerial();

    void convert(double &value);
};

#endif