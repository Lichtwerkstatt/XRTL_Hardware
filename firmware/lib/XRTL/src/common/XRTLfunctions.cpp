#include "XRTLfunctions.h"

String serialInput(String query)
{
    Serial.print(query);
    while (!Serial.available())
    {
        yield();
    }
    String answer = Serial.readStringUntil('\n');
    Serial.println(answer);
    return answer;
}

String centerString(String str, uint8_t targetLength, char filler = ' ')
{
    char output[targetLength];

    uint8_t startPoint = (targetLength - str.length()) / 2 - 1;
    memset(output, filler, targetLength - 1);
    output[targetLength - 1] = '\0';
    memcpy(output + startPoint, str.c_str(), str.length());

    return String(output);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}