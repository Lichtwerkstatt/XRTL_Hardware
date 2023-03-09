#ifndef XRTLMODULE_H
#define XRTLMODULE_H

#include "common/XRTLcommand.h"

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
static const char* moduleNames[9] = {
  "socket",
  "wifi",
  "info LED",
  "stepper motor",
  "servo motor",
  "camera",
  "input",
  "output",
  "macro"
};

//used to push state changes to other modules
enum internalEvent{
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

//identify error category
enum componentError {
  deserialize_failed,
  field_is_null,
  wrong_type,
  out_of_bounds,
  unknown_key,
  disconnected,
  is_busy
};

// forward declaration: need pointer
class XRTL; 

// template class for all modules
class XRTLmodule {
  protected:
  // @brief (supposedly) unique identifier the module listens to
  // @note if two modules share the same ID, both will react to all commands
  String id;
  XRTL* xrtl; // core address, must be assigned when constructed: XRTLmodule(String id, XRTLmodule* source)
  bool debugging = true; // true: print status messages via serial monitor and accept serial events

  public:
  String getID(); // return id
  virtual moduleType getType();
  String& getComponent();
  bool isModule(String& moduleName); // @returns (id == moduleName)

  virtual bool handleCommand(String& command); // DEPRECATED react on a simple command offered by the core, return false if command is unkown

  // @brief processes and reacts to commands
  // @param controlId reference to the separated controlId
  // @param command reference to the whole command message including the command keys
  // @note to avoid controlId being separated by every individual module, it has to be done before distributing the command to the modules
  virtual void handleCommand(String& controlId, JsonObject& command);

  // @brief instruct core to send an event
  // @param event reference to the event to send (JsonArray)
  // @note event Format: [<event name>,{<payload>}]
  void sendEvent(JsonArray& event);

  // @brief send an error via the endpoint
  // @param ernr refers to the errortype, see componentError for details
  // @param message errormessage as single string
  void sendError(componentError ernr, String message);

  // @brief send binary data via the endpoint
  // @param binaryLeadFrame complete websocket frame to be sent prior to the data frame. Must include placeholder for data, see note
  // @param payload pointer to the data to be included
  // @param length size of the data object to be sent
  // @note placeholder: 451-[<event name>,{<additional payload>,{"_placeholder":true,"num":0}}]
  void sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length);

  // @brief instruct core to send the status of this module
  // @note content of the status is filled by getStatus(), if empty no status will be send
  void sendStatus();

  // @brief send internal event to all modules
  // @param eventId type of the occuring event, see internalEvent
  // @note events can be reacted on by using handleInternal()
  void notify(internalEvent eventId);

  // @brief react to internal events issued by a module
  // @param eventId type of the occuring event
  // @param sourceId ID of the module triggering the event
  virtual void handleInternal(internalEvent eventId, String& sourceId);

  virtual void setup(); // called once during setup
  virtual void loop();  // called once in every loop
  virtual void stop();  // stop all operation, restart of device could be imminent

  // @brief define if a status should be send and what information it should contain
  // @param status JsonObject to be filled with status information by the module
  // @returns true if this module should send a status, defaults to false
  virtual bool getStatus(JsonObject& status);

  // @brief save settings to flash when a planned shutdown occurs
  // @param settings JsonObject that must be filled with the data to be saved
  // @note format: {<key1>:<value1>,...,<keyN>:<valueN>}
  virtual void saveSettings(JsonObject& settings);

  // @brief load settings when device is started
  // @param settings JsonObject containing the data for this module loaded from flash
  // @note use this.loadValue() to safely initialize the settings
  virtual void loadSettings(JsonObject& settings);

  // @brief methode called when the settings of a module are to be set via the serial interface
  // @note all neccessary information must be polled from the user, serialInput() might be used for doing so
  virtual void setViaSerial();

  // @brief send a message over the serial monitor if in debug mode
  // @param ... uses printf syntax
  template<typename... Args>
  void debug(Args... args) {
    if (!debugging) return;
    Serial.printf("[%s] ", id.c_str());
    Serial.printf(args...);
    Serial.print('\n');
  }

  // @brief fetch a key value pair from a JsonObject 
  // @param name search for this key
  // @param file object that will be searched for name
  // @param target store value here if key was found
  // @param reportMissingField if True a report will be issued in case the key could not be found
  // @returns True if the key was found
  template<typename T>
  bool getValue(String name, JsonObject& file, T& target, bool reportMissingField = false) {
    auto field = file[name];
    if (field.isNull()) {
      if (!reportMissingField) return false;

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
  // @param reportMissingField if True a report will be issued in case the key could not be found
  // @returns True if the key was found
  template<typename T>
  bool getAndConstrainValue(String name, JsonObject& file, T& target, T minValue, T maxValue, bool reportMissingField = false) {
    if (!getValue<T>(name, file, target, reportMissingField)) return false;

    if ( (target < minValue) or (target > maxValue) ) {
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

  // @brief check if a specific controlId is present on the ESP32 and return the Address if so
  // @param moduleId controlId you are trying to find
  // @returns pointer to module
  // @note If the module can't be found on this ESP32 a nullpointer is returned. In that case the specific controlId might still exist on another ESP32
  XRTLmodule* findModule(String& moduleId);

  
};

#endif