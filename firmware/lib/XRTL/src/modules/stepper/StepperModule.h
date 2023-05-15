#ifndef STEPPERMODULE_H
#define STEPPERMOUDLE_H

#include "AccelStepper.h"
#include "modules/XRTLmodule.h"

class StepperModule : public XRTLmodule
{
private:
    uint16_t accel = 500;
    uint16_t speed = 500;
    int32_t position = 0;
    int32_t minimum = -10000;
    int32_t maximum = 10000;
    int32_t initial = 0;

    uint8_t pin[4] = {19, 22, 21, 23};

    bool wasRunning = false;
    bool isInitialized = true;
    bool holdOn = false;

    AccelStepper *stepper = NULL;
    String infoLED = "";

public:
    StepperModule(String moduleName);
    moduleType type = xrtl_stepper;
    moduleType getType();

    void driveStepper(JsonObject &command);

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    void setViaSerial();
    bool getStatus(JsonObject &status);

    void setup();
    void loop();
    void stop();

    bool handleCommand(String &command);
    void handleCommand(String &controlId, JsonObject &command);
};

#endif
