#ifndef XRTLFUNCTIONS_H
#define XRTLFUNCTIONS_H

#include "Arduino.h"
#include "ArduinoJson.h"
#include "LittleFS.h"

/**
 * @brief safely load a setting value from a file
 * @param key name of the key that is searched for
 * @param file will be searched for key
 * @param defaultValue return this value if key is missing
 * @returns value read from file or defaultValue if key is missing
 * @note This function makes sure a value is always provided but gives no feedback whether the default was used. For a version with feedback use XRTLmodule::getValue()
 */
template <typename T>
T loadValue(String key, JsonObject &file, T defaultValue) {
    auto field = file[key];
    if (field.isNull()) {
        Serial.printf("WARNING: %s not set, using default value\n", key.c_str());
        return defaultValue;
    }
    return field.as<T>();
}

String serialInput(String query);

String centerString(String str, uint8_t targetLength, char filler);

void highlightString(String str, char filler);

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

#endif