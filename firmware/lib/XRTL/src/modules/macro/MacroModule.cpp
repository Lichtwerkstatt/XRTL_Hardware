#include "MacroModule.h"

MacroModule::MacroModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

moduleType MacroModule::getType() {
  return xrtl_macro;
}

void MacroModule::setup() {
    if (initState == "" || initState == NULL ) return;
    currentState = initState;
}

void MacroModule::loadSettings(JsonObject& settings){
    currentState = loadValue<String>("currentState", settings, "");
    initState = loadValue<String>("initState", settings, "");
    controlKey = loadValue<String>("controlKey", settings, "key");

    Serial.printf("current state: %s\n", currentState);
    Serial.printf("initial state: %s\n", initState);
    Serial.printf("control key: %s\n", controlKey);
    Serial.println("");

    settings.remove("currentVal");
    settings.remove("initState");

    for (JsonPair kv : settings) {
        if ( (!kv.value().isNull()) && (kv.value().is<JsonObject>()) ) {
            JsonObject moduleSettings = kv.value().as<JsonObject>();
            String stateName = kv.key().c_str();
            addState(stateName);

            JsonObject stateSettings = kv.value().as<JsonObject>();
            states[stateCount - 1]->loadSettings(stateSettings);
            Serial.println("");
        }
    }
}

void MacroModule::saveSettings(JsonObject& settings){
    settings["currentState"] = currentState;
    settings["initState"] = initState;
    settings["controlKey"] = controlKey;
    for (int i = 0; i < stateCount; i++) {
        JsonObject stateSettings = settings.createNestedObject(states[i]->getName());
        states[i]->saveSettings(stateSettings);
    }
}

void MacroModule::listStates() {
    for (int i = 0; i < stateCount; i++) {
        printf("%i: %s\n", i, states[i]->getName());
    }
}

void MacroModule::dialog(){
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

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

    if (choice == "r") return;
    else if (choice == "c") {
        id = serialInput("controlId: ");
        controlKey = serialInput("control key: ");
        currentState = serialInput("current state: ");
        initState = serialInput("initial state: ");
        return;
    }
    else if (choice == "a") {
        String newState = serialInput("state name: ");
        if (newState == NULL || newState == "") return;
        for (int i = 0; i < stateCount; i++) {
            if (newState == states[i]->getName()) {
                debug("state name already in use by this module");
                return;
            }
        }
        addState(newState);
        return;
    }
    else if (choice == "d") {
        Serial.println("available states: ");
        listStates();
        Serial.println("");

        choiceInt = serialInput("delete: ").toInt();
        if (choiceInt < stateCount){
            delState(choiceInt);
        }
        return;
    }
    else if (choiceInt <= stateCount) {
        states[choiceInt]->setViaSerial();
        return;
    }
}

void MacroModule::setViaSerial(){
    dialog();
    while ( serialInput("more changes to this module (y/n)?") == "y" ) dialog();
}

MacroState* MacroModule::findState(String& wantedState) {
    for (int i = 0; i < stateCount; i++) {
        if (states[i]->getName() == wantedState) return states[i];
    }
    return NULL;
}

void MacroModule::addState(String& stateName){
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