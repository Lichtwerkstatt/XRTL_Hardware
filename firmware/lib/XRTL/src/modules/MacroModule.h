#ifndef MACROMODULE_H
#define MACROMODULE_H

#include "XRTLmodule.h"

// TODO: turn this into a template class
class XRTLcommand {
    private:
    String id;
    String key;
    JsonVariant val;

    public:
    XRTLcommand() {
        id = "tmp";
        key = "key";
        val.set("val");
    }

    void set(String& controlId, String& controlKey, JsonVariant& controlVal) {
        id = controlId;
        key = controlKey;
        val = controlVal;
    }

    String getId() {
        return id;
    }

    JsonArray getEvent() {
        DynamicJsonDocument doc(512);
        JsonArray event = doc.to<JsonArray>();
        event.add("command");
        
        JsonObject command = event.createNestedObject();
        command["controlId"] = id;
        command[key] = val;

        return event;
    }

    JsonObject getCommand() {
        DynamicJsonDocument doc(512);
        JsonObject command = doc.to<JsonObject>();

        command["controlId"] = id;
        command[key] = val;

        return command;
    }

    void saveSettings(JsonObject& settings) {
        settings[key] = val;
    }

    void setViaSerial() {
        id = serialInput("controlId: ");
        key = serialInput("control key: ");
        Serial.println("available types");
        Serial.println("0: bool");
        Serial.println("1: int");
        Serial.println("2: float");
        Serial.println("3: string");
        Serial.println("");
        switch (serialInput("control value type: ").toInt()) {
            case (0): {
                String input = serialInput("value: ");
                val.set((input == "true" || input.toInt() == 1));
                break;
            }
            case (1): {
                val.set(serialInput("value: ").toInt());
                break;
            }
            case (2): {
                val.set(serialInput("value: ").toDouble());
                break;
            }
            case (3): {
                val.set(serialInput("value: "));
                break;
            }
        }
    }
};

class MacroState {
    private:
    XRTLmodule* macro;
    uint8_t commandCount = 0;
    XRTLcommand* commands[8];

    String stateName;

    void listCommands() {
        for (int i = 0; i < commandCount; i++) {
            printf("%i: %s\n", i, commands[i]->getId());
        }
    }

    void addCommand() {
        if (commandCount > 7) {
            Serial.println("maximum number of commands reached");
            return;
        }
        commands[commandCount++] = new XRTLcommand();
    }

    void addCommand(String& id, String& key, JsonVariant& val) {
        addCommand();
        commands[commandCount - 1]->set(id, key, val);
    }

    void delCommand(uint8_t number) {
        delete commands[number];
        for (int i = number; i < commandCount - 1; i++) {
            commands[i] = commands[i+1];
        }
        commands[commandCount--] = NULL;
    }

    void swpCommand(uint8_t firstNumber, uint8_t secondNumber){
        XRTLcommand* temp = commands[firstNumber];
        commands[firstNumber] = commands[secondNumber];
        commands[secondNumber] = temp;
    }

    void dialog(){
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

    public:
    MacroState(String& name, XRTLmodule* parent) {
        stateName = name;
        macro = parent;
    }

    void activate(){
        for (int i = 0; i < commandCount; i++) {
            JsonArray event = commands[i]->getEvent();
            macro->sendEvent(event); 
        }
    }
    
    String getName() {
        return stateName;
    }

    void saveSettings(JsonObject& settings) {
        for (int i = 0; i < commandCount; i++) {
            JsonObject commandSettings = settings.createNestedObject(commands[i]->getId());
            commands[i]->saveSettings(commandSettings);
        }
    }

    void loadSettings(JsonObject& settings) {
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

    void setViaSerial() {
        dialog();
        while ( serialInput("more changes to this state (y/n)?") == "y" ) dialog();
    }
};

class MacroModule : public XRTLmodule {
    private:
    uint8_t stateCount = 0;
    MacroState* states[16];

    String controlKey;
    String currentState;
    String initState;

    void listStates();
    void dialog();

    public:
    MacroModule(String moduleName, XRTL* source);
    moduleType getType();

    void setup();

    void addState(String& stateName);
    void delState(uint8_t number);

    bool getStatus(JsonObject& status);
    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();

    MacroState* findState(String& state);
    void selectState(String& targetState);

    void handleCommand(String& controlId, JsonObject& command);
};

#endif