/*
 * using WebSockets with socketIO subclass by Markus Sattler:
 * https://github.com/Links2004/arduinoWebSockets
 * adapted from /examples/esp32/WebSocketClientSocketIOack/WebsocketClientSocketIOack.ino
 */

#include <Arduino.h>
#include <AccelStepper.h>
#include <ArduinoJson.h>
#include <WebSocketsClient.h>
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

const int stepperTwoA = 32;
const int stepperTwoB = 25;
const int stepperTwoC = 33;
const int stepperTwoD = 26;
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

  settings.ssid = doc["ssid"].as<String>();
  settings.password = doc["password"].as<String>();
  settings.socketIP = doc["socketIP"].as<String>();
  settings.socketPort = doc["socketPort"];
  settings.socketURL = doc["socketURL"].as<String>();
  settings.componentID = doc["componentID"].as<String>();
  settings.stepperOneName = doc["stepperOneName"].as<String>();
  settings.stepperTwoName = doc["stepperTwoName"].as<String>();
  settings.stepperOneSpeed = doc["stepperOneSpeed"];
  settings.stepperOneAcc = doc["stepperOneAcc"];
  settings.stepperTwoSpeed = doc["stepperTwoSpeed"];
  settings.stepperTwoAcc = doc["stepperTwoAcc"];
  settings.stepperOnePos = doc["stepperOnePos"];
  settings.stepperTwoPos = doc["stepperTwoPos"];

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
      break;
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to URL: %s\n", payload);

      // join default namespace
      socketIO.send(sIOtype_CONNECT, "/");
      reportState();
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
        if (component == settings.componentID) {
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
            if (command == "stop") {
              stepperOne.stop();
              stepperTwo.stop();
              Serial.println("Received stop command. Issuing stop at maximum decceleration.");
            }
            if (command == "restart") {
              Serial.println("Received restart command. Disconnecting now.");
              stepperOne.stop();
              stepperTwo.stop();
              delay(500);
              saveSettings();
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
  parameters["component"] = settings.componentID;
  JsonObject state = parameters.createNestedObject("status");
  state["busy"] = busyState;
  state[settings.stepperOneName] = stepperOne.currentPosition();
  state[settings.stepperTwoName] = stepperTwo.currentPosition();
  String output;
  serializeJson(payload, output);
  //socketIO.sendEVENT(output);
  Serial.print("string sent: ");
  Serial.println(output);
}

void driveStepper(String stepperName, int steps) {
  busyState = true;
  reportState();
  if (stepperName == settings.stepperOneName) {
    Serial.printf("moving %s by %d steps... ", settings.stepperOneName, steps);
    stepperOne.enableOutputs();
    stepperOne.move(steps);
    wasRunning = true;
  }
  if (stepperName == settings.stepperTwoName) {
    Serial.printf("moving %s by %d steps... ", settings.stepperTwoName, steps);
    stepperTwo.enableOutputs();
    stepperTwo.move(steps);
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

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
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

  // connect to WiFi
  Serial.println("starting WiFi setup.");
  #if defined(ESP32)
    WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  #endif
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());
  //Serial.println("Connecting to WiFi");
  //while (WiFi.status() != WL_CONNECTED) {
    //Serial.print(".");
  //  delay(500);
  //}
  //Serial.println(" done.");
  #if defined(ESP8266)
    WiFi.setAutoReconnect(true);
  #endif
  //String ip = WiFi.localIP().toString();
  //Serial.printf("[SETUP] Connected to WiFi as %s\n", ip.c_str());

  socketIO.begin(settings.socketIP, settings.socketPort, settings.socketURL);
  
  // pass event handler
  socketIO.onEvent(socketIOEvent);
  
  // setup complete
}

void loop() {
  socketIO.loop();
  if (Serial.available() > 0) {
    String SerialInput=Serial.readStringUntil('\n');
    if (SerialInput.indexOf("top") != -1) {
      SerialInput = SerialInput.substring(4);
      driveStepper("top", SerialInput.toInt());
    }
    else if (SerialInput.indexOf("bottom") != -1) {
      SerialInput = SerialInput.substring(7);
      driveStepper("bottom", SerialInput.toInt());
    }
  }
  if ((stepperOne.isRunning()) or (stepperTwo.isRunning())) {
    stepperOne.run();
    stepperTwo.run();
  }
  else if (wasRunning){
    depowerStepper();
    if ((socketIO.isConnected()) and (WiFi.status() == WL_CONNECTED)) {
      busyState = false;
      wasRunning = false;
      Serial.printf("done. New Position: %d\n", stepperOne.currentPosition());
      reportState();
    }
  }
}
