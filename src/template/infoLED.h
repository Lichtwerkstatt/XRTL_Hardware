#if HASINFOLED
#include "Adafruit_NeoPixel.h"

// wrapper for the adafruit neopixel library to display simple (non blocking) light patterns

class InfoLED {
  public:
  InfoLED(uint8_t n, uint8_t pin) : led(n, pin, NEO_GRB + NEO_KHZ800){}
  Adafruit_NeoPixel led;
  uint8_t currentLED = 12;
  uint32_t interval = 200;
  uint32_t now;
  uint32_t last;

  uint16_t hue;
  uint8_t sat = 255;
  uint8_t val = 255;

  uint16_t hueIncrement = 10;

  uint8_t deltaVal = 1;
  uint8_t maxVal = 255;
  uint8_t minVal = 0;

  bool isOn = false;
  bool isPulsing = false;
  bool isCycling = false;
  bool isHueing = false;
  bool continueCycle = false;

  void begin();

  // let a value "run" over the LED chain
  void cycle(uint32_t t, bool b);

  // pulse brightness between minimum and maximum
  void pulse(uint32_t t, uint8_t minimum, uint8_t maximum);

  // gradually change hue, can be called with cycle() or pulse() but not both simultaneously
  void rainbow(uint16_t h);

  // enables the LEDs, also continues patterns after stop() is called
  void start();

  // turns off all LEDs until start is called
  void stop();

  // set a constant HSV to be shown
  void hsv(uint16_t h, uint8_t s, uint8_t v);

  // disables all dynamics, whatever HSV was shown last remains
  void constant();

  // check the selected pattern and change the corresponding values if necessary
  // must be called whenever a none constant light pattern should be displayed
  void loop();
};

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

InfoLED infoLED(INFOLEDNUMBER, INFOLEDPIN);
#endif
