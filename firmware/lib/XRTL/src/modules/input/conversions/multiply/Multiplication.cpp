#include "Multiplication.h"

void Multiplication::loadSettings(JsonObject& settings, bool debugMode) {
    multiplicator = loadValue<double>("multiplicator", settings, (double) 1);

    if (!debugMode) return;
    Serial.println(centerString("multiplicator conversion", 39, ' '));
    Serial.printf("multiplicator: %f\n", multiplicator);
}

void Multiplication::saveSettings(JsonArray& settings) {
    JsonObject saving = settings.createNestedObject();
    saving["type"] = multiplication;
    saving["multiplicator"] = multiplicator;
}

void Multiplication::setViaSerial() {
    multiplicator = serialInput("multiplicator: ").toDouble();
}

void Multiplication::convert(double& value) {
    value = value * multiplicator;
}