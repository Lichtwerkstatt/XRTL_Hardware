#include "XRTLval.h"

void XRTLval::addValueToKey(String& key, JsonObject& command) {
    switch (currentType) {
        case (xrtlval_bool): {
            command[key] = valBool;
            return;
        }
        case (xrtlval_int): {
            command[key] = valInt;
            return;
        }
        case (xrtlval_float): {
            command[key] = valFloat;
            return;
        }
        case (xrtlval_string): {
            command[key] = valString;
            return;
        }
    }
}

void XRTLval::set(JsonVariant& newVal) {
    if (newVal.is<bool>()) {
        currentType = xrtlval_bool;
        valBool = newVal.as<bool>();
    }
    else if (newVal.is<int>()) {
        currentType = xrtlval_int;
        valInt = newVal.as<int>();
    }
    else if (newVal.is<float>() || newVal.is<double>()) {
        currentType = xrtlval_float;
        valFloat = newVal.as<double>();
    }
    else if (newVal.is<String>()) {
        currentType = xrtlval_string;
        valString = newVal.as<String>();
    }
}