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
  String alias;

  uint8_t failedConnectionCount = 0;
  bool timeSynced = false;
  bool isBusy = false;
  SocketIOclientMod* socket = new SocketIOclientMod;
  static SocketModule* lastModule;
 

  public:
  SocketModule(String moduleName, XRTL* source);

  friend void socketHandler(socketIOmessageType_t type, uint8_t* payload, size_t length);
  void handleEvent(DynamicJsonDocument& doc);
  
  void pushCommand(String& command);
  void pushCommand(JsonObject& command);

  String createJWT();
  void sendConnect();
  
  void sendEvent(JsonArray& event);
  void sendError(componentError err, String msg);
  void sendBinary(String* binaryLeadFrame, uint8_t* payload, size_t length);

  void saveSettings(DynamicJsonDocument& settings);
  void loadSettings(DynamicJsonDocument& settings);
  void setViaSerial();
  void getStatus(JsonObject& payload, JsonObject& status);

  void setup();
  void loop();
  void stop();

  void handleInternal(internalEvent event);
  
  template<typename... Args>
  void debug(Args... args);
};

void socketHandler(socketIOmessageType_t type, uint8_t* payload, size_t length);

#endif