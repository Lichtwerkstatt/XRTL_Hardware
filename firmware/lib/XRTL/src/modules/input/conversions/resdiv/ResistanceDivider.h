#ifndef RESISTANCEDIVIDERCONV_H
#define RESISTANCEDIVIDERCONV_H

#include "modules/input/conversions/InputConverter.h"

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
class ResistanceDivider : public InputConverter
{
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
    ResistanceDivider();

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings, bool debugMode);
    void setViaSerial();

    void convert(double &value);
};

#endif