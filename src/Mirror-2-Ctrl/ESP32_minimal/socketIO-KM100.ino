/*
 * using WebSockets with socektsIO subclass by Markus Sattler:
 * https://github.com/Links2004/arduinoWebSockets
 * adapted from /examples/esp32/WebSocketClientSocketIOack/WebsocketClientSocketIOack.ino
 */

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <Stepper.h>

WiFiMulti WiFiMulti;
SocketIOclient socketIO;

// identify component
String componentID = "km100_1";

// definition for stepper motors
const int stepperOneA = 16;
const int stepperOneB = 18;
const int stepperOneC = 17;
const int stepperOneD = 19;
const int stepsPerRevolution = 2048;
int stepperOnePos;// for storing rotation
String stepperOneName = "top";
Stepper stepperOne(stepsPerRevolution, stepperOneA, stepperOneB, stepperOneC, stepperOneD);

// define events types
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.printf("[IOc] Disconnected!\n");
      break;
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to URL: %s\n", payload);

      // join default namespace
      socketIO.send(sIOtype_CONNECT, "/");
      break;
    case sIOtype_EVENT: {
      char * sptr = NULL;
      int id = strtol((char *)payload, &sptr, 10);
      Serial.printf("[IOc] get event: %s     (id: %d)\n", payload, id);
      DynamicJsonDocument incomingEvent(1024);
      DeserializationError error = deserializeJson(incomingEvent,payload,length);
      if(error) {
        Serial.printf("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
      }

      String eventName = incomingEvent[0];
      Serial.printf("[IOc] event name: %s\n", eventName.c_str());

      if (eventName == "control") {
        // analyze and store input
        JsonObject receivedPayload = incomingEvent[1];
        String component = receivedPayload["component"];
        String control = receivedPayload["control"];
        JsonObject command = receivedPayload["command"];
        int steps = command["steps"];

        // act only when involving this component
        if (component == componentID) {
          driveStepper(control,steps);
        }
      }
    }
    case sIOtype_ACK:
      Serial.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      Serial.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      Serial.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      Serial.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}

// communicate with server, drive stepper and report
void driveStepper(String stepperName, int steps) {
  // set status to busy (to be implemented)
  if (stepperName == stepperOneName) {
    stepperOnePos += steps;
    Serial.printf("moving %s by %d steps... ", stepperOneName, steps);
    stepperOne.step(steps);
    // depower stepper? Might decrease precision slightly but increase durability and save power
    digitalWrite(stepperOneA,0);
    digitalWrite(stepperOneB,0);
    digitalWrite(stepperOneC,0);
    digitalWrite(stepperOneD,0);
    Serial.printf("done. New Position: %d\n", stepperOnePos);
  }
  // set status to available (to be implemented)
  // report new position to server (to be implemented)
}

void setup() {
  // use serial for debugging
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  stepperOne.setSpeed(10);

  // boot delay
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %ds...\n",t);
    Serial.flush();
    delay(1000);
  }

  // connect to WiFi
  WiFiMulti.addAP("Himbeere", "remotelab");
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
  }
  String ip = WiFi.localIP().toString();
  Serial.printf("[SETUP] Connected to WiFi as %s\n", ip.c_str());

  // connect to socketIO server: IP, port, URL
  socketIO.begin("192.168.4.1", 7000,"/socket.io/?EIO=4");

  // pass event handler
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  socketIO.loop();
}
