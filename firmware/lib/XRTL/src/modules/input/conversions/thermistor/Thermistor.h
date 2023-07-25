#ifndef THERMISTORCONV_H
#define THERMISTORCONV_H

#include "modules/input/conversions/InputConverter.h"

// convert a resistance into temperature by use of an NTC
class Thermistor : public InputConverter {
private:
    // Temperature at which the normal resistance was measured.
    // Unit of measurement must be identical with beta and relative to absolute zero, output unit of measurement is determined by this.
    // Strong suggestion: use Kelvin, not Rankine.
    double tempNormal;
    // Normal resistance of the NTC at normal temperature.
    // Unit of measurement must be identical to input resistance.
    double resNormal;
    // Beta value of the NTC.
    // Units must match with tempNormal.
    // Example: tempNormal = 298.15 K, beta = 3750 K
    double beta;

public:
    Thermistor();

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings, bool debugMode);
    void setViaSerial();

    void convert(double &value);
};

#endif