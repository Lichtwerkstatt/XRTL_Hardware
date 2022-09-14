#ifndef XRTLMODULE_H
#define XRTLMODULE_H

#include "Arduino.h"
#include "ArduinoJson.h"

//which module to initiate?
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
// rescale floats
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max);

class XRTL; // declare for using core pointer

// template class for all modules
class XRTLmodule {
  protected:
  String id; // must be unique per ESP; assigned when constructed
  XRTL* xrtl; // core address, must be assigned when constructed: XRTLmodule(String id, XRTLmodule* source)
  bool debugging = true; // print status messages via serial monitor; debug methode can't be inherited and must be implemented for every module type

  public:
  String getID(); // return id
  bool isModule(String& moduleName);//  return (id == moduleName)

  virtual bool handleCommand(String& command); // react on a simple command offered by the core, return false if command is unkown
  virtual bool handleCommand(String& controlId, JsonObject& command); // react to a complex command offered by the core, return false if command is unknown

  void sendEvent(JsonArray& event); // send event via endpoint
  void sendError(componentError err, String message); // send error via endpoint
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
};

#endif