#ifndef SERVOMODULE_H
#define SERVOMODULE_H

#include "ESP32Servo.h"
#include "XRTLmodule.h"

class ServoModule : public XRTLmodule {
  private:
  String controlId;
  uint16_t frequency;
  uint16_t minDuty;
  uint16_t maxDuty;
  int16_t minAngle;
  int16_t maxAngle;
  int16_t initial;
  bool relativeCtrl;

  uint8_t pin = 26;
  
  Servo* servo = NULL;

  public:
  ServoModule(String moduleName, XRTL* source);

  int16_t read();
  void write(int16_t target);
  void driveServo(JsonObject& command);
  
  bool handleCommand(String& command);
  bool handleCommand(String& control, JsonObject& command);

  void saveSettings(DynamicJsonDocument& settings);
  void loadSettings(DynamicJsonDocument& settings);
  void setViaSerial();
  void getStatus(JsonObject& payload, JsonObject& status);

  void setup();
  void loop();
};

#endif
