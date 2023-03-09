#ifndef XRTLFUNCTIONS_H
#define XRTLFUNCTIONS_H

#include "Arduino.h"
#include "ArduinoJson.h"
#include "LittleFS.h"

// @brief safely load a setting value from a file
// @param key name of the key that is searched for
// @param file will be searched for key
// @param defaultValue return this value if key is missing
// @returns value read from file or defaultValue if key is missing
// @note This function makes sure a value is always provided but gives no feedback whether the default was used. For a version with feedback use XRTLmodule::getValue()
template <typename T>
T loadValue(String key, JsonObject& file, T defaultValue) {
  auto field = file[key];
  if ( field.isNull() ) {
    Serial.printf("WARNING: %s not set, using default value\n", key.c_str());
    return defaultValue;
  }
  return field.as<T>();
}

// @brief poll user input via serial monitor
// @param query user will be presented with this string before the program waits for input
// @returns user input until the first return character is received
String serialInput(String query);

// @brief center str on a line, fill remaining space with characters
// @param str string to be centered
// @param targetLength total length of the line
// @param filler remaining space is filled with this char
// @note example: filler + =>  ++++str++++
String centerString(String str, uint8_t targetLength, char filler);

// @brief rescale floats, analogue to map
// @param x value to be mapped
// @param in_min minimum value of input range
// @param in_max maximum value of input range
// @param out_min minimum value of output range
// @param out_max maximum value of output range
// @returns value linearly mapped to output range
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

#endif