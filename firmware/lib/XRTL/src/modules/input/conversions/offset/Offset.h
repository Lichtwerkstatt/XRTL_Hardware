#ifndef OFFSETCONV_H
#define OFFSETCONV_H

#include "modules/input/conversions/Converter.h"

// @brief add a specified (positive or negative) offset to the input
class Offset: public Converter {
    private:
    // @brief value that is added to the input.
    // @note positive and negative values possible.
    // unit of measurement must be identical to input.
    double offsetValue;

    public:

    void saveSettings(JsonArray& settings);
    void setViaSerial();
    void loadSettings(JsonObject& settings, bool debugMode);

    void convert(double& value);
};

#endif