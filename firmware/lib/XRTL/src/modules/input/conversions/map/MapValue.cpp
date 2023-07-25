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

void MapValue::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void MapValue::loadSettings(JsonObject &settings, bool debugMode) {
    parameters.load(settings);
    if (debugMode)
        parameters.print();
}

void MapValue::setViaSerial() {
    parameters.setViaSerial();
}

void MapValue::convert(double &value) {
    value = (value - inMin) * (outMax - outMin) / (inMax - inMin) + outMin;
}