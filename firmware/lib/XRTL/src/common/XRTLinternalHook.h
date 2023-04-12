#ifndef XRTLINTERNALHOOK_H
#define XRTLINTERNALHOOK_H

#include "modules/XRTLmodule.h"

class InternalHook
{
private:
    internalEvent eventType = busy;
    String listeningId = "";
    XRTLcommand2 command;
    ParameterPack parameters;

public:
    InternalHook();
    bool isTriggered(internalEvent &eventId, String &sourceId);
    internalEvent &getType();
    String &getId();
    XRTLcommand &getCommand();

    void set(internalEvent &eventId, String &sourceId);
    void setViaSerial();
    void save(JsonObject &settings);
    void load(JsonObject &settings, bool &debugging);
};

#endif