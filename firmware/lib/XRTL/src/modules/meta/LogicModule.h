#ifndef LOGICMODULE_H
#define LOGICMODULE_H

#include "XRTLmodule.h"
#include "commands/MacroState.h"

class InternalEvent {
    private:
    internalEvent eventId;
    String sourceId;

    public:
    void handle(internalEvent currentEvent, String& responsibleModule);
};

class SocketEvent {

};

class LogicModule : public XRTLmodule {
    private:
    InternalEvent* internalEvents[16];
    SocketEvent* socketEvents[16];

    public:
    LogicModule(String moduleName, XRTL* source);

    void handleInternal(internalEvent eventId, String& sourceId);

    void handleCommand(String& controlId, JsonObject& command);
};

#endif