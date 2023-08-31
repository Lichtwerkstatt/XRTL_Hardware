#ifndef SERVOMODULE_H
#define SERVOMODULE_H

#include "modules/XRTLmodule.h"

class ServoModule : public XRTLmodule {
private:
    uint8_t channel = 3;     // ledc channel. Note: some channels share timers and therefore might affect each other. Don't use channels 1 & 2 with ESP32Cam active.
    uint16_t frequency = 50; // servo frequency in Hz
    uint16_t minDuty = 1000; // minimum duty time in µs
    uint16_t maxDuty = 2000; // maximum duty time in µs
    float minAngle = 0;      // minimum of value range that duty gets maped to
    float maxAngle = 90;     // maximum of value range that duty gets maped to
    float maxSpeed = 0.0;

    double tickScalingFactor; // used to scale the duty cycle (in µs) to ticks
    uint32_t timeStep;
    float initial = 0;       // value between minAngle and maxAngle that servo gets initialized with

    int64_t nextStep;
    uint16_t targetDuty;
    uint16_t currentDuty;
    bool positiveDirection;
    bool wasRunning = false;
    bool holdOn = false;
    bool isHolding = false;

    uint8_t pin = 26;

    String infoLED = "";

public:
    ServoModule(String moduleName);
    moduleType type = xrtl_servo;
    moduleType getType();

    float read();                         // read the current servo position, delivered on value range
    void write(float target);             // move servo to target value on value range
    void writeDuty(uint16_t target);      // move servo to target duty
    void driveServo(JsonObject &command); // process the command object and move servo if needed

    void handleCommand(String &controlId, JsonObject &command);

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    void setViaSerial();
    bool getStatus(JsonObject &status);

    void setup();
    void loop();
    void stop();
};

#endif
