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
  socket,
  wifi,
  infoLED,
  stepper,
  servo,
  camera,
  input,
  output
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

// poll input via serial monitor
String serialInput(String query);
// filler = +    ==>  ++++str++++
String centerString(String str, uint8_t targetLength, char filler);
// warning message
String missingValue(String name);
//
// rescale floats
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

class XRTL; // declare for using core pointer

// template class for all modules
class XRTLmodule {
  protected:
  String id; // must be unique per ESP; assigned when constructed
  String controlId; // can potentially be uinified with ID?
  XRTL* xrtl; // core address, must be assigned when constructed: XRTLmodule(String id, XRTLmodule* source)
  bool debugging = true; // print status messages via serial monitor; debug methode can't be inherited and must be implemented for every module type

  public:
  String getID(); // return id
  String& getComponent();
  bool isModule(String& moduleName);//  return (id == moduleName)

  virtual bool handleCommand(String& command); // react on a simple command offered by the core, return false if command is unkown
  virtual bool handleCommand(String& controlId, JsonObject& command); // react to a complex command offered by the core, return false if command is unknown

  void sendEvent(JsonArray& event); // send event via endpoint
  void sendError(componentError err, String message); // send error via endpoint
  void sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length); // send binary via endpoint
  void sendStatus(); // tell core to send status

  void notify(internalEvent state); // send internal event to all modules
  virtual void handleInternal(internalEvent event); // react to internal event issued by other modules

  virtual void setup(); // called once during setup
  virtual void loop();  // called once in every loop
  virtual void stop();  // deinit

  virtual void getStatus(JsonObject& payload, JsonObject& status); // edit status payload; pointer to status field for conveniance
  virtual void saveSettings(DynamicJsonDocument& settings); // determines how and where parameters are to be written in settings file
  virtual void loadSettings(DynamicJsonDocument& settings); // determines how and where parameters are to be read from settings file
  virtual void setViaSerial(); // directly poll parameters via serial monitor

  // debug message printing the module ID
  template<typename... Args>
  void debug(Args... args) {
    if (!debugging) return;
    Serial.printf("[%s] ", id.c_str());
    Serial.printf(args...);
    Serial.print('\n');
  }

  // get value of type A from JsonObject 
  template<typename A>
  bool getValue(String name, JsonObject& file, A& target, bool reportMissingField = false) {
    auto field = file[name];
    if (field.isNull()) {
      if (!reportMissingField) return false;

      String errormsg = "[";
      errormsg += controlId;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> could not be found";
      sendError(field_is_null, errormsg);
      return false;
    }

    if (!field.is<A>()) {
      String errormsg = "[";
      errormsg += controlId;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> is of wrong type";
      sendError(wrong_type, errormsg);
      return false;
    }
    
    target = field.as<A>();
    return true;
  }

  template<typename A>
  bool getValue(String name, JsonObject& file, A&target, A minValue, A maxValue, bool reportMissingField = false) {
    bool ret = getValue<A>(name, file, target, reportMissingField);
    if (!ret) return ret;

    if ( (target < minValue) or (target > maxValue) ) {
      target = constrain(target,minValue,maxValue);

      String errormsg = "[";
      errormsg += controlId;
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

  // get value of type A or B from JsonObject -- A is the return type, make sure B can be cast to A
  // deprecated with new command structure?
  template<typename A, typename B>
  bool getValue(String name, JsonObject& file, A& target, bool reportMissingField = false) {
    auto field = file[name];
    if (field.isNull()) {
      if (!reportMissingField) return false;

      String errormsg = "[";
      errormsg += controlId;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> could not be found";
      sendError(field_is_null, errormsg);
    }

    if (!(field.is<A>() or field.is<B>())) {
      String errormsg = "[";
      errormsg += controlId;
      errormsg += "] command rejected: <";
      errormsg += name;
      errormsg += "> is of wrong type";
      sendError(wrong_type, errormsg);
      return false;
    }

    target = field.as<A>();
    return true;
  }
};

#endif