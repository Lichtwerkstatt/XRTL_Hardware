#ifndef THERMISTORCONV_H
#define THERMISTORCONV_H

#include "modules/input/conversions/Converter.h"

// convert a resistance into temperature by use of an NTC
class Thermistor: public Converter {
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

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = thermistor;
        saving["tempNormal"] = tempNormal;
        saving["resNormal"] = resNormal;
        saving["beta"] = beta;
    }

    void loadSettings(JsonObject& settings, bool debugMode) {
        tempNormal = loadValue<double>("tempNormal", settings, (double) 298.15);
        resNormal = loadValue<double>("resNormal", settings, (double) 10);
        beta = loadValue<double>("beta", settings, (double) 3750);

        if (!debugMode) return;
        Serial.println(centerString("thermistor conversion", 39, ' '));
        Serial.printf("normal temperature: %f\n", tempNormal);
        Serial.printf("normal resistance: %f\n", resNormal);
        Serial.printf("beta: %f\n", beta);
    }

    void setViaSerial() {
        tempNormal = serialInput("normal temperature (K): ").toDouble();
        resNormal = serialInput("normal resistance: ").toDouble();
        beta = serialInput("beta (K): ").toDouble();
    }

    void convert(double& value) {
        value = 1 / ( log( value / resNormal ) / beta + (1 / tempNormal) );
    }
};

#endif