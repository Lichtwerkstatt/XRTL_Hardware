#ifndef OFFSETCONV_H
#define OFFSETCONV_H

#include "modules/input/conversions/InputConverter.h"

// @brief add a specified (positive or negative) offset to the input
class Offset : public InputConverter
{
private:
    // @brief value that is added to the input.
    // @note positive and negative values possible.
    // unit of measurement must be identical to input.
    double offsetValue;

public:
    Offset();

    void saveSettings(JsonObject &settings);
    void setViaSerial();
    void loadSettings(JsonObject &settings, bool debugMode);

    void convert(double &value);
};

#endif