#ifndef MACROMODULE_H
#define MACROMODULE_H

#include "MacroState.h"

class MacroModule : public XRTLmodule
{
private:
    int64_t nextAction = 0;
    uint8_t stateCount = 0;
    MacroState *states[16];
    MacroState *activeState = NULL;

    String controlKey = "key";
    String currentStateName = "";
    String initState = "";

    void listStates();
    bool dialog();

public:
    MacroModule(String moduleName);
    ~MacroModule();
    moduleType type = xrtl_macro;
    ParameterPack stateParameters;

    void setup();
    void loop();
    void stop();

    void addState(String &stateName);
    void delState(uint8_t number);

    bool getStatus(JsonObject &status);
    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    void setViaSerial();

    MacroState *findState(String &state);
    void selectState(String &targetState);

    void handleCommand(String &controlId, JsonObject &command);
};

#endif