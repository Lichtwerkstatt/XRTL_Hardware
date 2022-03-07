#include <Arduino.h>
#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include "infoLED.h"
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
  long stepperOnePos;
  long stepperOneMin;
  long stepperOneMax;
} settings;

// identify component
bool busyState = false;
bool wasRunning = false;

// definition for stepper motors
const int stepperOneA = 16;
const int stepperOneB = 18;
const int stepperOneC = 17;
const int stepperOneD = 19;
AccelStepper stepperOne(AccelStepper::HALF4WIRE, stepperOneA, stepperOneB, stepperOneC, stepperOneD);

void loadSettings() {
  if(!FILESYSTEM.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }
  File file = FILESYSTEM.open("/settings.txt","r");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.printf("deserializeJson() failed: ");
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
  settings.stepperOnePos = doc["stepperOnePos"];
  Serial.printf("stepper one position: %i\n", settings.stepperOnePos);
  settings.stepperOneMin = doc["stepperOneMin"];
  Serial.printf("stepper one minimum: %i\n", settings.stepperOneMin);
  settings.stepperOneMax = doc["stepperOneMax"];
  Serial.printf("stepper one maximum: %i\n", settings.stepperOneMax);
  Serial.println("=======================");

  stepperOne.setCurrentPosition(settings.stepperOnePos);
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
  doc["stepperOnePos"] = stepperOne.currentPosition();
  doc["stepperOneMin"] = settings.stepperOneMin;
  doc["stepperOneMax"] = settings.stepperOneMax;

  if(!FILESYSTEM.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  File file = FILESYSTEM.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("Failed to write to file");
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
      else if (WiFi.status() == WL_CONNECTED){
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
  DeserializationError error = deserializeJson(incomingEvent,eventPayload,eventLength);
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
      String control = "iris";
      if (receivedPayload["command"].is<JsonObject>()) {
        JsonObject command = receivedPayload["command"];
        Serial.println("identified nested command structure");
        Serial.printf("identified controlID: %s\n", control);
        int targetPos = command["position"];
        Serial.printf("identified target position: %i\n", targetPos);
        driveStepper(control,targetPos);
      }
      else if (receivedPayload["command"].is<String>()) {
        String command = receivedPayload["command"];
        if (strcmp(command.c_str(),"getStatus") == 0) {
          reportState();
        }
        if (strcmp(command.c_str(),"stop") == 0) {
          stepperOne.stop();
          while (stepperOne.isRunning()) {
            stepperOne.run();
          }
          depowerStepper();
          Serial.println("Received stop command. Issuing stop at maximum decceleration.");
        }
        if (strcmp(command.c_str(),"restart") == 0) {
          Serial.println("Received restart command. Disconnecting now.");
          stepperOne.stop();
          while (stepperOne.isRunning()) {
            stepperOne.run();
          }
          depowerStepper();
          saveSettings();
          ESP.restart();
        }
      }
    }
  }
}

#if defined(ESP32)
  void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[System] lost connection. Trying to reconnect.\n");
    WiFi.reconnect();
  }
#endif

void reportState() {
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  payload.add("status");
  JsonObject parameters = payload.createNestedObject();
  parameters["component"] = settings.componentID;
  JsonObject state = parameters.createNestedObject("status");
  state["busy"] = busyState;
  state[settings.stepperOneName] = map(stepperOne.currentPosition(), settings.stepperOneMin, settings.stepperOneMax, 0, 100);
  String output;
  serializeJson(payload, output);
  //socketIO.sendEVENT(output);
  Serial.print("string sent: ");
  Serial.println(output);
}

void driveStepper(String stepperName, int steps) {
  busyState = true;
  reportState();
  if (strcmp(stepperName.c_str(),settings.stepperOneName.c_str()) == 0) {
    Serial.printf("moving %s by %d steps... ", settings.stepperOneName, steps);
    stepperOne.enableOutputs();
    /*stepperOne.move(steps);
    stepperOne.moveTo(constrain(stepperOne.targetPosition(), settings.stepperOneMin, settings.stepperOneMax));*/
    stepperOne.moveTo(map(constrain(steps, 0, 100), 0, 100, settings.stepperOneMin, settings.stepperOneMax));
    wasRunning = true;
    infoLED.pulse(0, 40, 100);
  }
}

void depowerStepper() {
  stepperOne.disableOutputs();
}

void stepperLoop() {
  if (stepperOne.isRunning()) {
    stepperOne.run();
  }
  else if (wasRunning){
    depowerStepper();
    busyState = false;
    wasRunning = false;
    depowerStepper();
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
  
  stepperOne.setMaxSpeed(250);
  stepperOne.setAcceleration(250);

  // connect to WiFi and handle auto reconnect
  Serial.println("starting WiFi setup.");
  #if defined(ESP32)
    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  #endif
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());
  #if defined(ESP8266)
    WiFi.setAutoReconnect(true);
  #endif

  // connect to socketIO server: IP, port, URL
  socketIO.begin(settings.socketIP.c_str(), settings.socketPort, settings.socketURL.c_str());
  
  // pass event handler
  socketIO.onEvent(socketIOEvent);
  
  // initialize iris as fully open:
  stepperOne.move(-2048);
  infoLED.pulse(0, 40, 100);
  while (stepperOne.isRunning()) {
    stepperOne.run();
    infoLED.loop();
    yield();
  }
  depowerStepper();
  stepperOne.setCurrentPosition(0);
  
  // setup complete
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
