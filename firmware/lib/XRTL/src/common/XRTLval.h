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
    xrtlVal_t currentType = xrtlval_bool;

    bool valBool = false;
    int  valInt;
    double valFloat;
    String valString;

    void clear();

    public:
    void passValue(String& key, JsonObject& command);
    void set(JsonVariant& newVal);
};



#endif