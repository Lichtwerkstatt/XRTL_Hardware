#include "Offset.h"

void Offset::saveSettings(JsonArray& settings) {
    JsonObject saving = settings.createNestedObject();
    saving["type"] = offset;
    saving["offsetValue"] = offsetValue;
}

void Offset::setViaSerial(){
    offsetValue = serialInput("offset: ").toDouble();
}

void Offset::loadSettings(JsonObject& settings, bool debugMode) {
    offsetValue = loadValue<double>("offsetValue", settings, (double) 0);

    if (!debugMode) return;
    Serial.println(centerString("offset conversion", 39, ' '));
    Serial.printf("offset: %f\n", offsetValue);
}

void Offset::convert(double& value) {
    value = value + offsetValue;
}