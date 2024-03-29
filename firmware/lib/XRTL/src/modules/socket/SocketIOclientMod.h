#ifndef SOCKETIOCLIENTMOD_H
#define SOCKETIOCLIENTMOD_H

#include "SocketIOclient.h"
#include "WebSocketsClient.h" //Websockets by Markus Sattler

class SocketModule;

class SocketIOclientMod : public SocketIOclient {
private:
    SocketModule *parent;

public:
    bool sendBIN(String &binaryLeadFrame, uint8_t *payload, size_t length, bool headerToPayload = false);
    bool sendBIN(String &binaryLeadFrame, const uint8_t *payload, size_t length);
    bool disconnect();
};

#endif