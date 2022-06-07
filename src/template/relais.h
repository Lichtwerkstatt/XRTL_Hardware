#if RELAISCOUNT > 0

// simple class collecting methodes for switching relays

class Relais {
  public:
  bool relaisState = false;
  int relaisPin;

  void attach(int pin) {
    relaisPin = pin;
    pinMode(relaisPin, OUTPUT);
    digitalWrite(relaisPin, LOW);
  }

  void stop() {
    digitalWrite(relaisPin, LOW);
    relaisState = false;
  }

  void toggle(bool state) {
    if (state != relaisState) {
      if (relaisState) {
        digitalWrite(relaisPin, LOW);
        relaisState = false;
      }
      else if (!relaisState) {
        digitalWrite(relaisPin, HIGH);
        relaisState = true;
      }
    }
  }
};

Relais relaisOne;

#if RELAISCOUNT > 1
Relais relaisTwo;
#endif

#endif
