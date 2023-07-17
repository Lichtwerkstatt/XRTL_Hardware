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

/**
 * @brief save the currently stored settings to a JsonObject
 * @param settings JsonObject the command will be stored in
*/
void XRTLsetableCommand::saveSettings(JsonObject& settings)
{
    settings["id"] = id;
    val->passValue(*key, settings);
}

/**
 * @brief fill a JsonObject with the stored command
 * @param command JsonObject that will receive the command info
*/
void XRTLsetableCommand::fillCommand(JsonObject& command)
{
    command["controlId"] = id;
    val->passValue(*key, command);
}

/**
 * @brief dialog to query all information to set the command via the serial interface
*/
void XRTLsetableCommand::setViaSerial()
{
    Serial.println("");
    String newId = serialInput("command controlId: ");
    String newKey = serialInput("command control key: ");

    Serial.println("");
    Serial.println("control value type needs to be determined");
    Serial.println("available types:");
    Serial.println("0: bool");
    Serial.println("1: int");
    Serial.println("2: float");
    Serial.println("3: string");
    Serial.println("");
    switch (serialInput("control value type: ").toInt())
    {
    case (0):
    {
        String input = serialInput("value (y/n): ");
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
        long input = serialInput("value (int): ").toInt();
        set(newId, newKey, input);
        break;
    }
    case (2):
    {
        double input = serialInput("value (float): ").toDouble();
        set(newId, newKey, input);
        break;
    }
    case (3):
    {
        String input = serialInput("value (string): ");
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

/**
 * @brief fill a JsonObject with all stored command info
 * @param command JsonObject that will be filled with the command
*/
void XRTLdisposableCommand::fillCommand(JsonObject &command)
{
    command["controlId"] = id;
    for (int i = 0; i < keyCount; i++)
    {
        vals[i]->passValue(*keys[i], command);
    }
}