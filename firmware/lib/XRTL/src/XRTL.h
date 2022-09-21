#ifndef XRTL_H
#define XRTL_H

#include "ArduinoJson.h"
#include "LittleFS.h"
#include "XRTLmodule.h"

#include "modules/ServoModule.h"
#include "modules/SocketModule.h"
#include "modules/WifiModule.h"
#include "modules/StepperModule.h"
#include "modules/InfoLEDModule.h"
#include "modules/OutputModule.h"
#include "modules/InputModule.h"
#include "modules/CameraModule.h"

// core, manages modules
class XRTL {
  private:
  // store modules here
  uint8_t moduleCount;
  XRTLmodule* module[16];
  // endpoint for sending
  SocketModule* socketIO = NULL;

  // enable serial debug output
  bool debugging = true;

  public:
  // add and address module
  void addModule(String moduleName, moduleType category);
  XRTLmodule* operator[](String moduleName); // returns pointer to module with specified ID

  // offer command events to modules
  void pushCommand(JsonObject& command);
  void pushCommand(String& command);

  // send stuff via endpoint
  void sendEvent(JsonArray& event);
  void sendError(componentError err, String msg);
  void sendBinary(String* binaryLeadFrame, uint8_t* payload, size_t length);

  // send string over Serial if debugging is activated (printf syntax)
  template<typename... Args>
  void debug(Args... args) {
    if (!debugging) return;
    Serial.print("[core] ");
    Serial.printf(args...);
    Serial.print('\n');
  };

  // manage active modules
  void loadConfig();
  void editConfig();

  // manage module settings
  void loadSettings();
  void saveSettings();
  void setViaSerial();
  void getStatus();

  // calls corresponding methodes of all modules
  void setup();
  void loop();
  void stop();

  // distribute event to all modules
  void notify(internalEvent event);
};

#endif