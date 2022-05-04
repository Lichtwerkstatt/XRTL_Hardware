#if STEPPERCOUNT > 0

#include <AccelStepper.h>//none blocking library

bool wasRunning = false;
bool isInitialized = true;

AccelStepper stepperOne(AccelStepper::HALF4WIRE, STEPPERONEPINA, STEPPERONEPINB, STEPPERONEPINC, STEPPERONEPIND);
#if STEPPERCOUNT >1
AccelStepper stepperTwo(AccelStepper::HALF4WIRE, STEPPERTWOPINA, STEPPERTWOPINB, STEPPERTWOPINC, STEPPERTWOPIND);
#endif

#endif
