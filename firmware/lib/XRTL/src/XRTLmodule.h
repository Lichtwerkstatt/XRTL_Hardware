#ifndef XRTLMODULE_H
#define XRTLMODULE_H

#include "Arduino.h"
#include "ArduinoJson.h"

// @brief safely load a setting value from a file
// @param key name of the key that is searched for
// @param file will be searched for key
// @param defaultValue return this value if key is missing
// @returns value read from file or defaultValue if key is missing
template <typename T>
T loadValue(String key, JsonObject& file, T defaultValue) {
  auto field = file[key];
  if ( field.isNull() ) {
    Serial.printf("WARNING: %s not set, using default value\n", key.c_str());
    return defaultValue;
  }
  return field.as<T>();
}

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



//push state changes to other modules
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

// return type for reading values from JsonObjects
enum getValueReturn_t {
  is_missing,
  is_first,
  is_second,
  is_wrong_type
};

// @brief poll user input via serial monitor
// @param query user will be presented with this string before the program waits for input
// @returns user input until the first return character is received
String serialInput(String query);

// @brief center str on a line, fill remaining space with characters
// @param str string to be centered
// @param targetLength total length of the line
// @param filler remaining space is filled with this char
// @note example: filler + =>  ++++str++++
String centerString(String str, uint8_t targetLength, char filler);

// @brief rescale floats, analogue to map
// @param x value to be mapped
// @param in_min minimum value of input range
// @param in_max maximum value of input range
// @param out_min minimum value of output range
// @param out_max maximum value of output range
// @returns value linearly mapped to output range
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

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
  // @param status JsonObject that must be filled with status information
  // @returns true if this module should send a status, defaults to false
  virtual bool getStatus(JsonObject& status);

  // @brief save settings to flash when a planned shutdown occurs
  // @param settings JsonObject that must be filled with the data to be saved
  // @note format: {<key1>:<value1>,...,<keyN>:<valueN>}
  virtual void saveSettings(JsonObject& settings);

  // @brief load settings when device is started
  // @param settings JsonObject containing the data for this module loaded from flash
  // @note use loadValue() to safely initialize the settings
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
  template<typename A>
  bool getValue(String name, JsonObject& file, A& target, bool reportMissingField = false) {
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

    if (!field.is<A>()) {
      String errormsg = "[";
      errormsg += id;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> is of wrong type";
      sendError(wrong_type, errormsg);
      return false;
    }
    
    target = field.as<A>();
    return true;
  }

  template<typename A, typename B>
  getValueReturn_t getValue(String name, JsonObject& file, A& targetA, B& targetB, bool reportMissingField = false) {
    auto field = file[name];
    if (field.isNull()) {
      if (!reportMissingField) return is_missing;

      String errormsg = "[";
      errormsg += id;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> is missing";
      
      sendError(field_is_null, errormsg);
      return is_missing;
    }

    if (field.is<A>()) {
      targetA = field.as<A>();
      return is_first;
    }
    else if (field.is<B>()) {
      targetB = field.as<B>();
      return is_second;
    }
    else {
      String errormsg = "[";
      errormsg += id;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> is wrong type";

      sendError(wrong_type, errormsg);
    }
    
    return is_wrong_type;
  }

  // @brief fetch a key value pair from a JsonObject and constrain the value to a specified interval
  // @param name search for this key
  // @param file object that will be searched for name
  // @param target store value here if key was found
  // @param minValue lower bound of the constraining interval
  // @param maxValue upper bound of the constraining interval
  // @param reportMissingField if True a report will be issued in case the key could not be found
  // @returns True if the key was found
  template<typename A>
  bool getAndConstrainValue(String name, JsonObject& file, A& target, A minValue, A maxValue, bool reportMissingField = false) {
    bool ret = getValue<A>(name, file, target, reportMissingField);
    if (!ret) return ret;

    if ( (target < minValue) or (target > maxValue) ) {
      target = constrain(target,minValue,maxValue);

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

    return ret;
  }
};

#endif