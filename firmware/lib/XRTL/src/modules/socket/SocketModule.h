#ifndef SOCKETMODULE_H
#define SOCKETMODULE_H

#include "SocketIOclientMod.h"
#include "esp_sntp.h"
#include "mbedtls/md.h"
#include "modules/XRTLmodule.h"

class SocketModule : public XRTLmodule
{
private:
    String ip = "192.168.178.1";
    uint16_t port = 3000;
    String url = "/socket.io/?EIO=4";
    String key = "key";
    String component = "XRTL_ESP32";
    bool useSSL = false;
    bool timeSynced = false;

    uint8_t failedConnectionCount = 0;
    bool isBusy = false;
    SocketIOclientMod *socket = new SocketIOclientMod;
    static SocketModule *lastModule;

public:
    SocketModule(String moduleName);
    moduleType type = xrtl_socket;
    moduleType getType();

    String &getComponent();

    friend void timeSyncCallback(struct timeval *tv);
    friend void socketHandler(socketIOmessageType_t type, uint8_t *payload, size_t length);
    void handleEvent(DynamicJsonDocument &doc);

    void pushCommand(String &controlId, JsonObject &command);
    void pushStatus(String &controlId, JsonObject &status);

    String createJWT();
    void sendConnect();

    void sendEvent(JsonArray &event);
    void sendError(componentError err, String msg);
    void sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length);

    void saveSettings(JsonObject &settings);
    void loadSettings(JsonObject &settings);
    void setViaSerial();

    void setup();
    void loop();
    void stop();

    void handleInternal(internalEvent eventId, String &sourceId);
    void sendAllStatus();
};

void socketHandler(socketIOmessageType_t type, uint8_t *payload, size_t length);

void timeSyncCallback(struct timeval *tv);

#endif