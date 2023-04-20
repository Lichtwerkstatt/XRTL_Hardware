#include "MacroModule.h"

MacroModule::MacroModule(String moduleName)
{
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(initState, "initState", "String");
    parameters.add(controlKey, "controlKey", "String");
}

MacroModule::~MacroModule()
{
    for (int i = 0; i < stateCount; i++)
    {
        delete states[i];
    }
}

void MacroModule::setup()
{
    if (!initState || initState == "" || stateCount == 0)
        return;

    currentStateName = initState;
    activeState = NULL;
}

void MacroModule::loop()
{
    if (!activeState || esp_timer_get_time() < nextAction) // only continue if executing a state AND no timer condition is violated (interrupts reset timer) 
        return;

    nextAction = 0; // reset timer if condition met
    while (nextAction == 0 && activeState) // loop through commands until paused or completed (indicated by activeState == NULL)
    {
        activeState->loop(); // state will call stop() when completed
    }
}

void MacroModule::stop()
{
    if (!activeState) // no active state: nothing to stop
        return;
    
    listeningId = ""; // listeningId not persistent beyond state execution
    currentStateName = activeState->getName();
    nextAction = 0;
    activeState = NULL;
    sendStatus();
    notify(ready);
}

void MacroModule::loadSettings(JsonObject &settings)
{
    // parameters.load(settings);

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
    for (JsonPair kv : stateCollection)
    {
        if (!kv.value().isNull() && kv.value().is<JsonArray>())
        {
            String stateName = kv.key().c_str();
            addState(stateName);

            JsonArray stateSettings = kv.value().as<JsonArray>();
            states[stateCount - 1]->loadSettings(stateSettings);
            Serial.println("");
        }
    }

    if (debugging)
        parameters.print();
}

void MacroModule::saveSettings(JsonObject &settings)
{
    /*settings["currentState"] = currentState;
    settings["initState"] = initState;
    settings["controlKey"] = controlKey;*/
    JsonObject subSettings;
    parameters.save(settings, subSettings);

    if (stateCount == 0)
        return;

    JsonObject stateSettings = subSettings.createNestedObject("states");
    for (int i = 0; i < stateCount; i++)
    {
        JsonArray currentSettings = stateSettings.createNestedArray(states[i]->getName());
        states[i]->saveSettings(currentSettings);
    }
}

void MacroModule::listStates()
{
    for (int i = 0; i < stateCount; i++)
    {
        printf("%i: %s\n", i, states[i]->getName());
    }
}

bool MacroModule::dialog()
{
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

    if (choice == "r")
        return false;
    else if (choice == "c")
    {
        id = serialInput("controlId: ");
        controlKey = serialInput("control key: ");
        currentStateName = serialInput("current state: ");
        initState = serialInput("initial state: ");
    }
    else if (choice == "a")
    {
        String newState = serialInput("state name: ");
        if (newState == NULL || newState == "")
            return true;
        addState(newState);
    }
    else if (choice == "d")
    {
        Serial.println("available states: ");
        listStates();
        Serial.println("");

        choiceInt = serialInput("delete: ").toInt();
        if (choiceInt < stateCount)
        {
            delState(choiceInt);
        }
    }
    else if (choiceInt <= stateCount)
    {
        states[choiceInt]->setViaSerial();
    }
    return true;
}

void MacroModule::setViaSerial()
{
    while (dialog())
    {
    }
}

MacroState *MacroModule::findState(String &wantedState)
{
    for (int i = 0; i < stateCount; i++)
    {
        if (states[i]->getName() == wantedState)
            return states[i];
    }
    return NULL;
}

void MacroModule::addState(String &stateName)
{
    if (findState(stateName) != NULL)
    {
        debug("state <%s> already in use by this module", stateName.c_str());
        return;
    }

    states[stateCount++] = new MacroState(stateName, this);
    debug("state added: %s", stateName);
}

void MacroModule::delState(uint8_t number)
{
    delete states[number];
    for (int i = number; i < stateCount; i++)
    {
        states[i] = states[i + 1];
    }
    states[stateCount--] = NULL;
}

bool MacroModule::getStatus(JsonObject &status)
{
    status["busy"] = activeState ? true : false;
    status[controlKey] = currentStateName;
    return true;
}

void MacroModule::selectState(String &targetState)
{
    /* if (activeState) // already active
    {
        if (activeState->isCompleted()) // auto switching to new state is permitted only at the very end
        {
            stop();
        }
        else
        {
            //TODO: send error? will make switching impossible in certain cases
            String errmsg = "currently activating <";
            errmsg += activeState->getName();
            errmsg += ">";
            sendError(is_busy, errmsg);
            return;
        }
    } */

    activeState = findState(targetState);
    if (!activeState)
    {
        String errmsg = "target state <";
        errmsg += targetState;
        errmsg += "> could not be found";
        sendError(out_of_bounds, errmsg);   
        return;
    }

    nextAction = 0;
    activeState->activate();
    sendStatus();
    notify(busy);
}

void MacroModule::handleCommand(String &controlId, JsonObject &command)
{
    if (!isModule(controlId))
        return;

    bool tmpBool = false;
    if (getValue<bool>("getStatus", command, tmpBool) && tmpBool)
    {
        sendStatus();
    }

    if (getValue<bool>("stop", command, tmpBool) && tmpBool)
    {
        stop();
    }

    if (getValue("hold", command, tmpBool))
    {
        nextAction = tmpBool ? 9223372036854775807 : 0; // maximum int64_t value
        debug("hold %s", tmpBool ? "on" : "off");
    }

    uint32_t duration;
    if (getValue("wait", command, duration))
    {
        nextAction = esp_timer_get_time() + 1000 * duration;
        debug("waiting for %d ms", duration);
    }

    if (getValue("complete", command, duration))
    {
        nextAction = duration == 0 ? 9223372036854775807 : 1000 * duration + esp_timer_get_time();
        listeningId = activeState ? activeState->relCommand(-2)->getId() : ""; // get controlId of the last command (currentCommand was incremented twice)
        debug("waiting for completion of <%s>", listeningId);
    }

    String waitingId;
    if (activeState && getValue("listen", command, waitingId))
    {
        listeningId = waitingId;
        debug("listening for <%s>", listeningId);
    }

    String targetState;
    if (getValue<String>(controlKey, command, targetState))
    {
        // custom control key recognized
        selectState(targetState);
    }

}

void MacroModule::handleInternal(internalEvent eventId, String &sourceId)
{
    if (!activeState || listeningId == "" || eventId != ready || sourceId != listeningId)
        return;
    else
    {
        nextAction = 0;
    } 
}

void MacroModule::handleStatus(String &controlId, JsonObject &status)
{
    if (!activeState || listeningId == "" || controlId != listeningId)
        return;
    bool isBusy;
    if (getValue("busy", status, isBusy) && isBusy)
        return;
    nextAction = 0;
}