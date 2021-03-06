bool busyState = false;

//modified socketIO client, added methode to send binary data
char binaryLeadFrame[100];// message has 68 characters, 32 left for componentId - example: do_not_exceed_this_maximum_okay?

class SocketIOclientMod : public SocketIOclient {
  public:
  bool sendBIN(uint8_t * payload, size_t length, bool headerToPayload = false);
  bool sendBIN(const uint8_t * payload, size_t length);
  bool disconnect();
};

bool SocketIOclientMod::sendBIN(uint8_t * payload, size_t length, bool headerToPayload) {
  bool ret = false;
  if (length == 0) {
    length = strlen((const char *) payload);
  }
  ret = sendFrame(&_client, WSop_text, (uint8_t *) binaryLeadFrame,
        strlen((const char*) binaryLeadFrame), true, headerToPayload);
  if (ret) {
    ret = sendFrame(&_client, WSop_binary, payload, length, true, headerToPayload);
  }
  return ret;
}

bool SocketIOclientMod::sendBIN(const uint8_t * payload, size_t length) {
  return sendBIN((uint8_t *)payload, length);
}

bool SocketIOclientMod::disconnect(){
  return send(sIOtype_DISCONNECT,"");
}

SocketIOclientMod socketIO;
