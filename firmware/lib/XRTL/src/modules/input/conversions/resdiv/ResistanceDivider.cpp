#include "ResistanceDivider.h"

void ResistanceDivider::saveSettings(JsonArray& settings) {
    JsonObject saving = settings.createNestedObject();
    saving["type"] = resistance_voltage_divider;
    saving["refVoltage"] = refVoltage;
    saving["refResistor"] = refResistor;
}

void ResistanceDivider::loadSettings(JsonObject& settings, bool debugMode) {
    refVoltage = loadValue<double>("refVoltage", settings, (double) 3300);
    refResistor = loadValue<double>("refResistor", settings, (double) 3);

    if (!debugMode) return;
    Serial.println(centerString("resistance conversion", 39, ' '));
    Serial.printf("reference voltage: %f\n", refVoltage);
    Serial.printf("reference resistor: %f\n", refResistor);
}

void ResistanceDivider::setViaSerial(){
    refVoltage = serialInput("reference Voltage (mV): ").toDouble();
    refResistor = serialInput("reference Resistor: ").toDouble();
}

void ResistanceDivider::convert(double& value) {
    value = refResistor * value / (refVoltage - value);
}