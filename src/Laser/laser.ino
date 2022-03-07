#include <Arduino.h>
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

#define LASERPIN 33

SocketIOclient socketIO;

bool laserState = false;

struct {
  String ssid;
  String password;
  String socketIP;
  int socketPort;
  String socketURL;
  String componentID;
} settings;

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
  Serial.println("=======================");
}

void saveSettings() {
  DynamicJsonDocument doc(1024);

  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["socketIP"] = settings.socketIP;
  doc["socketPort"] = settings.socketPort;
  doc["socketURL"] = settings.socketURL;
  doc["componentID"] = settings.componentID;

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
      digitalWrite(LASERPIN, LOW);
      break;
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to URL: %s\n", payload);
      socketIO.send(sIOtype_CONNECT, "/");
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
    JsonObject receivedPayload = incomingEvent[1];
    String component = receivedPayload["componentId"];
    Serial.println("identified command event");
    if (strcmp(component.c_str(),settings.componentID.c_str()) == 0) {
      Serial.printf("componentId identified (me): %s\n", component);
      if (receivedPayload["command"].is<JsonObject>()) {
        Serial.println("identified nested command structure");
        JsonObject command = receivedPayload["command"];
        bool targetState = command["laser"];
        Serial.print("identified target laser state: ");
        Serial.println(targetState);
        switchLaser(targetState);
      }
      else if (receivedPayload["command"].is<String>()) {
        String command = receivedPayload["command"];
        if (strcmp(command.c_str(),"getStatus") == 0) {
          reportState();
        }
        if (strcmp(command.c_str(),"restart") == 0) {
          Serial.println("received restart command. Disconnecting now.");
          digitalWrite(LASERPIN, LOW);
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
  state["laserState"] = laserState;
  String output;
  serializeJson(payload, output);
  //socketIO.sendEVENT(output);
  Serial.print("string sent: ");
  Serial.println(output);
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
    Serial.printf("[System] lost connection. Trying to reconnect.\n");
    WiFi.reconnect();
}

void switchLaser(bool state) {
  if (socketIO.isConnected()) {
    reportState();
    if (state != laserState) {
      if (laserState) {
        digitalWrite(LASERPIN, LOW);
        laserState = false;
      }
      else if (!laserState) {
        digitalWrite(LASERPIN, HIGH);
        laserState = true;
      }
      reportState();
    }
  }
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  pinMode(LASERPIN, OUTPUT);
  digitalWrite(LASERPIN, LOW);
  
  loadSettings();
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());

  socketIO.begin(settings.socketIP.c_str(), settings.socketPort, settings.socketURL.c_str());
  socketIO.onEvent(socketIOEvent);
}

void loop() {
  if (Serial.available() > 0) {
    String SerialInput=Serial.readStringUntil('\n');
    /*if (SerialInput == "true"){
      switchLaser(true);
    }
    else if (SerialInput == "false") {
      switchLaser(false);
    }*/
    eventHandler((uint8_t *)SerialInput.c_str(), SerialInput.length());
  }
  socketIO.loop();
}
