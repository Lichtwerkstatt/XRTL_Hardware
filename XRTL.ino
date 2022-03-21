#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <WiFi.h>
#include <LITTLEFS.h>
#define FILESYSTEM LITTLEFS

// compiler flags
#define STEPPERCOUNT 0   // current max: 2
#define SERVOCOUNT 2     // current max: 2
#define RELAISCOUNT 0    // current max: 2
#define ISCAMERA true    // true: ESP is a camera
#define HASINFOLED false // true: info LED is installed

// info LED
#define INFOLEDNUMBER 12 // number of LEDs
#define INFOLEDPIN 25

// pins for stepper
#define STEPPERONEPINA 16
#define STEPPERONEPINB 18
#define STEPPERONEPINC 17
#define STEPPERONEPIND 19

// pins for stepper two
#define STEPPERTWOPINA 27
#define STEPPERTWOPINB 33
#define STEPPERTWOPINC 26
#define STEPPERTWOPIND 32

// pins for servos
#define SERVOONEPIN 14
#define SERVOTWOPIN 15

// pins for relais
#define RELAISONEPIN 42
#define RELAISTWOPIN 42

//
#include "camera.h"
#include "stepper.h"
#include "servo.h"
#include "relais.h"
#include "socketMod.h"
#include "infoLED.h"
#include "settings.h"
#include "functions.h"


void setup() {
  XRTLsetup();
}

void loop() {
  XRTLloop();
}
