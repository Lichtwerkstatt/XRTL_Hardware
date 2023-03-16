#ifndef XRTLCOMMAND_H
#define XRTLCOMMAND_H

#include "XRTLval.h"

// @brief stores all information necessary to issue a baisc valid command
class XRTLcommand {
    private:
    String id;
    String key;
    XRTLval val;

    public:
    XRTLcommand();
    void set(String& controlId, String& controlKey, JsonVariant& controlVal);
    
    String& getId();
    // @brief fill a JsonObject with the command
    // @note the JsonDocument must be called externally or it would not the survive the context of the function call
    void fillCommand(JsonObject& command);

    void saveSettings(JsonObject& settings);
    void setViaSerial();
};

#endif