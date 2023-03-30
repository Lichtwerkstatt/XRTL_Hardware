#ifndef MACROSTATE_H
#define MACROSTATE_H

#include "modules/XRTLmodule.h"
#include "common/XRTLcommand.h"

// @brief container for several commands that are called in succession once the macro is activated
// @note if a the controlId featured in the macro cannot be found on this ESP, an attempt to send the command via the socket server will be made
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

    bool dialog();

    public:
    MacroState(String& name, XRTLmodule* parent);

    void activate();
    
    String getName();

    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
};

#endif