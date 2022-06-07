#include <Arduino.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>//Websockets by Markus Sattler
#include <SocketIOclient.h>
#include <WiFi.h>
#include <LittleFS.h>
#define FILESYSTEM LittleFS

// compile parameters
#define STEPPERCOUNT 0   // current max: 2
#define SERVOCOUNT 2     // current max: 2
#define RELAISCOUNT 0    // current max: 2
#define ISCAMERA true   // true: ESP is a camera
#define HASINFOLED false  // true: info LED is installed

// info LED
#define INFOLEDNUMBER 12 // number of LEDs on the stripe/ring
#define INFOLEDPIN 32

// pins for stepper
#define STEPPERONEPINA 19
#define STEPPERONEPINB 22
#define STEPPERONEPINC 21
#define STEPPERONEPIND 23

// pins for stepper two
#define STEPPERTWOPINA 4
#define STEPPERTWOPINB 17
#define STEPPERTWOPINC 16
#define STEPPERTWOPIND 18

// pins for servos
#define SERVOONEPIN 33
#define SERVOTWOPIN 25

// pins for relays
#define RELAISONEPIN 27
#define RELAISTWOPIN 26

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
