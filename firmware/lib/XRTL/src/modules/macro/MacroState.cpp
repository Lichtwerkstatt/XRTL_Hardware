#include "MacroState.h"

MacroState::MacroState(String &name, XRTLmodule *parent)
{
    stateName = name;
    macro = parent;
}

MacroState::~MacroState()
{
    for (int i = 0; i < commandCount; i++)
    {
        delete commands[i];
    }
}

void MacroState::listCommands()
{
    for (int i = 0; i < commandCount; i++)
    {
        printf("%i: %s\n", i, commands[i]->getId().c_str());
    }
}

void MacroState::addCommand()
{
    if (commandCount > 7)
    {
        Serial.println("maximum number of commands reached");
        return;
    }
    commands[commandCount++] = new XRTLcommand2();
}

void MacroState::addCommand(String &id, String &key, JsonVariant &val)
{
    addCommand();

    if (val.is<bool>())
        commands[commandCount - 1]->set(id, key, val.as<bool>());
    else if (val.is<int>())
        commands[commandCount - 1]->set(id, key, val.as<long>());
    else if (val.is<float>())
        commands[commandCount - 1]->set(id, key, val.as<double>());
    if (val.is<String>())
        commands[commandCount - 1]->set(id, key, val.as<String>());
}

void MacroState::delCommand(uint8_t number)
{
    if (commands[number] == NULL)
        delete commands[number];
    for (int i = number; i < commandCount - 1; i++)
    {
        commands[i] = commands[i + 1];
    }
    commands[commandCount--] = NULL;
}

void MacroState::swpCommand(uint8_t firstNumber, uint8_t secondNumber)
{
    XRTLcommand *temp = commands[firstNumber];
    commands[firstNumber] = commands[secondNumber];
    commands[secondNumber] = temp;
}

bool MacroState::dialog()
{
    Serial.println(centerString(stateName, 39, ' '));
    Serial.println("");
    listCommands();
    Serial.println("");

    Serial.println("a: add command");
    Serial.println("d: delete command");
    Serial.println("s: swap commands");
    Serial.println("c: change state name");
    Serial.printf("r: return");

    Serial.println("");
    String choice = serialInput("choice: ");
    uint8_t choiceInt = choice.toInt();

    if (choice == "r")
        return false;
    else if (choice == "a")
    {
        addCommand();
        commands[commandCount - 1]->setViaSerial();
    }
    else if (choice == "d")
    {
        Serial.println("");
        Serial.println("commands:");
        listCommands();
        choice = serialInput("delete: ");
        choiceInt = choice.toInt();

        if (choiceInt < commandCount)
        {
            delCommand(choiceInt);
        }
    }
    else if (choice == "s")
    {
        uint8_t firstNumber = serialInput("first command: ").toInt();
        uint8_t secondNumber = serialInput("second command: ").toInt();
        swpCommand(firstNumber, secondNumber);
    }
    else if (choice == "c")
    {
        stateName = serialInput("state name: ");
    }
    else if (choiceInt < commandCount)
    {
        commands[choiceInt]->setViaSerial();
    }
    return true;
}

void MacroState::activate()
{
    macro->debug("activated <%s>", stateName);
    currentCommand = 0;
}

bool MacroState::isCompleted()
{
    return (currentCommand >= commandCount);
}

void MacroState::loop()
{
    if (isCompleted())
    {
        macro->stop();
        return;
    }

    macro->sendCommand(*commands[currentCommand++]);
}

String MacroState::getName()
{
    return stateName;
}

XRTLcommand *MacroState::relCommand(int8_t distance)
{
    int8_t query = commandCount + distance;
    if (query < 0 || query >= commandCount)
        return NULL;
    
    return commands[query];
}

void MacroState::saveSettings(JsonArray &settings)
{
    for (int i = 0; i < commandCount; i++)
    {
        //JsonObject commandSettings = settings.createNestedObject(commands[i]->getId());
        JsonObject commandSettings = settings.createNestedObject();
        commands[i]->saveSettings(commandSettings);
    }
}

void MacroState::loadSettings(JsonArray &settings)
{
    for (JsonObject commandSettings : settings)
    {
        String commandId = loadValue("id", commandSettings, "");
        if (commandId == "")
            continue;
        
        commandSettings.remove("id");
        String commandKey;
        JsonVariant commandVal;
        for (JsonPair commandKV : commandSettings)
        {
            commandKey = commandKV.key().c_str();
            commandVal = commandKV.value();
        }
        Serial.printf("%s:{%s:%s}\n", commandId.c_str(), commandKey, commandVal.as<String>().c_str());
        addCommand(commandId, commandKey, commandVal);
    }
}

void MacroState::setViaSerial()
{
    while (dialog())
    {
    }
}