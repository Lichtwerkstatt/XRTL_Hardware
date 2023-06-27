#include "XRTLcommand.h"

XRTLsetableCommand::~XRTLsetableCommand()
{
    clear();
}

void XRTLsetableCommand::clear()
{
    if (key)
        delete key;
    if (val)
        delete val;
    key = NULL;
    val = NULL;
}

void XRTLsetableCommand::set(const String &controlId, const String &controlKey, bool controlVal)
{
    clear();
    id = controlId;
    key = new String(controlKey);
    val = new XRTLvalBool(controlVal);
}

void XRTLsetableCommand::set(const String &controlId, const String &controlKey, long controlVal)
{
    clear();
    id = controlId;
    key = new String(controlKey);
    val = new XRTLvalInt(controlVal);
}

void XRTLsetableCommand::set(const String &controlId, const String &controlKey, double controlVal)
{
    clear();
    id = controlId;
    key = new String(controlKey);
    val = new XRTLvalFloat(controlVal);
}

void XRTLsetableCommand::set(const String &controlId, const String &controlKey, String controlVal)
{
    clear();
    id = controlId;
    key = new String(controlKey);
    val = new XRTLvalString(controlVal);
}

void XRTLsetableCommand::saveSettings(JsonObject& settings)
{
    settings["id"] = id;
    val->passValue(*key, settings);
}

void XRTLsetableCommand::fillCommand(JsonObject& command)
{
    command["controlId"] = id;
    val->passValue(*key, command);
}

void XRTLsetableCommand::setViaSerial()
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

XRTLdisposableCommand::XRTLdisposableCommand(String targetId)
{
    id = targetId;
}
XRTLdisposableCommand::~XRTLdisposableCommand()
{
    for (int i = 0; i < keyCount; i++)
    {
        delete keys[i];
        delete vals[i];
    }
}

void XRTLdisposableCommand::add(const String &key, bool val)
{
    keys[keyCount] = new String(key);
    vals[keyCount++] = new XRTLvalBool(val);
}

void XRTLdisposableCommand::add(const String &key, int val)
{
    keys[keyCount] = new String(key);
    vals[keyCount++] = new XRTLvalInt(val);
}

void XRTLdisposableCommand::add(const String &key, double val)
{
    keys[keyCount] = new String(key);
    vals[keyCount++] = new XRTLvalFloat(val);
}

void XRTLdisposableCommand::add(const String &key, String val)
{
    keys[keyCount] = new String(key);
    vals[keyCount++] = new XRTLvalString(val);
}

void XRTLdisposableCommand::fillCommand(JsonObject &command)
{
    command["controlId"] = id;
    for (int i = 0; i < keyCount; i++)
    {
        vals[i]->passValue(*keys[i], command);
    }
}