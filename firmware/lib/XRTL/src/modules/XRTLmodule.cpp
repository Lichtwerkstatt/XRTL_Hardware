#include "XRTLmodule.h"

String XRTLmodule::getID()
{
    return id;
}

void XRTLmodule::setParent(XRTL *parent)
{
    xrtl = parent;
}

/**
 * 
 * @brief confirm the identity of a module by its controlId
 * @param moduleName controlId that is searched for
 * @returns true if controlId and moduleName match 
 */
bool XRTLmodule::isModule(String &moduleName)
{
    return (id == moduleName);
}

/**
 *
 * @brief initialization of the module, takes place only once 
 */
void XRTLmodule::setup()
{
    return;
}

/**
 * 
 * @brief gets called once during every loop of the main program 
 */
void XRTLmodule::loop()
{
    return;
}

/**
 * 
 * @brief define if a status should be send and what information it should contain
 * @param status JsonObject to be filled with status information by the module
 * @returns true if this module should send a status, defaults to false
 */
bool XRTLmodule::getStatus(JsonObject &status)
{
    return false;
}

/**
 * 
 * @brief save settings to flash when a planned shutdown occurs
 * @param settings JsonObject that must be filled with the data to be saved
 * @note format: {<key1>:<value1>,...,<keyN>:<valueN>}
 */
void XRTLmodule::saveSettings(JsonObject &settings)
{
    return;
}

/**
 * 
 * @brief load settings when device is started
 * @param settings JsonObject containing the data for this module loaded from flash
 * @note use this->loadValue() to safely initialize the settings
 */
void XRTLmodule::loadSettings(JsonObject &settings)
{
    return;
}

/**
 * @brief methode called when the settings of a module are to be set via the serial interface
 * @note all neccessary information must be polled from the user
 */
void XRTLmodule::setViaSerial()
{
    return;
}

void XRTLmodule::stop()
{
    return;
}

/**
 * 
 * @brief react to internal events issued by a module
 * @param eventId type of the occuring event
 * @param sourceId ID of the module triggering the event
 */
void XRTLmodule::handleInternal(internalEvent eventId, String &sourceId)
{
    switch (eventId)
    {
    case debug_off:
    {
        debugging = false;
        break;
    }
    case debug_on:
    {
        debugging = true;
        break;
    }
    }
}

/**
 * 
 * @brief processes and reacts to commands
 * @param controlId reference to the separated controlId
 * @param command reference to the whole command message including the command keys
 * @note to avoid controlId being separated by every individual module, it has to be done before distributing the command to the modules 
 */
void XRTLmodule::handleCommand(String &controlId, JsonObject &command)
{
    return;
}

/**
 * @brief processes and reacts to status EventBits_t
 * @param controlId reference to the separated controlId of the issuing module
 * @param status JsonObject holding the status information
 * @note to avoid controlId being separated by every individual module, it has to be done before distributing the command to the modules
 */
void XRTLmodule::handleStatus(String &controlId, JsonObject &status)
{
    return;
}