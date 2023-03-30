#ifndef XRTLINTERNALHOOK_H
#define XRTLINTERNALHOOK_H

#include "modules/XRTLmodule.h"

class InternalHook {
    private:

    internalEvent eventType = busy;
    String listeningId= "";
    XRTLcommand* command = NULL;

    public:
    void setCommand(String& controlId, String& controlKey, JsonVariant& controlVal);

    bool isTriggered(internalEvent& eventId, String& sourceId);
    String& getId();
    void fillCommand(JsonObject& commandObj);
    XRTLcommand& getCommand();

    void save(JsonObject& settings);
    void load(JsonObject& settings);
};

#endif