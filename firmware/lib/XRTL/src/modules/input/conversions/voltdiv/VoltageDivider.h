#ifndef VOLTAGEDIVIDERCONV_H
#define VOLTAGEDIVIDERCONV_H

#include "modules/input/conversions/InputConverter.h"

// convert a voltage into a resistance by use of a simple voltage divider
// ┌---o    converted voltage
// |
// █        R1
// |
// ├---o    measured voltage
// |
// █        R2
// |
// └---o    Ground
class VoltageDivider : public InputConverter
{
private:
    // first reference voltage of the voltage divider,
    // unit of measurement must match refTwo.
    double refOne;
    // second reference resistor used in your circuit.
    // Unit of measurement must match refOne.
    double refTwo;

public:
    VoltageDivider();

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings, bool debugMode);
    void setViaSerial();

    void convert(double &value);
};

#endif