#ifndef XRTLVAL_H
#define XRTLVAL_H

#include "XRTLfunctions.h"

enum xrtlVal_t {
    xrtlval_bool,
    xrtlval_int,
    xrtlval_float,
    xrtlval_string
};

// @brief container to store the value a command might hold, ensures type conversion to one of the four allowed types
// @note can exlusively be set by and read to JSON files because I can't get polymorphism to work anywhere close to how that library does
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