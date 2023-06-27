#ifndef XRTLVAL_H
#define XRTLVAL_H

#include "XRTLfunctions.h"

// @brief container to store the value a command might hold
class XRTLval
{
public:
    virtual void passValue(const String &key, JsonObject &command){}
};

class XRTLvalBool : public XRTLval
{
private:
    bool val;

public:
    XRTLvalBool(bool newVal);
    void passValue(const String &key, JsonObject &command);
};

class XRTLvalInt : public XRTLval
{
private:
    int val;

public:
    XRTLvalInt(int newVal);
    void passValue(const String &key, JsonObject &command);
};

class XRTLvalFloat : public XRTLval
{
private:
    double val;

public:
    XRTLvalFloat(double newVal);
    void passValue(const String &key, JsonObject &command);
};

class XRTLvalString : public XRTLval
{
private:
    String val;

public:
    XRTLvalString(String newVal);
    void passValue(const String &key, JsonObject &command);
};

#endif