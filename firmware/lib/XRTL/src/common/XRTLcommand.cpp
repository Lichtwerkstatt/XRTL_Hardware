#include "XRTLcommand.h"

XRTLcommand::XRTLcommand() {
    id = "tmp";
    key = "key";
}

void XRTLcommand::set(String& controlId, String& controlKey, JsonVariant& controlVal) {
    id = controlId;
    key = controlKey;
    val.set(controlVal);
}

String& XRTLcommand::getId() {
    return id;
}

void XRTLcommand::fillCommand(JsonObject& command) {
    command["controlId"] = id;
    val.addValueToKey(key, command);
}

void XRTLcommand::saveSettings(JsonObject& settings) {
    val.addValueToKey(key, settings);
}

void XRTLcommand::setViaSerial() {
    id = serialInput("controlId: ");
    key = serialInput("control key: ");
    Serial.println("available types");
    Serial.println("0: bool");
    Serial.println("1: int");
    Serial.println("2: float");
    Serial.println("3: string");
    Serial.println("");
    StaticJsonDocument<128> readVal;
    readVal.to<JsonArray>();
    switch (serialInput("control value type: ").toInt()) {
        case (0): {
            String input = serialInput("value: ");
            readVal.add((input == "true" || input.toInt() == 1));
            JsonVariant newVal = readVal[0];
            val.set(newVal);
            break;
        }
        case (1): {
            int input = serialInput("value: ").toInt();
            readVal.add(input);
            JsonVariant newVal = readVal[0];
            val.set(newVal);
            break;
        }
        case (2): {
            double input = serialInput("value: ").toDouble();
            readVal.add(input);
            JsonVariant newVal = readVal[0];
            val.set(newVal);
            break;
        }
        case (3): {
            readVal.add(serialInput("value: "));
            JsonVariant newVal = readVal[0];
            val.set(newVal);
            break;
        }
    }
}