#include "InfoLEDModule.h"

void InfoLED::begin()
{
    led.begin();
}

void InfoLED::cycle(uint32_t t, bool b = false)
{ // t: time before stepping to the next LED; b: true -> LEDs stay on after full cycle
    interval = t;
    continueCycle = b;
    isCycling = true;
    isPulsing = false;
}

void InfoLED::pulse(uint32_t t, uint8_t minimum, uint8_t maximum)
{ // t: time before next intensity value; minimum: minimum intensity; maximum: maximum intensity
    interval = t;
    minVal = minimum;
    maxVal = maximum;
    isCycling = false;
    isPulsing = true;
}

void InfoLED::rainbow(uint16_t h)
{ // h: hue is incremented by this amount per step
    hueIncrement = h;
    isHueing = true;
    if (isCycling)
    {
        isPulsing = false;
    }
    if (isPulsing)
    {
        isCycling = false;
    }
}

void InfoLED::start()
{
    isOn = true;
}

void InfoLED::stop()
{
    isOn = false;
    led.clear();
    led.show();
}

void InfoLED::hsv(uint16_t h, uint8_t s, uint8_t v)
{
    isCycling = false;
    isHueing = false;
    isPulsing = false;
    hue = h;
    sat = s;
    val = v;
}

void InfoLED::constant()
{
    isCycling = false;
    isHueing = false;
    isPulsing = false;
}

void InfoLED::loop()
{
    if (isOn)
    {
        now = millis() - last;
        if ((isCycling) && (now > interval))
        {
            if (currentLED == led.numPixels())
            {
                currentLED = 0;
                if (!continueCycle)
                {
                    led.clear();
                }
            }
            led.setPixelColor(currentLED, led.gamma32(led.ColorHSV(hue, sat, val)));
            currentLED++;
            last = millis();
        }
        if (isHueing)
        {
            hue = hue + hueIncrement;
        }
        if ((isPulsing) && (now > interval))
        {
            if (val == minVal)
            {
                deltaVal = 1;
            }
            if (val == maxVal)
            {
                deltaVal = -1;
            }
            val += deltaVal;
            last = millis();
        }
        if (!isCycling)
        {
            for (int i = 0; i < led.numPixels(); i++)
            {
                led.setPixelColor(i, led.gamma32(led.ColorHSV(hue, sat, val)));
            }
        }
        led.show();
    }
    else if (!isOn)
    {
        led.clear();
    }
}

InfoLEDModule::InfoLEDModule(String modulName)
{
    id = modulName;

    /*parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(pin, "pin", "pin: ");
    parameters.add(pixel, "pixel", "pixel: ");*/
}

moduleType InfoLEDModule::getType()
{
    return xrtl_infoLED;
}

void InfoLEDModule::setup()
{
    led = new InfoLED(pixel, pin);
    led->begin();
    led->hsv(0, 255, 255);
    led->cycle(100, false);
    led->start();
}

void InfoLEDModule::loop()
{
    led->loop();
}

void InfoLEDModule::saveSettings(JsonObject &settings)
{
    /*settings["pin"] = pin;
    settings["pixel"] = pixel;*/
    // parameters.save(settings);
}

void InfoLEDModule::loadSettings(JsonObject &settings)
{
    /*pin = loadValue<uint8_t>("pin", settings, 32);
    pixel = loadValue<uint8_t>("pixel", settings, 12);

    if (!debugging) return;
    Serial.printf("control pin: %i\n", pin);
    Serial.printf("pixel number: %i\n", pixel);*/
    // parameters.load(settings);
    // if (debugging) parameters.print();
}

void InfoLEDModule::setViaSerial()
{
    /*Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    id = serialInput("controlId: ");

    if (serialInput("change pin binding (y/n): ") != "y") return;

    pin = serialInput("control pin: ").toInt();
    pixel = serialInput("pixel number: ").toInt(); */
    // parameters.setViaSerial();
}

void InfoLEDModule::stop()
{
    led->hsv(8000, 255, 110);
    led->constant();
    led->loop(); // make sure the changes are applied immediately
}

bool InfoLEDModule::handleCommand(String &command)
{
    if (command != "reset")
        return false;

    led->hsv(8000, 255, 110);
    led->constant();
    led->loop(); // no loop will be executed before reset

    return true;
}

void InfoLEDModule::handleCommand(String &controlId, JsonObject &command)
{
    if (!isModule(controlId))
        return;
    if (getValue<uint16_t>("hue", command, userHue))
    {
        led->hsv(userHue, 255, 110);
        led->constant();
        led->loop();
    }

    String hexRGB;
    if (getValue<String>("color", command, hexRGB))
    {
        uint8_t userVal;
        uint8_t userSat;
        hexRGBtoHSV(hexRGB, userHue, userSat, userVal);
        led->hsv(userHue, userSat, 110);
        debug("accepted color in HSV: %d, %d, %d", userHue, userSat, userVal);
        // led->constant();
        led->loop();
    }

    // return false;
}

void InfoLEDModule::handleInternal(internalEvent eventId, String &sourceId)
{
    if (led == NULL)
        return;
    switch (eventId)
    {
    case socket_disconnected:
    {
        led->hsv(0, 255, 110);
        led->pulse(0, 40, 110);
        break;
    }
    case wifi_disconnected:
    {
        led->hsv(0, 255, 110);
        led->cycle(100, false);
        break;
    }
    case socket_connected:
    {
        led->hsv(8000, 255, 110);
        break;
    }
    case socket_authed:
    {
        led->hsv(20000, 255, 110);
        led->constant();
        break;
    }
    case wifi_connected:
    {
        led->pulse(0, 40, 110);
        break;
    }

    case busy:
    {
        led->pulse(0, 40, 110);
        break;
    }
    case ready:
    {
        led->constant();
        break;
    }

    case debug_off:
    {
        debugging = false;
        break;
    }
    case debug_on:
    {
        debugging = true;
        break;
    }
    }
}

void hexRGBtoHSV(String &hexRGB, uint16_t &hueTarget, uint8_t &satTarget, uint8_t &valTarget)
{
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

    if (minChannel == maxChannel)
    {
        hue = 0;
        sat = 0;
    }
    else if (maxChannel == RGB[0])
    {
        hue = (RGB[1] - RGB[2]) / (maxChannel - minChannel);
        sat = (maxChannel - minChannel) / maxChannel * 255.0;
    }
    else if (maxChannel == RGB[1])
    {
        hue = 2.0 + (RGB[2] - RGB[0]) / (maxChannel - minChannel);
        sat = (maxChannel - minChannel) / maxChannel * 255.0;
    }
    else if (maxChannel == RGB[2])
    {
        hue = 4.0 + (RGB[0] - RGB[1]) / (maxChannel - minChannel);
        sat = (maxChannel - minChannel) / maxChannel * 255.0;
    }

    if (hue < 0)
    {
        hue += 6;
    }

    hueTarget = round(hue * 10922.5);
    satTarget = round(sat);
    valTarget = round(val);
}