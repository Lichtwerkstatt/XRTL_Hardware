#include "XRTLval.h"

void XRTLval::clear()
{
    currentType = xrtlval_bool;
    valBool = false;
    valInt = 0;
    valFloat = 0.0;
    valString = "";
}

void XRTLval::passValue(String &key, JsonObject &command)
{
    switch (currentType)
    {
    case (xrtlval_bool):
    {
        command[key] = valBool;
        return;
    }
    case (xrtlval_int):
    {
        command[key] = valInt;
        return;
    }
    case (xrtlval_float):
    {
        command[key] = valFloat;
        return;
    }
    case (xrtlval_string):
    {
        command[key] = valString;
        return;
    }
    }
}

void XRTLval::set(JsonVariant &newVal)
{
    if (newVal.is<bool>())
    {
        clear();
        currentType = xrtlval_bool;
        valBool = newVal.as<bool>();
    }
    else if (newVal.is<int>())
    {
        clear();
        currentType = xrtlval_int;
        valInt = newVal.as<int>();
    }
    else if (newVal.is<float>() || newVal.is<double>())
    {
        clear();
        currentType = xrtlval_float;
        valFloat = newVal.as<double>();
    }
    else if (newVal.is<String>())
    {
        clear();
        currentType = xrtlval_string;
        valString = newVal.as<String>();
    }
}