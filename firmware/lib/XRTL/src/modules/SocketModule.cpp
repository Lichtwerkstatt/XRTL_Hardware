#include "SocketModule.h"
// BASE 64 URL encoding table
static const char base64url_en[] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
  'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
  'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
  'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
  'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
  'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
  'w', 'x', 'y', 'z', '0', '1', '2', '3',
  '4', '5', '6', '7', '8', '9', '-', '_',
};

bool base64url_encodeCharArray(const unsigned char *in, unsigned int inlen, char *out) {
  unsigned int i, j;

  for (i = j = 0; i < inlen; i++) {
    int s = i % 3;      // from 8 to 6 bit

    switch (s) {
    case 0:
      out[j++] = base64url_en[(in[i] >> 2) & 0x3F];
      continue;
    case 1:
      out[j++] = base64url_en[((in[i-1] & 0x3) << 4) + ((in[i] >> 4) & 0xF)];
      continue;
    case 2:
      out[j++] = base64url_en[((in[i-1] & 0xF) << 2) + ((in[i] >> 6) & 0x3)];
      out[j++] = base64url_en[in[i] & 0x3F];
    }
  }
  
  // check last character
  i -= 1;
  
  if ((i % 3) == 0) {
    out[j++] = base64url_en[(in[i] & 0x3) << 4];
  } else if ((i % 3) == 1) {
    out[j++] = base64url_en[(in[i] & 0xF) << 2];
  }

  // end string
  out[j++] = 0;

  return true;
}

String base64url_encode( const uint8_t * data, size_t dataLength) {
  size_t outputLength = ((((4 * dataLength) / 3) +3) & ~3);
  char * buffer = (char *) malloc(outputLength + 1);
  if (buffer) {
    base64url_encodeCharArray((const unsigned char*) &data[0], dataLength, &buffer[0]);
    String output = String(buffer);
    free(buffer);
    return output;
  }
  return String("-FAIL-");
}

String base64url_encode(String& text) {
  return base64url_encode( (uint8_t *) text.c_str(), text.length());
}

String SocketModule::createJWT() {
  DynamicJsonDocument document(512);
  
  JsonObject header = document.to<JsonObject>();
  header["alg"] = "HS256";
  header["typ"] = "JWT";

  String encoding;
  serializeJson(header,encoding);
  String headerBase64 = base64url_encode(encoding);

  JsonObject payload = document.to<JsonObject>();
  time_t now;
  time(&now);
  payload["sub"] = component;
  payload["component"] = "component";
  payload["iat"] = now;
  payload["exp"] = now + 605000;// ~ 1 week

  encoding = "";
  serializeJson(payload,encoding);
  String payloadBase64 = base64url_encode(encoding);

  String token = headerBase64;
  token += '.';
  token += payloadBase64;

  // cryptographic engine
  byte hmacResult[32];
  mbedtls_md_context_t mbedtlsContext;
  mbedtls_md_type_t mbedtls_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&mbedtlsContext);
  mbedtls_md_setup(&mbedtlsContext, mbedtls_md_info_from_type(mbedtls_type),1);
  mbedtls_md_hmac_starts(&mbedtlsContext, (const unsigned char *) key.c_str(), key.length());
  mbedtls_md_hmac_update(&mbedtlsContext, (const unsigned char *) token.c_str(), token.length());
  mbedtls_md_hmac_finish(&mbedtlsContext, hmacResult);
  mbedtls_md_free(&mbedtlsContext);

  String signature = base64url_encode(hmacResult,32);
  token += '.';
  token += signature;

  return token;
}





SocketModule* SocketModule::lastModule = NULL;

SocketModule::SocketModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
  lastModule = this;
}

void SocketModule::sendEvent(JsonArray& event){
  if (!socket->isConnected()) {
    if (!debugging) return;
    String output;
    serializeJson(event,output);
    debug("[%s] disconnected, unable to sent event: %s", id.c_str(), output.c_str());
    return;
  }

  String output;
  serializeJson(event,output);
  socket->sendEVENT(output);
  debug("[%s] sent event: %s", id.c_str(), output.c_str());
}

void SocketModule::sendError(componentError err, String msg){
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  
  payload.add("error");
  JsonObject parameters = payload.createNestedObject();
  parameters["errnr"] = err;
  parameters["errmsg"] = msg;

  sendEvent(payload);
}

void SocketModule::sendBinary(String* binaryLeadFrame, uint8_t* payload, size_t length) {
  socket->sendBIN(binaryLeadFrame, payload, length);
}

void socketHandler(socketIOmessageType_t type, uint8_t* payload, size_t length) {
  SocketModule* owner = SocketModule::lastModule;
  switch(type) {
    case sIOtype_DISCONNECT: {
        owner->notify(socket_disconnected);
        return;
      }
    case sIOtype_CONNECT: {
        owner->debug("[%s] connected to <%s>", owner->id.c_str(),payload);
        owner->failedConnectionCount = 0;
        owner->notify(socket_connected);
   
        String token = "{\"token\":\"";
        token += owner->createJWT();
        token += "\"}";
        owner->debug("[%s] sending token: %s", owner->id.c_str(), token.c_str());
        owner->socket->send(sIOtype_CONNECT, token);
        owner->debug("[%s] waiting for authentication", owner->id.c_str());
        return;
      }
    case sIOtype_EVENT: {
      owner->debug("[%s] got event: %s", owner->getID().c_str(), payload);
      DynamicJsonDocument incommingEvent(1024);
      DeserializationError error = deserializeJson(incommingEvent,payload,length);
      if (error) {
        //owner->sendError();
        String errormsg = "deserialization failed: ";
        errormsg += error.c_str();
        owner->sendError(deserialize_failed, errormsg);
        owner->debug("[%s] deserializeJson() failed to analyze event: <%s>", owner->getID().c_str(), error.c_str());
      }

      owner->handleEvent(incommingEvent);
      return;
    }
    case sIOtype_ACK: {}
    case sIOtype_ERROR: {}
    case sIOtype_BINARY_EVENT: {}
    case sIOtype_BINARY_ACK: {}
  }
}

