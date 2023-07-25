#include "Offset.h"

Offset::Offset() {
    type = offset;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(offsetValue, "offsetValue", "float");
}

void Offset::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void Offset::setViaSerial() {
    parameters.setViaSerial();
}

void Offset::loadSettings(JsonObject &settings, bool debugMode) {
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void Offset::convert(double &value) {
    value = value + offsetValue;
}