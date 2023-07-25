#include "InfoLEDModule.h"

InfoLEDModule::InfoLEDModule(String modulName) {
    id = modulName;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(pin, "pin", "pin: ");
    parameters.add(pixel, "pixel", "pixel: ");
}

InfoLEDModule::~InfoLEDModule() {
    if (led == NULL) return;
    delete led;
}

moduleType InfoLEDModule::getType() {
    return xrtl_infoLED;
}

void InfoLEDModule::setup() {
    led = new InfoLED(pixel, pin);
    led->begin();
    led->hsv(0, 255, 255);
    led->fill(true);
    led->constant();
}

void InfoLEDModule::loop() {
    led->loop();
}

void InfoLEDModule::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void InfoLEDModule::loadSettings(JsonObject &settings) {
    parameters.load(settings);
    if (debugging) parameters.print();
}

void InfoLEDModule::setViaSerial() {
    parameters.setViaSerial();
}

void InfoLEDModule::stop() {
    led->hsv(8000, 255, 110);
    led->constant();
}

bool InfoLEDModule::handleCommand(String &command) {
    if (command != "reset")
        return false;

    led->hsv(8000, 255, 110);
    led->constant();

    return true;
}

void InfoLEDModule::handleCommand(String &controlId, JsonObject &command) {
    if (!isModule(controlId))
        return;

    bool tempBool = false;
    if (getValue("hold", command, tempBool)) {
        led->hold(tempBool);
    }

    if (getValue("stop", command, tempBool)) {
        if (tempBool)
            stop();
    }

    if (getValue<uint16_t>("hue", command, userHue)) {
        led->hsv(userHue, 255, 110);
        led->constant();
        led->loop();
    }

    String hexRGB;
    if (getValue<String>("color", command, hexRGB)) {
        uint8_t userVal;
        uint8_t userSat;
        hexRGBtoHSV(hexRGB, userHue, userSat, userVal);
        led->hsv(userHue, userSat, 110);
        debug("accepted color in HSV: %d, %d, %d", userHue, userSat, userVal);
    }

    uint64_t duration;
    if (getValue("const", command, duration)) {
        led->fill(true);
        led->constant(duration);
    }

    if (getValue("pulse", command, duration)) {
        led->fill(true);
        led->pulse(40, 110, 1000, duration);
    }

    int64_t cycleDuration;
    if (getValue("cycle", command, cycleDuration)) {
        led->cycle(cycleDuration);
    }

    // return false;
}

void InfoLEDModule::handleInternal(internalEvent eventId, String &sourceId) {
    if (led == NULL)
        return;
    switch (eventId) {
    case socket_disconnected: {
        led->hsv(0, 255, 110);
        led->hold(true);
        led->cycle(5000);
        break;
    }
    case wifi_disconnected: {
        led->hsv(0, 255, 110);
        led->fill(true);
        led->hold(true);
        led->constant();
        break;
    }
    case socket_connected: {
        led->hsv(8000, 255, 110);
        led->fill(true);
        led->hold(true);
        led->constant();
        break;
    }
    case socket_authed: {
        led->hsv(20000, 255, 110);
        led->fill(true);
        led->hold(false);
        led->constant(2500);
        break;
    }
    case wifi_connected: {
        led->hsv(0, 255, 110);
        led->fill(true);
        led->hold(true);
        led->pulse(40, 100, 1000, 600); // after 600 seconds the device restarts -> pulsing until restart
        break;
    }

    case busy: {
        break;
    }
    case ready: {
        break;
    }

    case debug_off: {
        debugging = false;
        break;
    }
    case debug_on: {
        debugging = true;
        break;
    }
    }
}

/**
 * @brief convert a hexRGB string to HSV color space
 * @param hexRGB String encoding the color, must lead with #
 * @param hueTarget reference to an integer; hue will be stored here
 * @param satTarget reference to an integer; saturation will be stored here
 * @param valTarget reference to an integer; value (intensity) will be stored here
 * @note saturation and value are 8 bit unsigned integers, hue is a 16 bit unsigned integer
 */
void hexRGBtoHSV(String &hexRGB, uint16_t &hueTarget, uint8_t &satTarget, uint8_t &valTarget) {
    float RGB[3] = {0, 0, 0};
    long combinedRGB = strtol(&hexRGB[1], NULL, 16);
    RGB[0] = (combinedRGB >> 16);
    RGB[1] = (combinedRGB >> 8 & 0xFF);
    RGB[2] = (combinedRGB & 0xFF);

    float hue;
    float sat;
    float val;
    float maxChannel = max(max(RGB[0], RGB[1]), RGB[2]);
    float minChannel = min(min(RGB[0], RGB[1]), RGB[2]);
    val = maxChannel;

    if (minChannel == maxChannel) // gray => saturation to 0; hue defaults to 0 (but could be anything else)
    {
        hue = 0;
        sat = 0;
    } else if (maxChannel == RGB[0]) {
        hue = (RGB[1] - RGB[2]) / (maxChannel - minChannel);
        sat = (maxChannel - minChannel) / maxChannel * 255.0;
    } else if (maxChannel == RGB[1]) {
        hue = 2.0 + (RGB[2] - RGB[0]) / (maxChannel - minChannel);
        sat = (maxChannel - minChannel) / maxChannel * 255.0;
    } else if (maxChannel == RGB[2]) {
        hue = 4.0 + (RGB[0] - RGB[1]) / (maxChannel - minChannel);
        sat = (maxChannel - minChannel) / maxChannel * 255.0;
    }

    if (hue < 0) // if negative use periodicity to return to positive integers
    {
        hue += 6; // scaling in next step
    }

    hueTarget = round(hue * 10922.5); // bring to full uint16 range: 65535 = 6 * 10922.5
    satTarget = round(sat);
    valTarget = round(val);
}