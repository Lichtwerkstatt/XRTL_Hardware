#ifndef XRTLCOMMAND_H
#define XRTLCOMMAND_H

#include "XRTLval.h"
#include "XRTLmodule.h"

class XRTLcommand {
    private:
    String id;
    String key;
    XRTLval val;

    public:
    XRTLcommand();

    void set(String& controlId, String& controlKey, JsonVariant& controlVal);
    
    String& getId();

    void fillCommand(JsonObject& command);

    void saveSettings(JsonObject& settings);

    void setViaSerial();
};

#endif