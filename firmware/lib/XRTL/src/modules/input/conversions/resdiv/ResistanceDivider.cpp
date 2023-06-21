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
    parameters.save(settings);
}

void ResistanceDivider::loadSettings(JsonObject &settings, bool debugMode)
{
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void ResistanceDivider::setViaSerial()
{
    parameters.setViaSerial();
}

void ResistanceDivider::convert(double &value)
{
    value = refResistor * value / (refVoltage - value);
}