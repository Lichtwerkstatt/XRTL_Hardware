#ifndef XRTLCOMMAND_H
#define XRTLCOMMAND_H

#include "XRTLval.h"
/**
 * @brief stores all information necessary to issue a baisc valid command
 * @note this is a base class only used to provide a common interface, use one of the specialized classes XRTLsetableCommand or XRTLdisposableCommand instead
 */
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

class XRTLsetableCommand : public XRTLcommand
{
private:
    String *key = NULL;
    XRTLval *val = NULL;

    void clear();

public:
    ~XRTLsetableCommand();

    void set(const String &controlId, const String &controlKey, bool controlVal);
    void set(const String &controlId, const String &controlKey, long controlVal);
    void set(const String &controlId, const String &controlKey, double controlVal);
    void set(const String &controlId, const String &controlKey, String controlVal);

    void fillCommand(JsonObject& command);
    
    void saveSettings(JsonObject& settings);
    void setViaSerial();
};

class XRTLdisposableCommand : public XRTLcommand
{
private:
    String* keys[8];
    XRTLval* vals[8];
    uint8_t keyCount = 0;

public:
    XRTLdisposableCommand(String targetId);
    ~XRTLdisposableCommand();

    void add(const String &key, bool val);
    void add(const String &key, int val);
    void add(const String &key, double val);
    void add(const String &key, String val);

    void fillCommand(JsonObject &command);
};

#endif