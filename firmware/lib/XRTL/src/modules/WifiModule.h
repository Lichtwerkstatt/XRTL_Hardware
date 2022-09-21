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
  WiFiEventId_t eventIdGotIP; // use this when attaching the gotIP handler

  public:
  WifiModule(String moduleName, XRTL* source);

  void saveSettings(DynamicJsonDocument& settings);
  void loadSettings(DynamicJsonDocument& settings);
  void setViaSerial();

  void setup();
  void loop();
  void stop();

  void handleInternal(internalEvent event);
};

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info);

void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info);

#endif