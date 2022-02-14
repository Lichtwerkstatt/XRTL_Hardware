#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_camera.h>
#include <WebSocketsClient.h>
#include <SocketIOclient.h>
#include <WiFi.h>
#include <LITTLEFS.h>
#define FILESYSTEM LITTLEFS
/*#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"*/

// switch for stream start
bool streamRunning = false;
String binaryLeadFrame;

//modified socketIO client, added methode to send binary data
class SocketIOclientMod : public SocketIOclient {
  public:
  bool sendBIN(uint8_t * payload, size_t length, bool headerToPayload = false);
  bool sendBIN(const uint8_t * payload, size_t length);
};

//method for sending binaries via socketIO
bool SocketIOclientMod::sendBIN(uint8_t * payload, size_t length, bool headerToPayload) {
  bool ret = false;
  if (length == 0) {
    length = strlen((const char *) payload);
  }
  ret = sendFrame(&_client, WSop_text, (uint8_t *) binaryLeadFrame.c_str(), strlen((const char*) binaryLeadFrame.c_str()), true, headerToPayload);
  if (ret) {
    ret = sendFrame(&_client, WSop_binary, payload, length, true, headerToPayload);
  }
  return ret;
}

bool SocketIOclientMod::sendBIN(const uint8_t * payload, size_t length) {
    return sendBIN((uint8_t *)payload, length);
}

SocketIOclientMod socketIO;

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
}

void saveSettings() {
  DynamicJsonDocument doc(1024);

  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["socketIP"] = settings.socketIP;
  doc["socketPort"] = settings.socketPort;
  doc["socketURL"] = settings.socketURL;
  doc["componentID"] = settings.componentID;
  
  File file = FILESYSTEM.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("Failed to write file");
  }
  file.close();
}

void startStreaming() {
  binaryLeadFrame = "451-[\"pic\",{\"component\":\"";
  binaryLeadFrame += settings.componentID;
  binaryLeadFrame += "\",\"image\":{\"_placeholder\":true,\"num\":0}}]";
  streamRunning = true;
}

void stopStreaming() {
  streamRunning = false;
}

void camLoop() {
  if (streamRunning) {
    camera_fb_t *fb = esp_camera_fb_get();
    socketIO.sendBIN(fb->buf,fb->len);
    esp_camera_fb_return(fb);
    Serial.printf("frame send!\n");
  }
}

// ESP32Cam (AiThinker) PIN Map

#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1 //software reset will be performed
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

static camera_config_t camera_config = {
    .pin_pwdn  = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sscb_sda = CAM_PIN_SIOD,
    .pin_sscb_scl = CAM_PIN_SIOC,

    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,

    .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_SVGA,//UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 12,//10, //0-63 lower number means higher quality
    .fb_count = 2, //if more than one, i2s runs in continuous mode. Use only with JPEG
    //.grab_mode = CAMERA_GRAB_WHEN_EMPTY//CAMERA_GRAB_LATEST. Sets when buffers should be filled
};

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
          }
          else if (receivedPayload["command"].is<String>()) {
            String command = receivedPayload["command"];
            if (command == "getStatus") {
              reportState();
            }
            if (command == "restart") {
              Serial.println("Received restart command. Disconnecting now.");
              delay(500);
              saveSettings();
              ESP.restart();
            }
            if (command == "startStreaming") {
              startStreaming();
              reportState();
            }
            if (command == "stopStreaming") {
              stopStreaming();
              reportState();
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
  state["streaming"] = streamRunning;
  //state[""] = sideServo.read();
  //state[""] = heightServo.read();
  String output;
  serializeJson(payload, output);
  socketIO.sendEVENT(output);
  Serial.print("string sent: ");
  Serial.println(output);
}

static esp_err_t init_camera()
{
    //initialize the camera
    esp_err_t err = esp_camera_init(&camera_config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera Init Failed: %s\n", err);
        return err;
    }

    return ESP_OK;
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[System] lost connection. Trying to reconnect.\n");
  WiFi.reconnect();
}

void WiFiStationConnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[System] connected to WiFi\n");
}

void setup() {
  Serial.begin(115200);
  for(uint8_t t = 4; t > 0; t--) {
    Serial.printf("[SETUP] BOOT WAIT %d...\n", t);
    Serial.flush();
    delay(1000);
  }
  
  loadSettings();
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.onEvent(WiFiStationConnected, SYSTEM_EVENT_STA_CONNECTED);
  
  socketIO.begin(settings.socketIP, settings.socketPort, settings.socketURL);
  socketIO.onEvent(socketIOEvent);
  
  init_camera();

  while (!((socketIO.isConnected()) and (WiFi.status() == WL_CONNECTED))) {
    Serial.printf("waiting for connection\n");
    delay(500);
    socketIO.loop();
  }
}

void loop() {
  if (Serial.available() > 0) {
    String SerialInput=Serial.readStringUntil('\n');
    if (SerialInput.indexOf("start") != -1) {
      //SerialInput = SerialInput.substring(4);
      //driveStepper("top", SerialInput.toInt());
      startStreaming();
    }
    else if (SerialInput.indexOf("stop") != -1) {
      //SerialInput = SerialInput.substring(7);
      //driveStepper("bottom", SerialInput.toInt());
      stopStreaming();
    }
    else if (SerialInput.indexOf("side") != -1) {
      SerialInput = SerialInput.substring(5);
      //sideServo.write(SerialInput);
    }
    else if (SerialInput.indexOf("height") != -1) {
      SerialInput = SerialInput.substring(7);
      //heightServo.write(SerialInput);
    }
    else if (SerialInput.indexOf("report") != -1) {
      reportState();
    }
  }
  camLoop();
  socketIO.loop();
}
