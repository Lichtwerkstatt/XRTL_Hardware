#include "WifiModule.h"

WifiModule::WifiModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  WiFi.reconnect();
}

void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("[wifi] connected to WiFi with local IP: ");
  Serial.println(WiFi.localIP());
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
    debug("");
    debug(centerString("",39, '-').c_str());
    debug(centerString(id,39, ' ').c_str());
    debug(centerString("",39, '-').c_str());
    debug("");

  JsonObject loaded = settings[id];
  enterprise = loaded["enterprise"].as<bool>();
  ssid = loaded["ssid"].as<String>();
  password = loaded["password"].as<String>();

  if (enterprise) {
    debug(centerString("enterprise WiFi",39,' ').c_str());
  }
  else {  
    debug(centerString("regular WiFi",39,' ').c_str());
  }

  debug("SSID: %s",ssid.c_str());
  debug("password: %s", password.c_str());
  
  if (!enterprise) {
    return;
  }

  username = loaded["username"].as<String>();
  anonymous = loaded["anonymous"].as<String>();

  debug("username: %s", username.c_str());
  debug("anonymous: %s", anonymous.c_str());
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
  eventIdGotIP = WiFi.onEvent(WiFiStationGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);//SYSTEM_EVENT_STA_GOT_IP for older version
  //WiFi.setHostname(xrtl=>get hostname from socket module);
  WiFi.mode(WIFI_STA);

  if (enterprise) {
    if ( (username == 0) or (strcmp(username.c_str(),"null") == 0) ){
      debug("[%s] WARNING! username not set -- falling back to regular WiFi", id.c_str());

      enterprise = false;
    }
    else if ( (anonymous == 0) or (strcmp(anonymous.c_str(),"null") == 0) ) {
      debug("[%s] WARNING! anonymous identiy not set -- falling back to regular WiFi", id.c_str());

      enterprise = false;
    }
    else {
      debug("[%s] starting enterprise level WiFi", id.c_str());

      WiFi.begin(ssid.c_str(), WPA2_AUTH_PEAP, anonymous.c_str(), username.c_str(), password.c_str());
    }
  }

  if (!enterprise) {
    if ( (ssid == 0) or (strcmp(ssid.c_str(),"null") == 0) ) {
      debug("[%s] WARNING! SSID not set -- unable to start WiFi", id.c_str());
    }
    else {
      debug("[%s] starting regular WiFi", id.c_str());

      WiFi.begin(ssid.c_str(), password.c_str());
    }
  }
}

void WifiModule::loop() {
  if (checkConnection) {
    if (WiFi.isConnected()) {
      notify(wifi_connected);
      checkConnection = false; // only stop checking once connection is made
    }
    else {
      notify(wifi_disconnected);
    }
  }
}

void WifiModule::stop() {
  WiFi.removeEvent(eventIdGotIP); // get rid of autoamtic reconnect to avoid error messages
  WiFi.removeEvent(eventIdDisconnected);
  WiFi.disconnect(true); // disconnecting from WiFi should prevent unrelated output on serial
  WiFi.mode(WIFI_OFF);
}

void WifiModule::handleInternal(internalEvent event) {
  switch(event) {
    case socket_disconnected: {
      checkConnection = true; // check if wifi is down too
    }

    case debug_on: {
      debugging = true;
    }
    case debug_off: {
      debugging = false;
    }
  }
}

template<typename... Args>
void WifiModule::debug(Args... args) {
  if(!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};