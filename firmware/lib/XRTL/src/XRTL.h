#ifndef XRTL_H
#define XRTL_H

#include "modules/servo/ServoModule.h"
#include "modules/socket/SocketModule.h"
#include "modules/wifi/WifiModule.h"
#include "modules/stepper/StepperModule.h"
#include "modules/infoLED/InfoLEDModule.h"
#include "modules/output/OutputModule.h"
#include "modules/input/InputModule.h"
#include "modules/camera/CameraModule.h"
#include "modules/macro/Macromodule.h"

// core, module manager
class XRTL {
  private:
  String id = "core";
  // store modules here
  uint8_t moduleCount;
  XRTLmodule* module[16];
  // endpoint for sending
  SocketModule* socketIO = NULL;

  // enable serial debug output
  bool debugging = true;

  public:
  // manage Modules
  void listModules();
  bool addModule(String moduleName, moduleType category);
  void delModule(uint8_t number);
  void swapModules(uint8_t numberX, uint8_t numberY);
  XRTLmodule* operator[](String moduleName); // returns pointer to module with specified ID

  String& getComponent();

  // offer command events to modules
  void pushCommand(String& controlId, JsonObject& command);
  void pushCommand(String& command);

  // send stuff via endpoint
  void sendEvent(JsonArray& event);
  void sendError(componentError err, String msg);
  void sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length);

  // send string over Serial if debugging is activated
  // (uses printf syntax)
  template<typename... Args>
  void debug(Args... args) {
    if (!debugging) return;
    Serial.printf("[%s] ", id.c_str());
    Serial.printf(args...);
    Serial.print('\n');
  };

  // manage module settings
  void loadSettings();
  void saveSettings();
  void setViaSerial();
  void settingsDialogue();
  //void getStatus();
  void sendStatus();

  // calls corresponding methodes of all modules
  void setup();
  void loop();
  void stop();

  // distribute event to all modules
  void notify(internalEvent eventId, String& sourceId);
};

#endif