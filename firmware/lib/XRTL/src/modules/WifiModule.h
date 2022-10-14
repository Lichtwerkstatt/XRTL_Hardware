#ifndef WIFIMODULE_H
#define WIFIMODULE_H

#include "WiFi.h"
#include "esp_wpa2.h"
#include "XRTLmodule.h"

class WifiModule : public XRTLmodule {
  private:
  bool checkConnection = true;

  bool enterprise;
  String ssid;
  String password;
  String username;
  String anonymous;

  WiFiEventId_t eventIdDisconnected; // use this when attaching the disconnected handler; needed for deinit

  public:
  WifiModule(String moduleName, XRTL* source);
  moduleType getType();

  void saveSettings(JsonObject& settings);
  void loadSettings(JsonObject& settings);
  void setViaSerial();

  void setup();
  void loop();
  void stop();

  void handleInternal(internalEvent eventId, String& sourceId);
};

// callback used for automatically reconnecting
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

#endif