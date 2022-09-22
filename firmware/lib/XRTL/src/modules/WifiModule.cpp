#include "WifiModule.h"

WifiModule::WifiModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  WiFi.reconnect();
}


void WifiModule::saveSettings(DynamicJsonDocument& settings) {
  JsonObject saving = settings.createNestedObject(id);

  saving["enterprise"] = enterprise;
  saving["ssid"] = ssid;
  saving["password"] = password;

  if (enterprise) {
    saving["username"] = username;
    saving["anonymous"] = anonymous;
  }
}

void WifiModule::loadSettings(DynamicJsonDocument& settings) {
  JsonObject loaded = settings[id];

  enterprise = loadValue<bool>("enterprise", loaded, false);
  ssid = loadValue<String>("ssid", loaded, "");
  password = loadValue<String>("password", loaded, "");
  
  if (debugging) {
    Serial.printf(enterprise ? centerString("enterprise WiFi", 39, ' ').c_str() : centerString("regular WiFi", 39, ' ').c_str());
    Serial.println("");

    Serial.printf("SSID: %s\n",ssid.c_str());
    Serial.printf("password: %s\n", password.c_str());
  }
  
  if (!enterprise) return;

  username = loadValue<String>("username", loaded, "");
  anonymous = loadValue<String>("anonymous", loaded, "");

  if (!debugging) return;
  Serial.printf("username: %s\n", username.c_str());
  Serial.printf("anonymous: %s\n", anonymous.c_str());
}

void WifiModule::setViaSerial() {
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  enterprise = (strcmp(serialInput("enterprise WiFi (y/n): ").c_str(), "y") == 0);
  ssid = serialInput("SSID: ");
  password = serialInput("password: ");

  if (enterprise) {
    username = serialInput("username: ");
    anonymous = serialInput("anonymous identity: ");
  }
}

void WifiModule::setup() {
  eventIdDisconnected = WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);//SYSTEM_EVENT_STA_DISCONNECTED for older version
  //WiFi.setHostname(xrtl=>get hostname from socket module);
  WiFi.mode(WIFI_STA);

  if (enterprise) {
    if ( (username == 0) or (strcmp(username.c_str(),"null") == 0) ){
      debug("WARNING! username not set -- falling back to regular WiFi");

      enterprise = false;
    }
    else if ( (anonymous == 0) or (strcmp(anonymous.c_str(),"null") == 0) ) {
      debug("WARNING! anonymous identiy not set -- falling back to regular WiFi");

      enterprise = false;
    }
    else {
      debug("starting enterprise level WiFi");

      WiFi.begin(ssid.c_str(), WPA2_AUTH_PEAP, anonymous.c_str(), username.c_str(), password.c_str());
    }
  }

  if (!enterprise) {
    if ( (ssid == 0) or (strcmp(ssid.c_str(),"null") == 0) ) {
      debug("WARNING! SSID not set -- unable to start WiFi");
    }
    else {
      debug("starting regular WiFi");

      WiFi.begin(ssid.c_str(), password.c_str());
    }
  }
}

void WifiModule::loop() {
  if (checkConnection) {
    if (WiFi.isConnected()) {
      uint8_t nullAddress[4] = {0,0,0,0};
      if ( WiFi.localIP() == nullAddress ) return; // no IP yet, can't use online resources
      debug("connected to WiFi with local IP: %s", WiFi.localIP().toString().c_str());
      notify(wifi_connected);
      checkConnection = false; // only stop checking once connection is made
    }
    else {
      notify(wifi_disconnected);
    }
  }
}

void WifiModule::stop() {
  // get rid of automatic reconnect to avoid error messages in serial monitor, preparing for shutdown
  WiFi.removeEvent(eventIdDisconnected);
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void WifiModule::handleInternal(internalEvent event) {
  switch(event) {
    case socket_disconnected: {
      checkConnection = true; // check if WiFi is down in next loop
    }

    case debug_on: {
      debugging = true;
    }
    case debug_off: {
      debugging = false;
    }
  }
}