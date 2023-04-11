#ifndef XRTLVAL_H
#define XRTLVAL_H

#include "XRTLfunctions.h"

enum xrtlVal_t
{
    xrtlval_bool,
    xrtlval_int,
    xrtlval_float,
    xrtlval_string
};

// @brief container to store the value a command might hold, ensures type conversion to one of the four allowed types
// @note can exlusively be set by and read to JSON files because I can't get polymorphism to work anywhere close to how that library does
class XRTLval
{
private:
    xrtlVal_t currentType = xrtlval_bool;

    bool valBool = false;
    int valInt;
    double valFloat;
    String valString;

    void clear();

public:
    virtual void passValue(const String &key, JsonObject &command);
    void set(JsonVariant &newVal);
};

class XRTLvalBool : public XRTLval
{
private:
    bool val;

public:
    XRTLvalBool(bool newVal)
    {
        val = newVal;
    }

    void passValue(const String &key, JsonObject &command)
    {
        command[key] = val;
    }
};

class XRTLvalInt : public XRTLval
{
private:
    int val;

public:
    XRTLvalInt(int newVal)
    {
        val = newVal;
    }

    void passValue(const String &key, JsonObject &command)
    {
        command[key] = val;
    }
};

class XRTLvalFloat : public XRTLval
{
private:
    double val;

public:
    XRTLvalFloat(double newVal)
    {
        val = newVal;
    }

    void passValue(const String &key, JsonObject &command)
    {
        command[key] = val;
    }
};

class XRTLvalString : public XRTLval
{
private:
    String val;

public:
    XRTLvalString(String newVal)
    {
        val = newVal;
    }

    void passValue(const String &key, JsonObject &command)
    {
        command[key] = val;
    }
};

#endif