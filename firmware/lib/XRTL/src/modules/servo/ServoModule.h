#ifndef SERVOMODULE_H
#define SERVOMODULE_H

#include "ESP32Servo.h"
#include "modules/XRTLmodule.h"

class ServoModule : public XRTLmodule {
  private:
  uint16_t frequency;// servo frequency in Hz
  uint16_t minDuty;// minimum duty time in µs
  uint16_t maxDuty;// maximum duty time in µs
  int16_t minAngle;// minimum of value range that duty gets maped to
  int16_t maxAngle;// maximum of value range that duty gets maped to
  float maxSpeed;
  
  uint32_t timeStep;
  int16_t initial;// value between minAngle and maxAngle that servo gets initialized with
  // TODO: implement

  int64_t nextStep;
  uint16_t targetDuty;
  uint16_t currentDuty;
  bool positiveDirection;
  bool wasRunning = false;
  bool holdOn = false;

  uint8_t pin = 26;
  
  Servo* servo = NULL;// pointer to the servo module

  public:
  ServoModule(String moduleName, XRTL* source);
  moduleType getType();

  int16_t read();// read the current servo position, delivered on value range
  void write(int16_t target);// move servo to target on the value range
  void driveServo(JsonObject& command);// process the command object and move servo if needed
  
  bool handleCommand(String& command);
  void handleCommand(String& controlId, JsonObject& command);

  void saveSettings(JsonObject& settings);
  void loadSettings(JsonObject& settings);
  void setViaSerial();
  bool getStatus(JsonObject& status);

  void setup();
  void loop();
  void stop();
};

#endif
