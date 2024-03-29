#ifndef MACROMODULE_H
#define MACROMODULE_H

#include "MacroState.h"

class MacroModule : public XRTLmodule {
private:
    int64_t nextAction = 0;
    uint8_t stateCount = 0;
    MacroState *states[16];
    MacroState *activeState = NULL;

    String controlKey = "state";
    String currentStateName = "";
    String initState = "";

    void listStates();
    bool dialog();
    bool stateDialog();

    String listeningId = "";
    String infoLED = "";

    MacroState *findState(String &state);
    void selectState(String &targetState);

    void addState(String &stateName);
    void delState(uint8_t number);

public:
    MacroModule(String moduleName);
    ~MacroModule();

    moduleType type = xrtl_macro;
    ParameterPack stateParameters;

    void setup();
    void loop();
    void stop();

    bool getStatus(JsonObject &status);
    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    void setViaSerial();

    void handleCommand(String &controlId, JsonObject &command);
    void handleInternal(internalEvent eventId, String &sourceId);
    void handleStatus(String &controlId, JsonObject &status);
};

#endif