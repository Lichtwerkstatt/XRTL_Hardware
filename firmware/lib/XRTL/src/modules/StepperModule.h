#ifndef STEPPERMODULE_H
#define STEPPERMOUDLE_H

#include "AccelStepper.h"
#include "XRTLmodule.h"

class StepperModule : public XRTLmodule {
  private:
  String control;
  uint16_t accel;
  uint16_t speed;
  int32_t position;
  int32_t minimum;
  int32_t maximum;
  int32_t initial;
  bool relativeCtrl;

  uint8_t pin[4];

  bool wasRunning = false;
  bool isInitialized = true;
  
  AccelStepper* stepper = NULL;
  
  public:
  StepperModule(String moduleName, XRTL* source);

  void driveStepper(JsonObject& command);

  void saveSettings(DynamicJsonDocument& settings);
  void loadSettings(DynamicJsonDocument& settings);
  void setViaSerial();
  void getStatus(JsonObject& payload, JsonObject& status);

  void setup();
  void loop();
  void stop();

  bool handleCommand(String& command);
  bool handleCommand(String& controlId, JsonObject& command);
};

#endif
