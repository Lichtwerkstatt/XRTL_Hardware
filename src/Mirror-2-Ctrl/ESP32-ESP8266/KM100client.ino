/*
 * using WebSockets with socketIO subclass by Markus Sattler:
 * https://github.com/Links2004/arduinoWebSockets
 * adapted from /examples/esp32/WebSocketClientSocketIOack/WebsocketClientSocketIOack.ino
 */

#include <Arduino.h>
#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include "infoLED.h"
#include <SocketIOclient.h>

#if defined(ESP32)
  #include <WiFi.h>
  #include <LITTLEFS.h>
  #define FILESYSTEM LITTLEFS
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <LittleFS.h>
  #define FILESYSTEM LittleFS
#endif

SocketIOclient socketIO;
infoLED infoLED(12,25);

struct {
  String ssid;
  String password;
  String socketIP;
  int socketPort;
  String socketURL;
  String componentID;
  String stepperOneName;
  String stepperTwoName;
  int stepperOneSpeed;
  int stepperOneAcc;
  int stepperTwoSpeed;
  int stepperTwoAcc;
  long stepperOnePos;
  long stepperTwoPos;
  long stepperOneMin;
  long stepperTwoMin;
  long stepperOneMax;
  long stepperTwoMax;
} settings;

// states
bool busyState = false;
bool wasRunning = false;

// definition for stepper motors
const int stepperOneA = 16;
const int stepperOneB = 18;
const int stepperOneC = 17;
const int stepperOneD = 19;
AccelStepper stepperOne(AccelStepper::HALF4WIRE, stepperOneA, stepperOneB, stepperOneC, stepperOneD);

const int stepperTwoA = 27;
const int stepperTwoB = 33;
const int stepperTwoC = 26;
const int stepperTwoD = 32;
AccelStepper stepperTwo(AccelStepper::HALF4WIRE, stepperTwoA, stepperTwoB, stepperTwoC, stepperTwoD);

void loadSettings() {
  if(!FILESYSTEM.begin()){
    Serial.println("An Error has occurred while mounting LITTLEFS");
    return;
  }
  File file = FILESYSTEM.open("/settings.txt","r");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.printf("deserializeJson() while loading settings: ");
    Serial.println(error.c_str());
  }
  file.close();

  Serial.println("=======================");
  Serial.println("loading settings");
  Serial.println("=======================");
  settings.ssid = doc["ssid"].as<String>();
  Serial.printf("SSID: %s\n", settings.ssid.c_str());
  settings.password = doc["password"].as<String>();
  Serial.printf("password: %s\n", settings.password.c_str());
  settings.socketIP = doc["socketIP"].as<String>();
  Serial.printf("socket IP: %s\n", settings.socketIP.c_str());
  settings.socketPort = doc["socketPort"];
  Serial.printf("Port: %i\n", settings.socketPort);
  settings.socketURL = doc["socketURL"].as<String>();
  Serial.printf("URL: %s\n", settings.socketURL.c_str());
  settings.componentID = doc["componentID"].as<String>();
  Serial.printf("component ID: %s\n", settings.componentID.c_str());
  settings.stepperOneName = doc["stepperOneName"].as<String>();
  Serial.printf("stepper one name: %s\n", settings.stepperOneName.c_str());
  settings.stepperTwoName = doc["stepperTwoName"].as<String>();
  Serial.printf("stepper two name: %s\n", settings.stepperTwoName.c_str());
  settings.stepperOneSpeed = doc["stepperOneSpeed"];
  Serial.printf("stepper one speed: %i\n", settings.stepperOneSpeed);
  settings.stepperOneAcc = doc["stepperOneAcc"];
  Serial.printf("stepper one accelleration: %i\n", settings.stepperOneAcc);
  settings.stepperTwoSpeed = doc["stepperTwoSpeed"];
  Serial.printf("stepper two speed: %i\n", settings.stepperOneSpeed);
  settings.stepperTwoAcc = doc["stepperTwoAcc"];
  Serial.printf("stepper two accelleration: %i\n", settings.stepperTwoAcc);
  settings.stepperOnePos = doc["stepperOnePos"];
  Serial.printf("stepper one position: %i\n", settings.stepperOnePos);
  settings.stepperTwoPos = doc["stepperTwoPos"];
  Serial.printf("stepper two position: %i\n", settings.stepperTwoPos);
  settings.stepperOneMin = doc["stepperOneMin"];
  Serial.printf("stepper one minimum: %i\n", settings.stepperOneMin);
  settings.stepperTwoMin = doc["stepperTwoMin"];
  Serial.printf("stepper two minimum: %i\n", settings.stepperTwoMin);
  settings.stepperOneMax = doc["stepperOneMax"];
  Serial.printf("stepper one maximum: %i\n", settings.stepperOneMax);
  settings.stepperTwoMax = doc["stepperTwoMax"];
  Serial.printf("stepper two maximum: %i\n", settings.stepperTwoMax);
  Serial.println("=======================");

  stepperOne.setCurrentPosition(settings.stepperOnePos);
  stepperTwo.setCurrentPosition(settings.stepperTwoPos);
}

