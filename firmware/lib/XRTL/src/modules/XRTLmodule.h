#ifndef XRTLMODULE_H
#define XRTLMODULE_H

#include "common/XRTLcommand.h"
#include "common/XRTLparameterPack.h"

// internal reference for module type
enum moduleType {
    xrtl_socket,
    xrtl_wifi,
    xrtl_infoLED,
    xrtl_stepper,
    xrtl_servo,
    xrtl_camera,
    xrtl_input,
    xrtl_output,
    xrtl_macro
};

// display names for modules
static const char *moduleNames[9] =
    {
        "socket",
        "wifi",
        "info LED",
        "stepper motor",
        "servo motor",
        "camera",
        "input",
        "output",
        "macro"};

// used to push state changes to other modules
enum internalEvent {
    socket_disconnected,
    socket_connected,
    socket_authed,

    wifi_disconnected,
    wifi_connected,

    busy,
    ready,

    time_synced,

    debug_on,
    debug_off,

    input_trigger_low,
    input_trigger_high
};

static const char *internalEventNames[12] =
    {
        "socket disconnected",
        "socket connected",
        "socket authed",
        "wifi disconnected",
        "wifi connected",
        "busy",
        "ready",
        "time synced",
        "debug on",
        "debug off",
        "low input trigger",
        "high input trigger",
};

// identify error category
enum componentError {
    deserialize_failed,
    field_is_null,
    wrong_type,
    out_of_bounds,
    hardware_failure,
    disconnected,
    is_busy
};

// forward declaration: need pointer
class XRTL;

// template class for all modules
class XRTLmodule {
protected:
    // @brief (supposedly) unique identifier the module listens to
    // @note if two modules share the same ID, both will react to commands issued under that ID
    String id;
    XRTL *xrtl;            // core address, must be assigned after construction using setParent()
    bool debugging = true; // true: print status messages via serial monitor and accept serial events

public:
    ParameterPack parameters; // stores parameters for the module
    String getID();           // return id
    String &getComponent();
    void setParent(XRTL *parent);
    bool isModule(String &moduleName);

    virtual void handleCommand(String &controlId, JsonObject &command);
    virtual void handleStatus(String &controlId, JsonObject &status);

    void sendEvent(JsonArray &event);
    void sendError(componentError ernr, String message);
    void sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length);
    void sendCommand(XRTLcommand &command);
    void sendStatus();

    void notify(internalEvent eventId);
    virtual void handleInternal(internalEvent eventId, String &sourceId);

    virtual void setup(); // called once during setup
    virtual void loop();  // called once in every loop
    virtual void stop();  // stop all operation, restart of device could be imminent

    virtual bool getStatus(JsonObject &status);

    virtual void saveSettings(JsonObject &settings);
    virtual void loadSettings(JsonObject &settings);
    virtual void setViaSerial();

    void setViaSerialBasic();

    /** @brief send a message over the serial monitor if in debug mode
     * @param ... uses printf syntax
     * @note automatically adds [controlId] at the start and \\n at the end of the message
     */
    template <typename... Args>
    void debug(Args... args) {
        if (!debugging)
            return;
        Serial.printf("[%s] ", id.c_str());
        Serial.printf(args...);
        Serial.print('\n');
    }

    /** @brief fetch a key value pair from a JsonObject
     * @param name search for this key
     * @param file object that will be searched for name
     * @param target store value here if key was found
     * @param reportMissingField if True a report will be issued in case the key could not be found
     * @returns True if the key was found
     */
    template <typename T>
    bool getValue(String name, JsonObject &file, T &target, bool reportMissingField = false) {
        auto field = file[name];
        if (field.isNull()) {
            if (!reportMissingField)
                return false;

            String errormsg = "[";
            errormsg += id;
            errormsg += "] command rejected: <";
            errormsg += name;
            errormsg += "> is missing";
            sendError(field_is_null, errormsg);
            return false;
        }

        if (!field.is<T>()) {
            String errormsg = "[";
            errormsg += id;
            errormsg += "] command rejected: <";
            errormsg += name;
            errormsg += "> is of wrong type";
            sendError(wrong_type, errormsg);
            return false;
        }

        target = field.as<T>();
        return true;
    }

    // @brief fetch a key value pair from a JsonObject and constrain the value to a specified interval
    // @param name search for this key
    // @param file object that will be searched for name
    // @param target store value here if key was found
    // @param minValue lower bound of the constraining interval
    // @param maxValue upper bound of the constraining interval
    // @param reportMissingField if True a report will be issued in case the key could not be found, use only for mandatory keys
    // @returns True if the key was found
    template <typename T>
    bool getAndConstrainValue(String name, JsonObject &file, T &target, T minValue, T maxValue, bool reportMissingField = false) {
        if (!getValue<T>(name, file, target, reportMissingField))
            return false;

        if ((target < minValue) or (target > maxValue)) {
            target = constrain(target, minValue, maxValue);

            String errormsg = "[";
            errormsg += id;
            errormsg += "] out of bounds: <";
            errormsg += name;
            errormsg += "> was constrained to (";
            errormsg += minValue;
            errormsg += ",";
            errormsg += maxValue;
            errormsg += ")";
            sendError(out_of_bounds, errormsg);
        }

        return true;
    }
};

#endif