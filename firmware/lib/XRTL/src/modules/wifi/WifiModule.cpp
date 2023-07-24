#include "WifiModule.h"

WifiModule::WifiModule(String moduleName)
{
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(enterprise, "enterprise", "bool");
    parameters.add(ssid, "ssid", "String");
    parameters.add(password, "password", "String");
    parameters.addDependent(username, "username", "String", "enterprise", true);   // username only needed with enterprise WiFi
    parameters.addDependent(anonymous, "anonymous", "String", "enterprise", true); // anonymous identity only needed with enterprise WiFi
}

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info)
{
    WiFi.reconnect();
}

void WifiModule::saveSettings(JsonObject &settings)
{
    parameters.save(settings);
}

void WifiModule::loadSettings(JsonObject &settings)
{
    parameters.load(settings);
    if (debugging)
        parameters.print();
}

void WifiModule::setViaSerial()
{
    parameters.setViaSerial();
}

void WifiModule::setup()
{
    eventIdDisconnected = WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED); // SYSTEM_EVENT_STA_DISCONNECTED for older version
    WiFi.setHostname(getComponent().c_str());
    WiFi.mode(WIFI_STA);

    if (enterprise)
    {
        if ((username == 0) or (username == "null") or (username == ""))
        {
            debug("WARNING! username not set -- falling back to regular WiFi");

            enterprise = false;
        }
        else if ((anonymous == 0) or (anonymous == "null") or (anonymous == ""))
        {
            debug("WARNING! anonymous identiy not set -- falling back to regular WiFi");

            enterprise = false;
        }
        else
        {
            debug("starting enterprise level WiFi");

            WiFi.begin(ssid.c_str(), WPA2_AUTH_PEAP, anonymous.c_str(), username.c_str(), password.c_str());
        }
    }

    if (!enterprise)
    {
        if ((ssid == 0) or (ssid == "null") or (ssid == ""))
        {
            debug("WARNING! SSID not set -- unable to start WiFi");
        }
        else
        {
            debug("starting regular WiFi");

            WiFi.begin(ssid.c_str(), password.c_str());
        }
    }
}

void WifiModule::loop()
{
    if (!checkConnection)
    {
        return;
    }
    
    if (WiFi.isConnected())
    {
        uint8_t nullAddress[4] = {0, 0, 0, 0};
        if (WiFi.localIP() == nullAddress)
            return; // no IP yet, can't use online resources
        debug("connected to WiFi with local IP: %s", WiFi.localIP().toString().c_str());
        debug("signal strength: %d dBm", WiFi.RSSI());
        notify(wifi_connected);
        checkConnection = false; // only stop checking once connection is made
    }
    else
    {
        notify(wifi_disconnected);
    }

}

void WifiModule::stop()
{
    // get rid of automatic reconnect to avoid error messages in serial monitor, preparing for shutdown
    WiFi.removeEvent(eventIdDisconnected);
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

void WifiModule::handleInternal(internalEvent eventId, String &sourceId)
{
    switch (eventId)
    {
        case socket_disconnected:
        {
            checkConnection = true; // check if WiFi is down in next loop
            return;
        }

        case debug_off:
        {
            debugging = false;
            return;
        }
        case debug_on:
        {
            debugging = true;
            return;
        }
    }
}