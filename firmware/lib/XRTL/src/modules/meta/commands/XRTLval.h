#ifndef XRTLVAL_H
#define XRTLVAL_H

#include "Arduino.h"
#include "ArduinoJson.h"

enum xrtlVal_t {
    xrtlval_bool,
    xrtlval_int,
    xrtlval_float,
    xrtlval_string
};

class XRTLval {
    private:
    xrtlVal_t currentType = xrtlval_int;
    bool valBool;
    int  valInt = 0;
    double valFloat;
    String valString;

    public:
    void addValueToKey(String& key, JsonObject& command);

    void set(JsonVariant& newVal);
};

#endif