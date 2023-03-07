#include "MacroState.h"

MacroState::MacroState(String& name, XRTLmodule* parent) {
    stateName = name;
    macro = parent;
}

void MacroState::listCommands() {
    for (int i = 0; i < commandCount; i++) {
        printf("%i: %s\n", i, commands[i]->getId());
    }
}

void MacroState::addCommand() {
    if (commandCount > 7) {
        Serial.println("maximum number of commands reached");
        return;
    }
    commands[commandCount++] = new XRTLcommand();
}

void MacroState::addCommand(String& id, String& key, JsonVariant& val) {
    addCommand();
    commands[commandCount - 1]->set(id, key, val);
}

void MacroState::delCommand(uint8_t number) {
    delete commands[number];
    for (int i = number; i < commandCount - 1; i++) {
        commands[i] = commands[i+1];
    }
    commands[commandCount--] = NULL;
}

void MacroState::swpCommand(uint8_t firstNumber, uint8_t secondNumber){
    XRTLcommand* temp = commands[firstNumber];
    commands[firstNumber] = commands[secondNumber];
    commands[secondNumber] = temp;
}

void MacroState::dialog(){
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

    if (choice == "r") return;
    else if (choice == "a") {
        addCommand();
        commands[commandCount - 1]->setViaSerial();
        return;
    }
    else if (choice == "d") {
        Serial.println("");
        Serial.println("commands:");
        listCommands();
        choice = serialInput("delete: ");
        choiceInt = choice.toInt();
        
        if (choiceInt < commandCount) {
            delCommand(choiceInt);    
        }

        return;
    }
    else if (choice == "s") {
        uint8_t firstNumber = serialInput("first command: ").toInt();
        uint8_t secondNumber = serialInput("second command: ").toInt();
        swpCommand(firstNumber, secondNumber);
        return;
    }
    else if (choice == "c") {
        stateName = serialInput("state name: ");
        return;
    }
    else if (choiceInt < commandCount) {
        commands[choiceInt]->setViaSerial();
    }
}

void MacroState::activate(){
    macro->debug("activated <%s>", stateName);
    for (int i = 0; i < commandCount; i++) {
        String controlId = commands[i]->getId();
        XRTLmodule* targetModule = macro->findModule(controlId);
        DynamicJsonDocument doc(512);

        if (targetModule != NULL) { // module located on this hardware
            JsonObject command = doc.to<JsonObject>();

            commands[i]->fillCommand(command);
            targetModule->handleCommand(controlId, command);
        }
        else {
            JsonArray event = doc.to<JsonArray>();
            event.add("command");
            JsonObject command = event.createNestedObject();
            
            commands[i]->fillCommand(command);
            macro->sendEvent(event); 
        }

    }
}

String MacroState::getName() {
    return stateName;
}

void MacroState::saveSettings(JsonObject& settings) {
    for (int i = 0; i < commandCount; i++) {
        JsonObject commandSettings = settings.createNestedObject(commands[i]->getId());
        commands[i]->saveSettings(commandSettings);
    }
}

void MacroState::loadSettings(JsonObject& settings) {
    for (JsonPair kv : settings) {
        if ( (!kv.value().isNull()) && (kv.value().is<JsonObject>()) ) {
            JsonObject commandSettings = kv.value().as<JsonObject>();

            String commandId = kv.key().c_str();
            String commandKey;
            JsonVariant commandVal;

            for (JsonPair commandKv : commandSettings) {
                commandKey = commandKv.key().c_str();
                commandVal = commandKv.value();
            }
            Serial.printf("%s:{%s:%s}\n",commandId, commandKey, commandVal.as<String>());
            addCommand(commandId, commandKey, commandVal);
        }
    }
}