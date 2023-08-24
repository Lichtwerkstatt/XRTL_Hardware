#include "XRTLfunctions.h"

/**
 * @brief poll user input via serial monitor
 * @param query user will be presented with this string before the program waits for input
 * @returns user input until the first return character is received
 */
String serialInput(String query) {
    Serial.print(query);
    while (!Serial.available()) {
        yield();
    }
    String answer = sanitizeBackSpace(Serial.readStringUntil('\n'));
    Serial.println(answer);
    return answer;
}

/**
 * @brief allows for backspace in the serial input, removing it appropriately
 * @param input raw string that might contain backspace
 * @returns input string but with the backspace removed appropriately
*/
String sanitizeBackSpace(String input) {
    char output[input.length() + 1];
    int outputPos = 0;
    char currentChar;
    for (int i = 0; i < input.length(); i++) {
        currentChar = input[i];

        if (currentChar != '\b') { //encountered backspace
            output[outputPos++] = currentChar;
            continue;
        }

        if (outputPos == 0) continue; // already reached start of string

        outputPos--;
    }

    output[outputPos] = '\0'; // finalize String after input is finished
    return String(output);
}

/**
 * @brief center str on a line, fill remaining space with characters
 * @param str string to be centered
 * @param targetLength total length of the line
 * @param filler remaining space is filled with this char, defaults to space
 * @note example: filler + =>  ++++str++++
 */
String centerString(String str, uint8_t targetLength, char filler = ' ') {
    char output[targetLength];

    uint8_t startPoint = (targetLength - str.length()) / 2 - 1;
    memset(output, filler, targetLength - 1);
    output[targetLength - 1] = '\0';
    memcpy(output + startPoint, str.c_str(), str.length());

    return String(output);
}

/**
 * @brief hightlight a string in the serial monitor by centering it and framing it with a filler
 * @param str String to highlight
 * @param filler character to use as filler
 */
void highlightString(String str, char filler = '-') {
    Serial.println("");
    Serial.println(centerString("", 55, filler));
    Serial.println(centerString(str, 55));
    Serial.println(centerString("", 55, filler));
    Serial.println("");
}

/**
 * @brief rescale floats, analogue to map
 * @param x value to be mapped
 * @param in_min minimum value of input range
 * @param in_max maximum value of input range
 * @param out_min minimum value of output range
 * @param out_max maximum value of output range
 * @returns value linearly mapped to output range
 */
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}