#ifndef XRTLMODULE_H
#define XRTLMODULE_H

#include "Arduino.h"
#include "ArduinoJson.h"

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
  xrtl_output
};

// display names for modules
static const char* moduleNames[8] = {
  "socket",
  "wifi",
  "info LED",
  "stepper motor",
  "servo motor",
  "camera",
  "input",
  "output"
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
// poll input via serial monitor
// prints query on the serial monitor and records input as answer until return character is received
String serialInput(String query);
// center str on a line of targetLength, fill rest with a char
// filler = +:  ++++str++++
String centerString(String str, uint8_t targetLength, char filler);
// rescale floats, analogue to map
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

// forward declaration: need pointer
class XRTL; 

// template class for all modules
class XRTLmodule {
  protected:
  String id; // must be unique per ESP; assigned when constructed
  XRTL* xrtl; // core address, must be assigned when constructed: XRTLmodule(String id, XRTLmodule* source)
  bool debugging = true; // true: print status messages via serial monitor and accept serial events

  public:
  String getID(); // return id
  virtual moduleType getType();
  String& getComponent();
  bool isModule(String& moduleName);//  return (id == moduleName)

  virtual bool handleCommand(String& command); // react on a simple command offered by the core, return false if command is unkown
  virtual bool handleCommand(String& controlId, JsonObject& command); // react to a complex command offered by the core, return false if the module did not recognize it

  void sendEvent(JsonArray& event); // send event via endpoint
  void sendError(componentError err, String message); // send error via endpoint
  void sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length); // send binary via endpoint
  void sendStatus(); // tell core to send status

  void notify(internalEvent eventId); // send internal event to all modules
  virtual void handleInternal(internalEvent eventId, String& sourceId); // react to internal event issued by other modules

  virtual void setup(); // called once during setup
  virtual void loop();  // called once in every loop
  virtual void stop();  // stop all operation, restart of device could be imminent

  virtual void getStatus(JsonObject& payload, JsonObject& status); // edit status payload; reference to status field for efficiency
  virtual void saveSettings(JsonObject& settings);
  virtual void loadSettings(JsonObject& settings);
  virtual void setViaSerial(); // directly poll parameters via serial monitor

  // debug message over serial monitor
  // respects debugging status
  // uses printf syntax
  template<typename... Args>
  void debug(Args... args) {
    if (!debugging) return;
    Serial.printf("[%s] ", id.c_str());
    Serial.printf(args...);
    Serial.print('\n');
  }

  // get value of type A with a specific name from JsonObject file and store it in target
  // optional: send out an error if name can't be found within file, default to no error
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

  // get value of type A with specified name from JsonObject file and store it in target
  // check wether target is between minValue and maxValue and constrain it if necessary
  // optional: send out an error if name can't be found within file, defaults to no error
  template<typename A>
  bool getAndConstrainValue(String name, JsonObject& file, A&target, A minValue, A maxValue, bool reportMissingField = false) {
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