void saveSettings() {
  DynamicJsonDocument doc(1024);

  
  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["socketIP"] = settings.socketIP;
  doc["socketPort"] = settings.socketPort;
  doc["socketURL"] = settings.socketURL;
  doc["componentID"] = settings.componentID;
  doc["stepperOneName"] = settings.stepperOneName;
  doc["stepperTwoName"] = settings.stepperTwoName;
  doc["stepperOneSpeed"] = settings.stepperOneSpeed;
  doc["stepperOneAcc"] = settings.stepperOneAcc;
  doc["stepperTwoSpeed"] = settings.stepperTwoSpeed;
  doc["stepperTwoAcc"] = settings.stepperTwoAcc;
  doc["stepperOnePos"] = stepperOne.currentPosition();
  doc["stepperTwoPos"] = stepperTwo.currentPosition();
  doc["stepperOneMin"] = settings.stepperOneMin;
  doc["stepperTwoMin"] = settings.stepperTwoMin;
  doc["stepperOneMax"] = settings.stepperOneMax;
  doc["stepperTwoMax"] = settings.stepperTwoMax;

  if(!FILESYSTEM.begin()){
    Serial.println("An Error has occurred while mounting LITTLEFS");
    return;
  }

  File file = FILESYSTEM.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("Failed to write file");
  }
  file.close();
}

void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.printf("[IOc] Disconnected!\n");
      infoLED.hsv(0, 255, 40);
      if (WiFi.status() != WL_CONNECTED) {
        infoLED.cycle(100, false);
      }
      else if (WiFi.status() == WL_CONNECTED) {
        infoLED.pulse(0, 40, 100);
      }
      break;
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to URL: %s\n", payload);

      // join default namespace
      socketIO.send(sIOtype_CONNECT, "/");
      reportState();
      infoLED.hsv(20000, 255, 40);
      break;
    case sIOtype_EVENT:
      eventHandler(payload, length);
      break;
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

