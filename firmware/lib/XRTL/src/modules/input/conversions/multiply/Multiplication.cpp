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
    /*multiplicator = loadValue<double>("multiplicator", settings, (double) 1);

    if (!debugMode) return;
    Serial.println(centerString("multiplicator conversion", 39, ' '));
    Serial.printf("multiplicator: %f\n", multiplicator);*/
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void Multiplication::saveSettings(JsonObject &settings)
{
    /*JsonObject saving = settings.createNestedObject();
    saving["type"] = multiplication;
    saving["multiplicator"] = multiplicator;*/
    parameters.save(settings);
}

void Multiplication::setViaSerial()
{
    // multiplicator = serialInput("multiplicator: ").toDouble();
    parameters.setViaSerial();
}

void Multiplication::convert(double &value)
{
    value = value * multiplicator;
}