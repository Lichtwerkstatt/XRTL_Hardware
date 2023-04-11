#ifndef XRTLCOMMAND_H
#define XRTLCOMMAND_H

#include "XRTLval.h"

// @brief stores all information necessary to issue a baisc valid command
class XRTLcommand
{
protected:
    String id;
    String key;
    XRTLval val;

public:
    XRTLcommand();
    virtual void set(String &controlId, String &controlKey, JsonVariant &controlVal);

    String &getId();
    // @brief fill a JsonObject with the command
    // @note the JsonDocument must be called externally or it would not the survive the context of the function call
    virtual void fillCommand(JsonObject &command);
    // @brief fill a JsonObject with the command but substitute the stored value with the supplied value
    // @note the JsonDocument must be called externally or it would not the survive the context of the function call
    void fillCommand(JsonObject &command, JsonVariant &relayVal);

    void saveSettings(JsonObject &settings);
    void setViaSerial();
};

class XRTLdisposableCommand : public XRTLcommand
{
private:
    const String* keys[8];
    XRTLval* vals[8];
    uint8_t keyCount = 0;

public:
    XRTLdisposableCommand(String targetId)
    {
        id = targetId;
    }
    ~XRTLdisposableCommand()
    {
        for (int i = 0; i < keyCount; i++)
        {
            delete keys[i];
            delete vals[i];
        }
    }

    void add(const String &key, bool val)
    {
        keys[keyCount] = new String(key);
        vals[keyCount++] = new XRTLvalBool(val);
    }

    void add(const String &key, int val)
    {
        keys[keyCount] = new String(key);
        vals[keyCount++] = new XRTLvalInt(val);
    }

    void add(const String &key, double val)
    {
        keys[keyCount] = new String(key);
        vals[keyCount++] = new XRTLvalFloat(val);
    }

    void add(const String &key, String val)
    {
        keys[keyCount] = new String(key);
        vals[keyCount++] = new XRTLvalString(val);
    }

    void fillCommand(JsonObject &command)
    {
        command["controlId"] = id;
        for (int i = 0; i < keyCount; i++)
        {
            vals[i]->passValue(*keys[i], command);
        }
    }
};

#endif