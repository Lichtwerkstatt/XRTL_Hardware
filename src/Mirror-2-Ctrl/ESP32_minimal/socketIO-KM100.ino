/*
 * using WebSockets with socektsIO subclass by Markus Sattler:
 * https://github.com/Links2004/arduinoWebSockets
 * adapted from /examples/esp32/WebSocketClientSocketIOack/WebsocketClientSocketIOack.ino
 */

#include <Arduino.h>
#include <WiFi.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <AccelStepper.h>
#include <Preferences.h>

SocketIOclient socketIO;

/* save changes in rotation to flash. Note that this only logs software changes and 
does NOT keep track of physical influences like skips and forced rotations.
Number of write cycles limited, use EEPROM with care. */
Preferences preferences;

// identify component
String componentID = "km100_1";
bool busyState = false;
bool wasRunning = false;

// definition for stepper motors
const int stepperOneA = 16;
const int stepperOneB = 18;
const int stepperOneC = 17;
const int stepperOneD = 19;
const int stepsPerRevolution = 2048;
String stepperOneName = "top";
AccelStepper stepperOne(AccelStepper::HALF4WIRE, stepperOneA, stepperOneB, stepperOneC, stepperOneD);

const int stepperTwoA = 32;
const int stepperTwoB = 25;
const int stepperTwoC = 33;
const int stepperTwoD = 26;
String stepperTwoName = "bottom";
AccelStepper stepperTwo(AccelStepper::HALF4WIRE, stepperTwoA, stepperTwoB, stepperTwoC, stepperTwoD);

// define event types
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

        // act only when involving this component
        if (component == componentID) {
          // check for simple or extended command structure
          if (receivedPayload["command"].is<JsonObject>()) {
            JsonObject command = receivedPayload["command"];
            String control = receivedPayload["control"];
            int steps = command["steps"];
            driveStepper(control,steps);
          }
          else if (receivedPayload["command"].is<String>()) {
            String command = receivedPayload["command"];
            if (command == "getStatus") {
              reportState();
            }
            if (command == "restart") {
              Serial.println("Received restart command. Disconnecting now.");
              preferences.putLong("stepperOnePos",stepperOne.currentPosition());
              preferences.putLong("stepperTwoPos",stepperTwo.currentPosition());
              WiFi.disconnect();
              ESP.restart();
            }
          }
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

void reportState() {
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  payload.add("status");
  JsonObject parameters = payload.createNestedObject();
  parameters["component"] = componentID;
  JsonObject state = parameters.createNestedObject("status");
  state["busy"] = busyState;
  state[stepperOneName] = stepperOne.currentPosition();
  state[stepperTwoName] = stepperTwo.currentPosition();
  String output;
  serializeJson(payload, output);
  socketIO.sendEVENT(output);
  Serial.print("string sent: ");
  Serial.println(output);
}

void depowerStepper() {
  digitalWrite(stepperOneA,0);
  digitalWrite(stepperOneB,0);
  digitalWrite(stepperOneC,0);
  digitalWrite(stepperOneD,0);
  digitalWrite(stepperTwoA,0);
  digitalWrite(stepperTwoB,0);
  digitalWrite(stepperTwoC,0);
  digitalWrite(stepperTwoD,0);
}

// communicate with server, drive stepper and report
void driveStepper(String stepperName, int steps) {
  busyState = true;
  reportState();
  if (stepperName == stepperOneName) {
    Serial.printf("moving %s by %d steps... ", stepperOneName, steps);
    wasRunning = true;
    stepperOne.move(steps);
  }
  if (stepperName == stepperTwoName) {
    Serial.printf("moving %s by %d steps... ", stepperTwoName, steps);
    stepperTwo.move(steps);
  }
}

void setup() {
  // use serial for debugging
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  stepperOne.setMaxSpeed(250);
  stepperOne.setAcceleration(250);
  stepperTwo.setMaxSpeed(250);
  stepperTwo.setAcceleration(250);

  preferences.begin(componentID.c_str(), false);
  stepperOne.setCurrentPosition(preferences.getLong("stepperOnePos",0));
  stepperTwo.setCurrentPosition(preferences.getLong("stepperTwoPos",0));

  // boot delay
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %ds...\n",t);
    Serial.flush();
    delay(1000);
  }

  // connect to WiFi
  WiFi.begin("Himbeere", "remotelab");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  String ip = WiFi.localIP().toString();
  Serial.printf("[SETUP] Connected to WiFi as %s\n", ip.c_str());

  // connect to socketIO server: IP, port, URL
  socketIO.begin("192.168.4.1", 7000,"/socket.io/?EIO=4");

  // pass event handler
  socketIO.onEvent(socketIOEvent);

  // setup complete, report state
  reportState();
}

void loop() {
  socketIO.loop();
  if ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {
    stepperOne.run();
    stepperTwo.run();
  }
  else if (wasRunning) {
    busyState = false;
    wasRunning = false;
    depowerStepper();
    Serial.printf("done. New Position: %d\n", stepperOne.currentPosition());
    reportState();
  }
}