void SocketModule::handleEvent(DynamicJsonDocument& doc) {
  if (!doc[0].is<String>()) {
    sendError(wrong_type, "event rejected: <event name> was expected to be a string");
    return;
  }

  String eventName = doc[0].as<String>();
  if (strcmp(eventName.c_str(), "Auth") == 0){
    debug("[%s] authenticated by server",id.c_str());
    notify(socket_authed);
    return;
  }

  if ( !(strcmp(eventName.c_str(), "command") == 0) ) return;

  //command:
  auto componentField = doc[1]["componentId"];
  if (!componentField.is<String>()) {
    sendError(wrong_type, "command rejected: <componentId> is no string");
    return;
  }

  String componentName = componentField.as<String>();
  if (strcmp(componentName.c_str(),"null") == 0) {
    String error = "[";
    error += id;
    error += "] componentId is null";
    sendError(field_is_null, error);
    return;
  }

  auto commandField = doc[1]["command"];
  if (!commandField.is<String>() and !commandField.is<JsonObject>() ){
    sendError(wrong_type, "command rejected: <command> is neither a string nor a nested Object");
    return;
  }

  if ( (strcmp(componentName.c_str(),component.c_str()) == 0)
    or (strcmp(componentName.c_str(),alias.c_str()) == 0)
    or (strcmp(componentName.c_str(),"*") == 0) ) {

    if (commandField.is<String>()) {
      debug("[%s] pushing simple command to modules", id.c_str());
      String command = commandField.as<String>();
      pushCommand(command);
    }
    else {
      JsonObject command = commandField.as<JsonObject>();
      debug("[%s] pushing complex command to modules", id.c_str());
      pushCommand(command);
    }
  }

  return;//not me; don't care
}

void SocketModule::saveSettings(DynamicJsonDocument& settings) {
  JsonObject saving = settings.createNestedObject(id);
  
  saving["ip"] = ip;
  saving["port"] = port;
  saving["url"] = url;
  saving["key"] = key;
  saving["component"] = component;
  saving["alias"] = alias;
  
  return;
}

void SocketModule::loadSettings(DynamicJsonDocument& settings) {
  JsonObject loaded = settings[id];

  ip = loadValue<String>("ip", loaded, "192.168.1.1");
  port = loadValue<uint16_t>("port", loaded, 3000);
  url = loadValue<String>("url", loaded, "/socket.io/?EIO=4");
  key = loadValue<String>("key", loaded, "");
  component = loadValue<String>("component", loaded, "NAME ME!");
  alias = loadValue<String>("alias", loaded, "");

  if (!debugging) return;
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  Serial.printf("ip: %s\n", ip.c_str());
  Serial.printf("port: %i\n", port);
  Serial.printf("url: %s\n", url.c_str());
  Serial.printf("key: %s\n", key.c_str());
  Serial.printf("componentId: %s\n", component.c_str());
  Serial.printf("alias: %s\n", alias.c_str());
}

void SocketModule::setViaSerial() {
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  ip = serialInput("IP: ");
  port = serialInput("port: ").toInt();
  url = serialInput("URL: ");
  key = serialInput("key: ");
  component = serialInput("componentId: ");
  alias = serialInput("alias: ");
}

void SocketModule::getStatus(JsonObject& payload, JsonObject& status) {
  payload["componentId"] = component;
}

void SocketModule::setup(){
  configTime(0,0,"pool.ntp.org");
}

void SocketModule::loop(){
  if (!timeSynced) {
    time_t now;
    time(&now);
    //check if time is 2022 or later
    if (now > 1640991600){
      //time probably synced, stop checking and push state to modules
      timeSynced = true;
      notify(time_synced);
      return;
    }
    if (esp_timer_get_time() > 300000000) {// 5 minutes after start -> WiFi not working?
      debug("[%s] unable to sync time, restating device\n", id.c_str());
      ESP.restart(); 
    }
  }

  socket->loop();
}

void SocketModule::stop() {
  socket->disconnect();
}

void SocketModule::handleInternal(internalEvent event) {
  switch(event) {
    case socket_disconnected: {
      failedConnectionCount++;
        debug("[%s] disconnected, connection attempt: %i", id.c_str(), failedConnectionCount);
      if (failedConnectionCount > 49) {
        debug("[%s] unable to connect -- restarting device", id.c_str());
        ESP.restart();
      }
      return;
    }
    case socket_connected: {
      failedConnectionCount = 0;

      debug("[%s] succesfully connected to server", id.c_str());
      return;
    }
    case socket_authed: {
      int64_t waitTime = esp_timer_get_time() + random(100000,300000);
      while ( esp_timer_get_time() < waitTime) {
        yield();
      }
      sendStatus();

      return;
    }

    case busy: {
      isBusy = true;
      sendStatus();
      return;
    }
    case ready: {
      isBusy = false;
      sendStatus();
      return;
    }
    
    case time_synced: {
      socket->begin(ip.c_str(), port, url.c_str());
      socket->onEvent(socketHandler);
      if (!debugging) return;
      time_t now;
      time(&now);
      Serial.printf("[%s] time synced: %d\n", id.c_str(), now);
      Serial.printf("[%s] socket client started\n", id.c_str());
      return;
    }

    case debug_off: {
      debugging = false;
      return;
    }
    case debug_on: {
      debugging = true;
      return;
    }
  }
}