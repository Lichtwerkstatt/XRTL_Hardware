#ifndef STEPPERMODULE_H
#define STEPPERMOUDLE_H

#include "AccelStepper.h"
#include "modules/XRTLmodule.h"

class StepperModule : public XRTLmodule {
  private:
  uint16_t accel;
  uint16_t speed;
  int32_t position;
  int32_t minimum;
  int32_t maximum;
  int32_t initial;

  uint8_t pin[4];

  bool wasRunning = false;
  bool isInitialized = true;
  bool holdOn = false;
  
  AccelStepper* stepper = NULL;
  
  public:
  StepperModule(String moduleName);
  moduleType type = xrtl_stepper;
  moduleType getType();

  void driveStepper(JsonObject& command);

  void saveSettings(JsonObject& settings);
  void loadSettings(JsonObject& settings);
  void setViaSerial();
  bool getStatus(JsonObject& status);

  void setup();
  void loop();
  void stop();

  bool handleCommand(String& command);
  void handleCommand(String& controlId, JsonObject& command);
};

#endif
