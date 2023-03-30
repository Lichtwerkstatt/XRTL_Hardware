#include "MacroModule.h"

MacroModule::MacroModule(String moduleName) {
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(initState, "initState", "String");
    parameters.add(controlKey, "controlKey", "String");
}

void MacroModule::setup() {
    if (initState == "" || initState == NULL  || stateCount == 0) return;
    currentState = initState;
}

void MacroModule::loadSettings(JsonObject& settings){
    //parameters.load(settings);

    /*currentState = loadValue<String>("currentState", settings, "");
    initState = loadValue<String>("initState", settings, "");
    controlKey = loadValue<String>("controlKey", settings, "key");

    Serial.printf("current state: %s\n", currentState);
    Serial.printf("initial state: %s\n", initState);
    Serial.printf("control key: %s\n", controlKey);
    Serial.println("");*/
    JsonObject subSettings;
    parameters.load(settings, subSettings);

    JsonObject stateCollection = subSettings["states"];
    for (JsonPair kv : stateCollection) {
        if ( (!kv.value().isNull()) && (kv.value().is<JsonObject>()) ) {
            JsonObject moduleSettings = kv.value().as<JsonObject>();
            String stateName = kv.key().c_str();
            addState(stateName);

            JsonObject stateSettings = kv.value().as<JsonObject>();
            states[stateCount - 1]->loadSettings(stateSettings);
            Serial.println("");
        }
    }

    if (debugging) parameters.print();
}

void MacroModule::saveSettings(JsonObject& settings){
    /*settings["currentState"] = currentState;
    settings["initState"] = initState;
    settings["controlKey"] = controlKey;*/
    JsonObject subSettings;
    parameters.save(settings, subSettings);

    if (stateCount == 0) return;
    JsonObject stateSettings = subSettings.createNestedObject("states");
    for (int i = 0; i < stateCount; i++) {
        JsonObject currentSettings = stateSettings.createNestedObject(states[i]->getName());
        states[i]->saveSettings(currentSettings);
    }
}

void MacroModule::listStates() {
    for (int i = 0; i < stateCount; i++) {
        printf("%i: %s\n", i, states[i]->getName());
    }
}

bool MacroModule::dialog(){
    parameters.setViaSerial();
    /*Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");*/

    Serial.println("available settings:");
    listStates();
    Serial.println("");
    Serial.println("a: add state");
    Serial.println("d: delete state");
    Serial.println("c: change settings");
    Serial.println("r: return");
    
    Serial.println("");
    String choice = serialInput("choice: ");
    uint8_t choiceInt = choice.toInt();
    Serial.println("");

    if (choice == "r") return false;
    else if (choice == "c") {
        id = serialInput("controlId: ");
        controlKey = serialInput("control key: ");
        currentState = serialInput("current state: ");
        initState = serialInput("initial state: ");
    }
    else if (choice == "a") {
        String newState = serialInput("state name: ");
        if (newState == NULL || newState == "") return true;
        addState(newState);
    }
    else if (choice == "d") {
        Serial.println("available states: ");
        listStates();
        Serial.println("");

        choiceInt = serialInput("delete: ").toInt();
        if (choiceInt < stateCount){
            delState(choiceInt);
        }
    }
    else if (choiceInt <= stateCount) {
        states[choiceInt]->setViaSerial();
    }
    return true;
}

void MacroModule::setViaSerial(){
    while (dialog()){}
}

MacroState* MacroModule::findState(String& wantedState) {
    for (int i = 0; i < stateCount; i++) {
        if (states[i]->getName() == wantedState) return states[i];
    }
    return NULL;
}

void MacroModule::addState(String& stateName){
    if (findState(stateName) != NULL) {
        debug("state <%s> already in use by this module", stateName.c_str());
        return;
    }

    states[stateCount++] = new MacroState(stateName, this);
    debug("state added: %s", stateName);
}

void MacroModule::delState(uint8_t number) {
    delete states[number];
    for (int i = number; i < stateCount; i ++) {
        states[i] = states[i+1];
    }
    states[stateCount--] = NULL;
}

bool MacroModule::getStatus(JsonObject& status) {
    status[controlKey] = currentState;
    return true;
}

void MacroModule::selectState(String& targetState) {
    MacroState* targetStatePointer = findState(targetState);
    if (targetStatePointer == NULL) {
        String errmsg = "target state <";
        errmsg += targetState;
        errmsg += "> could not be found";
        sendError(out_of_bounds, errmsg);
        return;
    }

    targetStatePointer->activate();
    currentState = targetStatePointer->getName();
    sendStatus();
}

void MacroModule::handleCommand(String& controlId, JsonObject& command) {
    if (!isModule(controlId)) return;

    bool getStatus = false ;
    if (getValue<bool>("getStatus", command, getStatus) && getStatus) {
        sendStatus();
    }

    String targetState;
    if (getValue<String>(controlKey, command, targetState)) {
        selectState(targetState);
    }
}