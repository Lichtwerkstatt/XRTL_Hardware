#include "ResistanceDivider.h"

ResistanceDivider::ResistanceDivider()
{
    type = resistance_voltage_divider;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(refVoltage, "refVoltage", "m|V");
    parameters.add(refResistor, "refResistor", "k|Ohm");
}

void ResistanceDivider::saveSettings(JsonObject &settings)
{
    /*JsonObject saving = settings.createNestedObject();
    saving["type"] = resistance_voltage_divider;
    saving["refVoltage"] = refVoltage;
    saving["refResistor"] = refResistor;*/
    parameters.save(settings);
}

void ResistanceDivider::loadSettings(JsonObject &settings, bool debugMode)
{
    /*refVoltage = loadValue<double>("refVoltage", settings, (double) 3300);
    refResistor = loadValue<double>("refResistor", settings, (double) 3);

    if (!debugMode) return;
    Serial.println(centerString("resistance conversion", 39, ' '));
    Serial.printf("reference voltage: %f\n", refVoltage);
    Serial.printf("reference resistor: %f\n", refResistor);*/
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void ResistanceDivider::setViaSerial()
{
    /*refVoltage = serialInput("reference Voltage (mV): ").toDouble();
    refResistor = serialInput("reference Resistor: ").toDouble();*/
    parameters.setViaSerial();
}

void ResistanceDivider::convert(double &value)
{
    value = refResistor * value / (refVoltage - value);
}