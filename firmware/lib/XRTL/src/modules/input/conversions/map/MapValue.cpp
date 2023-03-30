#include "MapValue.h"

MapValue::MapValue() {
    type = map_value;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(inMin, "inMin", "float");
    parameters.add(inMax, "inMax", "float");
    parameters.add(outMin, "outMin", "float");
    parameters.add(outMax, "outMax", "float");
}

void MapValue::saveSettings(JsonObject& settings) {
    /*JsonObject saving = settings.createNestedObject();
    saving["type"] = map_value;
    saving["inMin"] = inMin;
    saving["inMax"] = inMax;
    saving["outMin"] = outMin;
    saving["outMax"] = outMax;*/
    parameters.save(settings);
}

void MapValue::loadSettings(JsonObject& settings, bool debugMode) {
    /*inMin = loadValue<double>("inMin", settings, (double) 0);
    inMax = loadValue<double>("inMax", settings, (double) 3300);
    outMin = loadValue<double>("outMin", settings, (double) 0);
    outMax = loadValue<double>("outMax", settings, (double) 5000);

    if (!debugMode) return;
    Serial.println(centerString("map conversion", 39, ' '));
    Serial.printf("input minimum: %f\n", inMin);
    Serial.printf("input maximum: %f\n", inMax);
    Serial.printf("output minimum: %f\n", outMin);
    Serial.printf("output maximum: %f\n", outMax);*/
    parameters.load(settings);
    if (debugMode) parameters.print();
}

void MapValue::setViaSerial() {
    /*inMin = serialInput("input minimum: ").toDouble();
    inMax = serialInput("input maximum: ").toDouble();
    outMin = serialInput("output minimum: ").toDouble();
    outMax = serialInput("output maximum: ").toDouble();*/
    parameters.setViaSerial();
}

void MapValue::convert(double& value) {
    value = (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}