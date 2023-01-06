#include "InfoLEDModule.h"

void InfoLED::begin() {
  led.begin();
}

void InfoLED::cycle(uint32_t t, bool b = false){// t: time before stepping to the next LED; b: true -> LEDs stay on after full cycle
  interval = t;
  continueCycle = b;
  isCycling = true;
  isPulsing = false;
}

void InfoLED::pulse(uint32_t t, uint8_t minimum, uint8_t maximum) {// t: time before next intensity value; minimum: minimum intensity; maximum: maximum intensity
  interval = t;
  minVal = minimum;
  maxVal = maximum;
  isCycling = false;
  isPulsing = true;
}

void InfoLED::rainbow(uint16_t h) {// h: hue is incremented by this amount per step
  hueIncrement = h;
  isHueing = true;
  if (isCycling) {
    isPulsing = false;
  }
  if (isPulsing) {
    isCycling = false;
  }
}

void InfoLED::start() {
  isOn = true;
}

void InfoLED::stop() {
  isOn = false;
  led.clear();
  led.show();
}

void InfoLED::hsv(uint16_t h, uint8_t s, uint8_t v) {
  isCycling = false;
  isHueing = false;
  isPulsing = false;
  hue = h;
  sat = s;
  val = v;
}

void InfoLED::constant() {
  isCycling = false;
  isHueing = false;
  isPulsing = false;
}

void InfoLED::loop() {
  if (isOn) {
    now = millis() - last;
    if ( (isCycling) && (now > interval) ) {
      if (currentLED == led.numPixels()) {
        currentLED = 0;
        if (!continueCycle) {
          led.clear();
        }
      }
      led.setPixelColor(currentLED, led.gamma32(led.ColorHSV(hue, sat, val)));
      currentLED++;
      last = millis();
    }
    if (isHueing) {
      hue = hue + hueIncrement;
    }
    if ( (isPulsing) && (now > interval) ) {
      if (val == minVal) {
        deltaVal = 1;
      }
      if (val == maxVal) {
        deltaVal = -1;
      }
      val += deltaVal;
      last = millis();
    }
    if (!isCycling){
      for (int i = 0; i < led.numPixels(); i++) {
        led.setPixelColor(i, led.gamma32(led.ColorHSV(hue,sat,val)));
      }
    }
    led.show();
  }
  else if (!isOn) {
    led.clear();
  }
}

InfoLEDModule::InfoLEDModule(String modulName, XRTL* source) {
  id = modulName;
  xrtl = source;
}

moduleType InfoLEDModule::getType() {
  return xrtl_infoLED;
}

void InfoLEDModule::setup() {
  led = new InfoLED(pixel,pin);
  led->begin();
  led->hsv(0,255,255);
  led->cycle(100,false);
  led->start();
}

void InfoLEDModule::loop() {
  led->loop();
}

void InfoLEDModule::saveSettings(JsonObject& settings) {
  //JsonObject saving = settings.createNestedObject(id);
  settings["pin"] = pin;
  settings["pixel"] = pixel;
}

void InfoLEDModule::loadSettings(JsonObject& settings) {
  //JsonObject loaded = settings[id];

  pin = loadValue<uint8_t>("pin", settings, 32);
  pixel = loadValue<uint8_t>("pixel", settings, 12);

  if (!debugging) return;
  Serial.printf("control pin: %i\n", pin);
  Serial.printf("pixel number: %i\n", pixel);
}

void InfoLEDModule::setViaSerial() {
  Serial.println("");
  Serial.println(centerString("",39,'-'));
  Serial.println(centerString(id, 39, ' '));
  Serial.println(centerString("",39,'-'));
  Serial.println("");

  if (serialInput("change pin binding (y/n): ") != "y") return;
  
  pin = serialInput("control pin: ").toInt();
  pixel = serialInput("pixel number: ").toInt(); 
}

void InfoLEDModule::stop() {
  led->hsv(8000,255,255);
  led->constant();
  led->loop();// make sure the changes are applied immediately
}

bool InfoLEDModule::handleCommand(String& command) {
  if (command != "reset") return false;

  led->hsv(8000,255,255);
  led->constant();
  led->loop();// no loop will be executed before reset

  return true;
}

void InfoLEDModule::handleCommand(String& controlId, JsonObject& command) {
  if (!isModule(controlId)) return;
  if (getValue<uint16_t>("", command, userHue)) {
    led->hsv(userHue, 255, 40);
    //return true;
  }
  
  //return false;
}

void InfoLEDModule::handleInternal(internalEvent eventId, String& sourceId) {
  if (led == NULL) return;
  switch(eventId) {
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
      led->hsv(8000,255,110);
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
      led->pulse(0,40,110);
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