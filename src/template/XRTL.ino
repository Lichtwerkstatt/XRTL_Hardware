#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>//Websockets by Markus Sattler
#include <SocketIOclient.h>
#include <WiFi.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

// compile parameters
#define STEPPERCOUNT 2   // current max: 2
#define SERVOCOUNT 0     // current max: 2
#define RELAISCOUNT 0    // current max: 2
#define ISCAMERA false   // true: ESP is a camera
#define HASINFOLED true  // true: info LED is installed

// info LED
#define INFOLEDNUMBER 12 // number of LEDs on the stripe/ring
#define INFOLEDPIN 23

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
#define RELAISONEPIN 13
#define RELAISTWOPIN 42

// subfiles:
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
