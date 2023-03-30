#ifndef MACROMODULE_H
#define MACROMODULE_H

#include "MacroState.h"

class MacroModule : public XRTLmodule {
    private:
    uint8_t stateCount = 0;
    MacroState* states[16];

    String controlKey = "key";
    String currentState = "";
    String initState = "";

    void listStates();
    void dialog();

    public:
    MacroModule(String moduleName);
    moduleType type = xrtl_macro;
    ParameterPack stateParameters;

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