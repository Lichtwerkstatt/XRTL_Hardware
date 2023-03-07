#include "LogicModule.h"

LogicModule::LogicModule(String moduleName, XRTL* source) {
    xrtl = source;
    id = moduleName;
}

void handleInternal(internalEvent eventId, String& sourceId) {

}

void handleCommand(String& controlId, JsonObject& command) {
    
}