void eventHandler(uint8_t * eventPayload, size_t eventLength) {
  char * sptr = NULL;
  int id = strtol((char *)eventPayload, &sptr, 10);
  Serial.printf("[IOc] get event: %s     (id: %d)\n", eventPayload, id);
  DynamicJsonDocument incomingEvent(1024);
  DeserializationError error = deserializeJson(incomingEvent, eventPayload, eventLength);
  if(error) {
    Serial.printf("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }

  String eventName = incomingEvent[0];
  Serial.printf("[IOc] event name: %s\n", eventName.c_str());

  if (strcmp(eventName.c_str(),"command") == 0) {
    // analyze and store input
    JsonObject receivedPayload = incomingEvent[1];
    String component = receivedPayload["componentId"];
    Serial.println("identified command event");

    // act only when involving this component
    if (strcmp(component.c_str(),settings.componentID.c_str()) == 0) {
      // check for simple or extended command structure
      Serial.printf("componentId identified (me): %s\n", component);
      if (receivedPayload["command"].is<JsonObject>()) {
        JsonObject command = receivedPayload["command"];
        Serial.println("identified nested command structure");
        String control = receivedPayload["controlId"];
        Serial.printf("identified controlID: %s\n", control);
        int steps = command["steps"];
        Serial.printf("identified number of steps: %i\n", steps);
        driveStepper(control,steps);
        }
      else if (receivedPayload["command"].is<String>()) {
        String command = receivedPayload["command"];
        Serial.printf("identified simple command: %s\n", command);
        if (strcmp(command.c_str(),"getStatus") == 0) {
          reportState();
        }
        if (strcmp(command.c_str(),"stop") == 0) {
          stepperOne.stop();
          stepperTwo.stop();
          while ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {
            stepperOne.run();
            stepperTwo.run();
          }
          depowerStepper();
          Serial.println("Received stop command. Issuing stop at maximum decceleration.");
        }
        if (strcmp(command.c_str(),"restart") == 0) {
          Serial.println("Received restart command. Disconnecting now.");
          stepperOne.stop();
          stepperTwo.stop();
          while ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {
            stepperOne.run();
            stepperTwo.run();
          }
          depowerStepper();
          saveSettings();
          ESP.restart();
        }
      }
    }
  }
}

void reportState() {
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  payload.add("status");
  JsonObject parameters = payload.createNestedObject();
  parameters["component"] = settings.componentID;
  JsonObject state = parameters.createNestedObject("status");
  state["busy"] = busyState;
  state[settings.stepperOneName] = stepperOne.currentPosition();
  state[settings.stepperTwoName] = stepperTwo.currentPosition();
  String output;
  serializeJson(payload, output);
  socketIO.sendEVENT(output);
  Serial.print("string sent: ");
  Serial.println(output);
}

void driveStepper(String stepperName, int steps) {
  busyState = true;
  reportState();
  if (stepperName.indexOf(settings.stepperOneName) != -1) {
    Serial.printf("moving %s by %d steps... ", settings.stepperOneName, steps);
    stepperOne.enableOutputs();
    stepperOne.move(steps);
    stepperOne.moveTo(constrain(stepperOne.targetPosition(), settings.stepperOneMin, settings.stepperOneMax));
    wasRunning = true;
  }
  if (stepperName.indexOf(settings.stepperTwoName) != -1) {
    Serial.printf("moving %s by %d steps... ", settings.stepperTwoName, steps);
    stepperTwo.enableOutputs();
    stepperTwo.move(steps);
    stepperTwo.moveTo(constrain(stepperTwo.targetPosition(), settings.stepperTwoMin, settings.stepperTwoMax));
    wasRunning = true;
  }
}

void depowerStepper() {
  stepperOne.disableOutputs();
  stepperTwo.disableOutputs();
}

#if defined(ESP32)
  void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[System] lost connection. Trying to reconnect.\n");
    WiFi.reconnect();
  }
#endif

void stepperLoop() {
  if ((stepperOne.isRunning()) or (stepperTwo.isRunning())) {
    stepperOne.run();
    stepperTwo.run();
    infoLED.pulse(0, 40, 100);
  }
  else if (wasRunning){
    depowerStepper();
    busyState = false;
    wasRunning = false;
    Serial.printf("done. New Position: %d\n", stepperOne.currentPosition());
    reportState();
    infoLED.constant();
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  infoLED.begin();
  infoLED.hsv(0,255,40);
  infoLED.cycle(100, false);
  infoLED.start();
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  
  loadSettings();

  stepperOne.setMaxSpeed(settings.stepperOneSpeed);
  stepperOne.setAcceleration(settings.stepperOneAcc);
  stepperTwo.setMaxSpeed(settings.stepperTwoSpeed);
  stepperTwo.setAcceleration(settings.stepperTwoAcc);

  // connect to WiFi and handle auto reconnect
  Serial.println("starting WiFi setup.");
  #if defined(ESP32)
    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  #endif
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());
  #if defined(ESP8266)
    WiFi.setAutoReconnect(true);
  #endif

  // setup socketIO and pass event handler
  socketIO.begin(settings.socketIP.c_str(), settings.socketPort, settings.socketURL.c_str());
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  socketIO.loop();
  if (Serial.available() > 0) {
    String SerialInput=Serial.readStringUntil('\n');
    eventHandler((uint8_t *)SerialInput.c_str(), SerialInput.length());
  }
  stepperLoop();
  infoLED.loop();
}
