#include "Thermistor.h"

Thermistor::Thermistor()
{
    type = thermistor;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(tempNormal, "tempNormal", "K");
    parameters.add(resNormal, "resNormal", "k|Ohm");
    parameters.add(beta, "beta", "K");
}

void Thermistor::saveSettings(JsonObject &settings)
{
    /*JsonObject saving = settings.createNestedObject();
    saving["type"] = thermistor;
    saving["tempNormal"] = tempNormal;
    saving["resNormal"] = resNormal;
    saving["beta"] = beta;*/
    parameters.save(settings);
}

void Thermistor::loadSettings(JsonObject &settings, bool debugMode)
{
    /*tempNormal = loadValue<double>("tempNormal", settings, (double) 298.15);
    resNormal = loadValue<double>("resNormal", settings, (double) 10);
    beta = loadValue<double>("beta", settings, (double) 3750);

    if (!debugMode) return;
    Serial.println(centerString("thermistor conversion", 39, ' '));
    Serial.printf("normal temperature: %f\n", tempNormal);
    Serial.printf("normal resistance: %f\n", resNormal);
    Serial.printf("beta: %f\n", beta);*/
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void Thermistor::setViaSerial()
{
    /*tempNormal = serialInput("normal temperature (K): ").toDouble();
    resNormal = serialInput("normal resistance: ").toDouble();
    beta = serialInput("beta (K): ").toDouble();*/
    parameters.setViaSerial();
}

void Thermistor::convert(double &value)
{
    value = 1 / (log(value / resNormal) / beta + (1 / tempNormal));
}