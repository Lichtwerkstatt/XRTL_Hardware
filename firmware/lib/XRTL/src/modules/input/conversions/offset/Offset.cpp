#include "Offset.h"

Offset::Offset() {
    type = offset;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(offsetValue, "offsetValue", "float");
}

void Offset::saveSettings(JsonObject& settings) {
    /*JsonObject saving = settings.createNestedObject();
    saving["type"] = offset;
    saving["offsetValue"] = offsetValue;*/
    parameters.save(settings);
}

void Offset::setViaSerial(){
   // offsetValue = serialInput("offset: ").toDouble();
   parameters.setViaSerial();
}

void Offset::loadSettings(JsonObject& settings, bool debugMode) {
    /*offsetValue = loadValue<double>("offsetValue", settings, (double) 0);

    if (!debugMode) return;
    Serial.println(centerString("offset conversion", 39, ' '));
    Serial.printf("offset: %f\n", offsetValue);*/
    parameters.load(settings);
    if (debugMode) parameters.print();
}

void Offset::convert(double& value) {
    value = value + offsetValue;
}