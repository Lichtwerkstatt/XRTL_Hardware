#include "InfoLEDModule.h"
/*
void InfoLED::begin() {
  led.begin();
}

void InfoLED::cycle(int64_t t, bool b = false){// t: time before stepping to the next LED; b: true -> LEDs stay on after full cycle
  interval = 1000*t;
  continueCycle = b;
  isCycling = true;
  isPulsing = false;
}

void InfoLED::pulse(int64_t t, uint8_t minimum, uint8_t maximum) {// t: time before next intensity value; minimum: minimum intensity; maximum: maximum intensity
  interval = 1000*t;
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
  if (!isOn) return;

  now = esp_timer_get_time();

  if (now < next) return;
  next = now + interval;

  if (isPulsing) {
    if (val == minVal) deltaVal = 1;
    if (val == maxVal) deltaVal = -1;
    val += deltaVal;
  }

  if (isHueing) {
    hue = hue + hueIncrement;
  }

  if (isCycling) {
    uint32_t gamma = led.gamma32(led.ColorHSV(hue,sat,val));
    if (currentLED == led.numPixels()) {
      currentLED = 0;
      if (!continueCycle) led.clear();
    }
    led.setPixelColor(currentLED++, gamma);
  }
  else {
    uint32_t gamma = led.gamma32(led.ColorHSV(hue,sat,val));
    for (int i = 0; i < led.numPixels(); i++) {
      led.setPixelColor(i, gamma);
    }
  }

  led.show();
}
*/

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

void InfoLEDModule::saveSettings(DynamicJsonDocument& settings) {
  JsonObject saving = settings.createNestedObject(id);
  saving["pin"] = pin;
  saving["pixel"] = pixel;
}

void InfoLEDModule::loadSettings(DynamicJsonDocument& settings) {
  JsonObject loaded = settings[id];
  pin = loaded["pin"].as<uint8_t>();
  pixel = loaded["pixel"].as<uint8_t>();

  if (pin == 0) {
    pin = 32;
  }

  if (pixel == 0) {
    pixel = 12;
  }

  if (!debugging) return;
  Serial.println("");
  Serial.println(centerString("",39,'-'));
  Serial.println(centerString(id, 39, ' '));
  Serial.println(centerString("",39,'-'));
  Serial.println("");

  Serial.printf("control pin: %i\n", pin);

  Serial.printf("pixel number: %i\n", pixel);
}

void InfoLEDModule::setViaSerial() {
  Serial.println("");
  Serial.println(centerString("",39,'-'));
  Serial.println(centerString(id, 39, ' '));
  Serial.println(centerString("",39,'-'));
  Serial.println("");

  if (strcmp(serialInput("change pin binding (y/n): ").c_str(), "y") != 0) return;
  
  pin = serialInput("control pin: ").toInt();
  pixel = serialInput("pixel number: ").toInt(); 
}

void InfoLEDModule::stop() {
  led->hsv(8000,255,255);
  led->constant();
  led->loop();
}

void InfoLEDModule::handleInternal(internalEvent event) {
  if (led == NULL) return;
  switch(event) {
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

template<typename... Args>
void InfoLEDModule::debug(Args... args) {
  if(!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};