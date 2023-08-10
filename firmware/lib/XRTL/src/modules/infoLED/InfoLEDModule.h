#ifndef INFOLEDMODULE_H
#define INFOLEDMODULE_H

#include "modules/XRTLmodule.h"
#include "modules/infoLED/InfoLED.h"

class InfoLEDModule : public XRTLmodule {
private:
    uint8_t pin = 32;
    uint8_t pixel = 12;
    InfoLED *led = NULL;

    uint16_t userHue = 20000;

public:
    InfoLEDModule(String moduleName);
    ~InfoLEDModule();
    moduleType type = xrtl_infoLED;
    moduleType getType();

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    void setViaSerial();

    void setup();
    void loop();
    void stop();

    void handleInternal(internalEvent eventId, String &sourceId);
    void handleCommand(String &controlId, JsonObject &command);
};

void hexRGBtoHSV(String &hexRGB, uint16_t &hueTarget, uint8_t &satTarget, uint8_t &valTarget);

#endif
