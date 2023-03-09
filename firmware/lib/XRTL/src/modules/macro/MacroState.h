#ifndef MACROSTATE_H
#define MACROSTATE_H

#include "XRTLmodule.h"
#include "common/XRTLcommand.h"

class MacroState {
    private:
    XRTLmodule* macro;
    uint8_t commandCount = 0;
    XRTLcommand* commands[8];

    String stateName;

    void listCommands();
    void addCommand();
    void addCommand(String& id, String& key, JsonVariant& val);
    void delCommand(uint8_t number);
    void swpCommand(uint8_t firstNumber, uint8_t secondNumber);

    void dialog();

    public:
    MacroState(String& name, XRTLmodule* parent);

    void activate();
    
    String getName();

    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
};

#endif