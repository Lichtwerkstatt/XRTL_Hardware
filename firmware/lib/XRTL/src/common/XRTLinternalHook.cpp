#include "XRTLinternalHook.h"

InternalHook::InternalHook()
{
    parameters.add(eventType, "eventId", "0-11");
    parameters.add(listeningId, "listeningId", "String");
}

bool InternalHook::isTriggered(internalEvent &eventId, String &sourceId)
{
    if (eventType != eventId)
        return false;
    if (sourceId != listeningId && sourceId != "*")
        return false;
    return true;
}

internalEvent &InternalHook::getType() {
    return eventType;
}

String &InternalHook::getId()
{
    return listeningId;
}

XRTLcommand &InternalHook::getCommand()
{
    return command;
}

void InternalHook::setViaSerial()
{
    parameters.setViaSerial();
    command.setViaSerial();
}

void InternalHook::set(internalEvent &eventId, String &sourceId)
{
    eventType = eventId;
    listeningId = sourceId;
}

void InternalHook::save(JsonObject &settings)
{
    parameters.save(settings);
    JsonObject commandSettings = settings.createNestedObject(command.getId());
    command.saveSettings(commandSettings);
}

void InternalHook::load(JsonObject &settings, bool &debugging)
{
    parameters.load(settings);
    settings.remove("eventId");
    settings.remove("listeningId");

    for (JsonPair kv : settings)
    {
        if (kv.value().isNull() || !kv.value().is<JsonObject>())
            return;
            
        JsonObject commandSettings = kv.value().as<JsonObject>();

        String commandId = kv.key().c_str();
        String commandKey;
        JsonVariant commandVal;

        for (JsonPair commandKv : commandSettings)
        {
            commandKey = commandKv.key().c_str();
            commandVal = commandKv.value();
        }
        
        if (commandVal.is<bool>())
            command.set(commandId, commandKey, commandVal.as<bool>());
        else if (commandVal.is<int>())
            command.set(commandId, commandKey, commandVal.as<long>());
        else if (commandVal.is<float>())
            command.set(commandId, commandKey, commandVal.as<double>());
        else if (commandVal.is<String>())
            command.set(commandId, commandKey, commandVal.as<String>());

        if (debugging)
        {
            Serial.printf("%s(%s) -> %s:{%s:%s}\n",
                listeningId.c_str(),
                internalEventNames[eventType],
                commandId.c_str(), commandKey.c_str(),
                commandVal.as<String>().c_str()
            );
        }
    }
}