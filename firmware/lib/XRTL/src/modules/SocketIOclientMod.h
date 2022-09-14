#ifndef SOCKETIOCLIENTMOD_H
#define SOCKETIOCLIENTMOD_H

#include "WebSocketsClient.h"//Websockets by Markus Sattler
#include "SocketIOclient.h"

class SocketModule;

class SocketIOclientMod : public SocketIOclient {
  private:
  char binaryLeadFrame[128];
  SocketModule* owner;
  
  public:
  bool sendBIN(uint8_t * payload, size_t length, bool headerToPayload = false);
  bool sendBIN(const uint8_t * payload, size_t length);
  bool disconnect();
};

#endif