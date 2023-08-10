#include "VoltageDivider.h"

VoltageDivider::VoltageDivider() {
    type = resistance_voltage_divider;

    parameters.setKey("");
    parameters.add(type, "type");
    parameters.add(refOne, "refOne", "k|Ohm");
    parameters.add(refTwo, "refTwo", "k|Ohm");
}

void VoltageDivider::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void VoltageDivider::loadSettings(JsonObject &settings, bool debugMode) {
    parameters.load(settings);
    if (debugMode) parameters.print();
}

void VoltageDivider::setViaSerial() {
    parameters.setViaSerial();
}

void VoltageDivider::convert(double &value) {
    value = (refOne + refTwo) / refTwo * value;
}