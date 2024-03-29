#include "MacroModule.h"

MacroModule::MacroModule(String moduleName)
{
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");

    parameters.add(initState, "initState", "String");
    parameters.add(controlKey, "controlKey", "String");

    parameters.add(infoLED, "infoLED", "String");
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
    // only continue if executing a state AND no timer is due
    if (!activeState || esp_timer_get_time() < nextAction) return;

    nextAction = 0; // reset timer
    while (nextAction == 0 && activeState) // loop through commands until paused or completed (indicated by activeState == NULL)
    {
        activeState->loop(); // state will call stop() when completed
    }
}

void MacroModule::stop()
{
    if (!activeState) return; // no active state: nothing to stop

    if (activeState->isCompleted()) {
        currentStateName = activeState->getName();
    } else {
        currentStateName = "aborted";
    }
    
    
    listeningId = ""; // so that listeningId does not persist beyond state execution
    nextAction = 0;
    activeState = NULL;
    sendStatus();
    notify(ready);
}

void MacroModule::loadSettings(JsonObject &settings)
{
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

    if (debugging && *debugging) parameters.print();
}

void MacroModule::saveSettings(JsonObject &settings)
{
    // initState = currentStateName; // set current state to init state

    JsonObject subSettings;
    parameters.save(settings, subSettings);

    if (stateCount == 0) return;

    JsonObject stateSettings = subSettings.createNestedObject("states");
    for (int i = 0; i < stateCount; i++)
    {
        JsonArray currentSettings = stateSettings.createNestedArray(states[i]->getName());
        states[i]->saveSettings(currentSettings);
    }
}

/**
 * @brief prints all registered states via the serial interface
*/
void MacroModule::listStates()
{
    for (int i = 0; i < stateCount; i++)
    {
        printf("%i: %s\n", i, states[i]->getName());
    }
}

/**
 * @brief handles serial setup of the states
 * @returns true if the dialog should be called again
*/
bool MacroModule::stateDialog()
{
    Serial.println("");
    Serial.println(centerString("",55,'-'));
    Serial.println(centerString(id.c_str(), 55, ' '));
    Serial.println(centerString("",55,'-'));
    Serial.println("");

    Serial.println("current states:");
    listStates();
    Serial.println("");
    Serial.println("a: add state");
    Serial.println("d: delete state");
    Serial.println("r: return");

    Serial.println("");
    String choice = serialInput("send single letter or number to edit state: ");
    uint8_t choiceInt = choice.toInt();
    Serial.println("");

    if (choice == "r") return false;

    else if (choice == "a")
    {
        String newState = serialInput("new state name: ");
        if (newState == NULL || newState == "")
            return true;
        addState(newState);
    }
    else if (choice == "d")
    {
        Serial.println("available states: ");
        listStates();
        Serial.println("");

        choiceInt = serialInput("send state number to delete: ").toInt();
        if (choiceInt < stateCount)
        {
            delState(choiceInt);
        }
    }
    else if (choiceInt < stateCount)
    {
        states[choiceInt]->setViaSerial();
    }
    return true;
}

/**
 * @brief handles the setup dialog on the serial interface
 * @returns true if the dialog should be called again
*/
bool MacroModule::dialog()
{
    highlightString(id.c_str(), '-');

    Serial.println("available settings:");
    Serial.println("");
    Serial.println("b: basic settings");
    Serial.println("m: manage states");
    Serial.println("r: return");
    Serial.println("");

    String choice = serialInput("send single letter or number to edit settings: ");
    if (choice == "r")
        return false;
    else if (choice == "b")
        parameters.setViaSerial();
    else if (choice == "m")
    {
        while (stateDialog())
        {
        }
    }

    return true;
}

void MacroModule::setViaSerial()
{
    while (dialog())
    {
    }
}

/**
 * @brief search for a state by its name
 * @param wantedState String of the module searched for
 * @returns pointer to the state or NULL if the state could not be found
*/
MacroState *MacroModule::findState(String &wantedState)
{
    for (int i = 0; i < stateCount; i++)
    {
        if (states[i]->getName() == wantedState)
            return states[i];
    }
    return NULL;
}

/**
 * @brief add a new state
 * @param stateName ID of the new state
*/
void MacroModule::addState(String &stateName)
{
    if (findState(stateName) != NULL)
    {
        debug("state <%s> already in use by this module", stateName.c_str());
        return;
    } else if (stateName == "aborted") {
        debug("state name <%s> reserved for internal use", stateName.c_str());
        return;
    }

    states[stateCount++] = new MacroState(stateName, this);
    debug("state added: %s", stateName);
}

/**
 * @brief delete a state permanently
 * @param number index of the state to remove
*/
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
    MacroState *candidateState = findState(targetState);
    if (!candidateState)
    {
        String errmsg = "target state <";
        errmsg += targetState;
        errmsg += "> could not be found";
        sendError(out_of_bounds, errmsg);   
        return;
    }


    nextAction = 0;
    activeState = candidateState;
    activeState->activate();
    sendStatus();
    notify(busy);
}

void MacroModule::handleCommand(String &controlId, JsonObject &command)
{
    if (!isModule(controlId) && controlId != "*")
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

    // if (getValue<bool>("manualSave", command, tmpBool) && tmpBool) {
    //     manualSave;
    // }

    String color;
    if (infoLED != "" && getValue("color", command, color))
    {
        XRTLdisposableCommand ledCommand(infoLED);
        ledCommand.add("color", color);
        sendCommand(ledCommand);
    }

    int64_t duration;
    if (getAndConstrainValue("wait", command, duration, (int64_t) -1, (int64_t) 9223372036854775))
    {
        switch (duration)
        {
            case -1: // stop waiting
            {
                nextAction = 0;
                debug("stopped waiting");
                break;
            }
            case 0: // wait indefinetly
            {
                nextAction = 9223372036854775807;
                debug("waiting indefinetly");
                break;
            }
            default: // time period is valid, interprete as ms
            {
                nextAction = esp_timer_get_time() + 1000 * duration;
                debug("waiting for %d ms", duration);
                break;
            }
        }
    }

    if (getAndConstrainValue("complete", command, duration, (int64_t) 0, (int64_t) 9223372036854775))
    {
        nextAction = duration == 0 ? 9223372036854775807 : 1000 * duration + esp_timer_get_time();
        if (activeState)
        {
            XRTLcommand *lastCommand = activeState->relCommand(-2); // get the last command; incremented twice: after execution of last command and call to `complete`
            listeningId = lastCommand ? lastCommand->getId() : ""; // make sure relCommand() returned a valid command
        }
        debug("waiting for completion of <%s>", listeningId.c_str());
    }

    String waitingId;
    if (getValue("listen", command, waitingId))
    {
        listeningId = waitingId;
        debug("listening for <%s>", listeningId.c_str());
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
    if (!activeState || listeningId == "" || eventId != ready || (sourceId != listeningId && listeningId != "*") )
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