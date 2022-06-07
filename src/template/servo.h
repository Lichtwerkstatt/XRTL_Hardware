#if SERVOCOUNT > 0

#include <ESP32Servo.h>

#if ISCAMERA
  // the ESP32 camera module as well as this servo library utilize the internal ESP32 PWM generators
  // to avoid conflicts the first two called instances of the servo MUST NOT be initialized
  // if this servo instances are touched whatsoever the camera will not function!
  Servo trashOne;
  Servo trashTwo;
#endif

Servo servoOne;

#if SERVOCOUNT > 1
  Servo servoTwo;
#endif

#endif
