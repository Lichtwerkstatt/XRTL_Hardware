#if SERVOCOUNT > 0

#include <ESP32Servo.h>

#if ISCAMERA
  Servo trashOne;
  Servo trashTwo;
#endif

Servo servoOne;
#if SERVOCOUNT > 1
  Servo servoTwo;
#endif

#endif
