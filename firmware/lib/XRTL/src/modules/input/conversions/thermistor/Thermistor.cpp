#include "Thermistor.h"

Thermistor::Thermistor() {
    type = thermistor;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(tempNormal, "tempNormal", "K");
    parameters.add(resNormal, "resNormal", "k|Ohm");
    parameters.add(beta, "beta", "K");
}

void Thermistor::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void Thermistor::loadSettings(JsonObject &settings, bool debugMode) {
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void Thermistor::setViaSerial() {
    parameters.setViaSerial();
}

void Thermistor::convert(double &value) {
    value = 1 / (log(value / resNormal) / beta + (1 / tempNormal));
}