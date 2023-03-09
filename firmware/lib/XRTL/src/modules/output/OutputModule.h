#ifndef OUTPUTMODULE_H
#define OUTPUTMODULE_H

#include "XRTLoutput.h"
#include "modules/XRTLmodule.h"

// XRTLmodule for controling output signals, either relays or MOSFETs operated via PWM
class OutputModule: public XRTLmodule {
    private:

    uint8_t pin;

    // @brief controls the output behavior
    // @note True: output is 8 bit pwm; False: output is a relay
    bool pwm = false;
    // @brief pwm channel of the ESP used to control output
    // @note do NOT use channel 1 or 2 with ESP32CAM
    uint8_t channel;
    // @brief pwm frequency in Hz
    // @note permitted values: 1000, 5000, 8000, 10000 
    uint16_t frequency;

    // @brief esp_timer value (µs) at which the module is switched off again
    int64_t switchTime = 0;

    // @brief ID (string) of the guarded module
    // @note trigger events of this module will cause immediate powerdown of the output
    String guardedModule = "";

    XRTLoutput* out = NULL; 

    public:

    OutputModule(String moduleName, XRTL* source);
    moduleType getType();

    void pulse(uint16_t milliSeconds);

    void setup();
    void loop();
    void stop();

    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
    bool getStatus(JsonObject& status);

    void handleInternal(internalEvent eventId, String& sourceId);
    void handleCommand(String& controlId, JsonObject& command);
};

#endif