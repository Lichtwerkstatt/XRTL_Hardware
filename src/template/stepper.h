#if STEPPERCOUNT > 0

#include <AccelStepper.h>//none blocking library

bool wasRunning = false;// used to detect if a movement just finished
bool isInitialized = true;// default is no movement on initialization

/*
AccelStepper is very versatile and can be configured for a variety of motors and driving methods:
FUNCTION    for implementing own driver functions (internal use, consult documentation: https://www.airspayce.com/mikem/arduino/AccelStepper/index.html)
DRIVER      if an external motor driver is used (1 step pin, 1 direction pin)
FULL2WIRE   2 motor coil contacts, full step
FULL3WIRE   3 motor coil contacts, full step
FULL4WIRE   4 motor coil contacts, full step
HALF3WIRE   3 motor coil contacts, half step
HALF4WIRE   4 motor coil contacts, half step

For 5V 28BYJ-48 motors, HALF4WIRE performed well -- make sure to swap motor pins 2 and 3!
*/

AccelStepper stepperOne(AccelStepper::HALF4WIRE, STEPPERONEPINA, STEPPERONEPINB, STEPPERONEPINC, STEPPERONEPIND);
#if STEPPERCOUNT >1
AccelStepper stepperTwo(AccelStepper::HALF4WIRE, STEPPERTWOPINA, STEPPERTWOPINB, STEPPERTWOPINC, STEPPERTWOPIND);
#endif

#endif
