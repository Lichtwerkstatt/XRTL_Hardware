#include "XRTLinternalHook.h"

bool InternalHook::isTriggered(internalEvent& eventId, String& sourceId) {
    if (eventType != eventId) return false;
    if (sourceId != listeningId && sourceId != "*") return false;
    return true;
}

void InternalHook::setCommand(String& controlId, String& controlKey, JsonVariant& controlVal) {
    command = new XRTLcommand();
    command->set(controlId, controlKey, controlVal);
}

void InternalHook::fillCommand(JsonObject& commandObj) {
    if (command == NULL) return;
    command->fillCommand(commandObj);
}

void InternalHook::save(JsonObject& settings) {
    settings["type"] = eventType;
    settings["id"] = 12;
    void* test = NULL;
    XRTLmodule* ptr = (XRTLmodule*) test;
    ptr->debug("test");
}