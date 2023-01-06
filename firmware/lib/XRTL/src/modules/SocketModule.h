#ifndef SOCKETMODULE_H
#define SOCKETMODULE_H

#include "SocketIOclientMod.h"
#include "mbedtls/md.h"
#include "XRTLmodule.h"

class SocketModule : public XRTLmodule {
  private:
  String ip;
  uint16_t port;
  String url;
  String key;
  String component;
  bool useSSL;

  uint8_t failedConnectionCount = 0;
  bool timeSynced = false;
  bool isBusy = false;
  SocketIOclientMod* socket = new SocketIOclientMod;
  static SocketModule* lastModule;
 

  public:
  SocketModule(String moduleName, XRTL* source);
  moduleType getType();

  String& getComponent();

  friend void socketHandler(socketIOmessageType_t type, uint8_t* payload, size_t length);
  void handleEvent(DynamicJsonDocument& doc);
  
  void pushCommand(String& command);
  void pushCommand(String& controlId, JsonObject& command);

  String createJWT();
  void sendConnect();
  
  void sendEvent(JsonArray& event);
  void sendError(componentError err, String msg);
  void sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length);

  void saveSettings(JsonObject& settings);
  void loadSettings(JsonObject& settings);
  void setViaSerial();

  void setup();
  void loop();
  void stop();

  void handleInternal(internalEvent eventId, String& sourceId);
  void sendAllStatus();
};

void socketHandler(socketIOmessageType_t type, uint8_t* payload, size_t length);

#endif