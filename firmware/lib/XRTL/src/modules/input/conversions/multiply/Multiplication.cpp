#include "Multiplication.h"

Multiplication::Multiplication()
{
    type = multiplication;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(multiplicator, "multiplicator", "float");
}

void Multiplication::loadSettings(JsonObject &settings, bool debugMode)
{
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void Multiplication::saveSettings(JsonObject &settings)
{
    parameters.save(settings);
}

void Multiplication::setViaSerial()
{
    parameters.setViaSerial();
}

void Multiplication::convert(double &value)
{
    value = value * multiplicator;
}