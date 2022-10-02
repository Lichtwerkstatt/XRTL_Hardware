#ifndef INFOLEDMODULE_H
#define INFOLEDMODULE_H

#include "Adafruit_NeoPixel.h"
#include "XRTLmodule.h"

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
  // must be called frequently whenever a none constant light pattern is intended to show
  void loop();
};

class InfoLEDModule : public XRTLmodule {
  private:
  uint8_t pin = 32;
  uint8_t pixel = 12;
  InfoLED* led = NULL;
  
  public:
  InfoLEDModule(String moduleName, XRTL* source);
  moduleType getType();

  void saveSettings(JsonObject& settings);
  void loadSettings(JsonObject& settings);
  void setViaSerial();

  void setup();
  void loop();
  void stop();

  void handleInternal(internalEvent event);
  bool handleCommand(String& command);
};


#endif
