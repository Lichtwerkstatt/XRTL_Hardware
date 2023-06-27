#include "XRTLval.h"

XRTLvalBool::XRTLvalBool(bool newVal)
{
    val = newVal;
}

void XRTLvalBool::passValue(const String &key, JsonObject &command)
{
    command[key] = val;
}

XRTLvalInt::XRTLvalInt(int newVal)
{
    val = newVal;
}

void XRTLvalInt::passValue(const String &key, JsonObject &command)
{
    command[key] = val;
}

XRTLvalFloat::XRTLvalFloat(double newVal)
{
    val = newVal;
}

void XRTLvalFloat::passValue(const String &key, JsonObject &command)
{
    command[key] = val;
}

XRTLvalString::XRTLvalString(String newVal)
{
    val = newVal;
}

void XRTLvalString::passValue(const String &key, JsonObject &command)
{
    command[key] = val;
}