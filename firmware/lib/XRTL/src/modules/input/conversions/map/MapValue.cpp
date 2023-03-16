#include "MapValue.h"

void MapValue::saveSettings(JsonArray& settings) {
    JsonObject saving = settings.createNestedObject();
    saving["type"] = map_value;
    saving["inMin"] = inMin;
    saving["inMax"] = inMax;
    saving["outMin"] = outMin;
    saving["outMax"] = outMax;
}

void MapValue::loadSettings(JsonObject& settings, bool debugMode) {
    inMin = loadValue<double>("inMin", settings, (double) 0);
    inMax = loadValue<double>("inMax", settings, (double) 3300);
    outMin = loadValue<double>("outMin", settings, (double) 0);
    outMax = loadValue<double>("outMax", settings, (double) 5000);

    if (!debugMode) return;
    Serial.println(centerString("map conversion", 39, ' '));
    Serial.printf("input minimum: %f\n", inMin);
    Serial.printf("input maximum: %f\n", inMax);
    Serial.printf("output minimum: %f\n", outMin);
    Serial.printf("output maximum: %f\n", outMax);
}

void MapValue::setViaSerial() {
    inMin = serialInput("input minimum: ").toDouble();
    inMax = serialInput("input maximum: ").toDouble();
    outMin = serialInput("output minimum: ").toDouble();
    outMax = serialInput("output maximum: ").toDouble();
}

void MapValue::convert(double& value) {
    value = (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}