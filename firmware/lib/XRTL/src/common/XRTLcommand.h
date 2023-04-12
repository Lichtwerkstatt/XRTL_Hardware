#ifndef XRTLCOMMAND_H
#define XRTLCOMMAND_H

#include "XRTLval.h"

// @brief stores all information necessary to issue a baisc valid command
class XRTLcommand
{
protected:
    String id;

public:
    virtual void set(const String &controlId, const String &controlKey, bool controlVal){}
    virtual void set(const String &controlId, const String &controlKey, long controlVal){}
    virtual void set(const String &controlId, const String &controlKey, double controlVal){}
    virtual void set(const String &controlId, const String &controlKey, String controlVal){}

    String &getId()
    {
        return id;
    }
    // @brief fill a JsonObject with the command
    // @note the JsonDocument must be called externally or it would not the survive the context of the function call
    virtual void fillCommand(JsonObject &command){}
    // @brief fill a JsonObject with the command but substitute the stored value with the supplied value
    // @note the JsonDocument must be called externally or it would not the survive the context of the function call
    //void fillCommand(JsonObject &command, JsonVariant &relayVal);

    virtual void saveSettings(JsonObject &settings) {}
    virtual void setViaSerial(){}
};

class XRTLcommand2 : public XRTLcommand
{
private:
    String *key = NULL;
    XRTLval *val = NULL;

    void clear()
    {
        if (key != NULL)
            delete key;
        if (val != NULL)
            delete val;
    }

public:
    ~XRTLcommand2()
    {
        clear();
    }

    void set(const String &controlId, const String &controlKey, bool controlVal)
    {
        clear();
        id = controlId;
        key = new String(controlKey);
        val = new XRTLvalBool(controlVal);
    }

    void set(const String &controlId, const String &controlKey, long controlVal)
    {
        clear();
        id = controlId;
        key = new String(controlKey);
        val = new XRTLvalInt(controlVal);
    }

    void set(const String &controlId, const String &controlKey, double controlVal)
    {
        clear();
        id = controlId;
        key = new String(controlKey);
        val = new XRTLvalFloat(controlVal);
    }

    void set(const String &controlId, const String &controlKey, String controlVal)
    {
        clear();
        id = controlId;
        key = new String(controlKey);
        val = new XRTLvalString(controlVal);
    }

    void saveSettings(JsonObject& settings)
    {
        val->passValue(*key, settings);
    }

    void fillCommand(JsonObject& command)
    {
        command["controlId"] = id;
        saveSettings(command);
    }
    
    void setViaSerial()
    {
        String newId = serialInput("controlId: ");
        String newKey = serialInput("control key: ");

        Serial.println("available types");
        Serial.println("0: bool");
        Serial.println("1: int");
        Serial.println("2: float");
        Serial.println("3: string");
        Serial.println("");
        switch (serialInput("control value type: ").toInt())
        {
        case (0):
        {
            String input = serialInput("value: ");
            if (input == "true" || input == "TRUE" || input == "y" || input == "1")
            {
                set(newId, newKey, true);
            }
            else if (input == "false" || input == "FALSE" || input == "n" || input == "0")
            {
                set(newId, newKey, false);
            }
            break;
        }
        case (1):
        {
            long input = serialInput("value: ").toInt();
            set(newId, newKey, input);
            break;
        }
        case (2):
        {
            double input = serialInput("value: ").toDouble();
            set(newId, newKey, input);
            break;
        }
        case (3):
        {
            String input = serialInput("value: ");
            set(newId, newKey, input);
            break;
        }
        }
    }
};

class XRTLdisposableCommand : public XRTLcommand
{
private:
    String* keys[8];
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