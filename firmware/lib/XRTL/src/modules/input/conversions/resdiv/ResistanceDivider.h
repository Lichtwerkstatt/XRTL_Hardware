#ifndef RESISTANCEDIVIDERCONV_H
#define RESISTANCEDIVIDERCONV_H

#include "modules/input/conversions/Converter.h"

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
class ResistanceDivider: public Converter {
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

    void saveSettings(JsonArray& settings) {
        JsonObject saving = settings.createNestedObject();
        saving["type"] = resistance_voltage_divider;
        saving["refVoltage"] = refVoltage;
        saving["refResistor"] = refResistor;
    }

    void loadSettings(JsonObject& settings, bool debugMode) {
        refVoltage = loadValue<double>("refVoltage", settings, (double) 3300);
        refResistor = loadValue<double>("refResistor", settings, (double) 3);

        if (!debugMode) return;
        Serial.println(centerString("resistance conversion", 39, ' '));
        Serial.printf("reference voltage: %f\n", refVoltage);
        Serial.printf("reference resistor: %f\n", refResistor);
    }

    void setViaSerial(){
        refVoltage = serialInput("reference Voltage (mV): ").toDouble();
        refResistor = serialInput("reference Resistor: ").toDouble();
    }

    void convert(double& value) {
        value = refResistor * value / (refVoltage - value);
    }
};

#endif