#include "SocketModule.h"
// BASE 64 URL encoding table
static const char base64url_en[] = {
    'A',
    'B',
    'C',
    'D',
    'E',
    'F',
    'G',
    'H',
    'I',
    'J',
    'K',
    'L',
    'M',
    'N',
    'O',
    'P',
    'Q',
    'R',
    'S',
    'T',
    'U',
    'V',
    'W',
    'X',
    'Y',
    'Z',
    'a',
    'b',
    'c',
    'd',
    'e',
    'f',
    'g',
    'h',
    'i',
    'j',
    'k',
    'l',
    'm',
    'n',
    'o',
    'p',
    'q',
    'r',
    's',
    't',
    'u',
    'v',
    'w',
    'x',
    'y',
    'z',
    '0',
    '1',
    '2',
    '3',
    '4',
    '5',
    '6',
    '7',
    '8',
    '9',
    '-',
    '_',
};

/**
 * @brief encode a char array using base64 with URL compatability
 * @param in pointer to the array to encode
 * @param inlen length of the array to encode
 * @param out pointer to the array in which the output is supposed to be stored
 * @returns true if the converstion was successful
 */
bool base64url_encodeCharArray(const unsigned char *in, unsigned int inlen, char *out) {
    unsigned int i, j;

    for (i = j = 0; i < inlen; i++) {
        int s = i % 3; // from 8 to 6 bit

        switch (s) {
        case 0:
            out[j++] = base64url_en[(in[i] >> 2) & 0x3F];
            continue;
        case 1:
            out[j++] = base64url_en[((in[i - 1] & 0x3) << 4) + ((in[i] >> 4) & 0xF)];
            continue;
        case 2:
            out[j++] = base64url_en[((in[i - 1] & 0xF) << 2) + ((in[i] >> 6) & 0x3)];
            out[j++] = base64url_en[in[i] & 0x3F];
        }
    }

    // check last character
    i--;

    if ((i % 3) == 0) {
        out[j++] = base64url_en[(in[i] & 0x3) << 4];
    } else if ((i % 3) == 1) {
        out[j++] = base64url_en[(in[i] & 0xF) << 2];
    }

    // end string
    out[j++] = 0;

    return true;
}

/**
 * @brief encode a char array using base64 with URL compatability
 * @param data pointer to the array to encode
 * @param dataLength length of the array to encode
 * @returns encoded data array as String
 */
String base64url_encode(const uint8_t *data, size_t dataLength) {
    size_t outputLength = ((((4 * dataLength) / 3) + 3) & ~3);
    char *buffer = (char *)malloc(outputLength + 1);
    if (buffer) {
        base64url_encodeCharArray((const unsigned char *)&data[0], dataLength, &buffer[0]);
        String output = String(buffer);
        free(buffer);
        return output;
    }
    return String("-FAIL-");
}

/**
 * @brief encode a char array using base64 with URL compatability
 * @param text String to encode
 * @returns encoded data array as String
 */
String base64url_encode(const String &text) {
    return base64url_encode((uint8_t *)text.c_str(), text.length());
}

/**
 * @brief create a JSON Web Token (JWT) based on the information stored in the module
 * @returns JWT as String
 */
String SocketModule::createJWT() {
    DynamicJsonDocument document(512);

    JsonObject header = document.to<JsonObject>();
    header["kid"] = "component"; // key ID, use to identify key used
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    String encoding;
    serializeJson(header, encoding);
    String headerBase64 = base64url_encode(encoding);

    JsonObject payload = document.to<JsonObject>();

    // do not remove: seems to be important for normal functioning of the socket connection
    // though the time is never used, it improves connection time significantly
    // maybe some undocumented problem in the network stack?
    time_t now;
    time(&now);

    payload["sub"] = component; // client identity
    payload["component"] = "component";

    encoding = "";
    serializeJson(payload, encoding);
    String payloadBase64 = base64url_encode(encoding);

    String token = headerBase64;
    token += '.';
    token += payloadBase64;

    // cryptographic engine
    byte hmacResult[32];
    mbedtls_md_context_t mbedtlsContext;
    mbedtls_md_type_t mbedtls_type = MBEDTLS_MD_SHA256;

    mbedtls_md_init(&mbedtlsContext);
    mbedtls_md_setup(&mbedtlsContext, mbedtls_md_info_from_type(mbedtls_type), 1);
    mbedtls_md_hmac_starts(&mbedtlsContext, (const unsigned char *)key.c_str(), key.length());
    mbedtls_md_hmac_update(&mbedtlsContext, (const unsigned char *)token.c_str(), token.length());
    mbedtls_md_hmac_finish(&mbedtlsContext, hmacResult);
    mbedtls_md_free(&mbedtlsContext);

    String signature = base64url_encode(hmacResult, 32);
    token += '.';
    token += signature;

    return token;
}

/**
 * pointer to the last initiated socket module (why would you have more than one?)
 * @note returns NULL if no module has been initialized
 */
SocketModule *SocketModule::lastModule = NULL;

SocketModule::SocketModule(String moduleName) {
    id = moduleName;
    lastModule = this;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(ip, "ip", "String");
    parameters.add(port, "port", "int");
    parameters.add(url, "url", "String");
    parameters.add(key, "key", "String");
    parameters.add(component, "component", "String");
    parameters.add(useSSL, "useSSL", "");
}

/**
 * @brief get type of the module
 * @note as this is a socket module, it will always return xrtl_socket
 */
moduleType SocketModule::getType() {
    return xrtl_socket;
}

/**
 * @brief send an event to the socket server if a connection has been established
 * @param event reference to the event formated as JsonArray
 * @note calls to this function before a connection is established will cause the event to be discarded. Events before authentication are sent, but likely ignored by the server.
 */
void SocketModule::sendEvent(JsonArray &event) {
    if (!socket->isConnected()) {
        if (!debugging) return;
        String output; // print the event to serial monitor
        serializeJson(event, output);
        debug("disconnected, unable to sent event: %s", output.c_str());
        return;
    }

    String output;
    serializeJson(event, output);
    socket->sendEVENT(output);
    debug("sent event: %s", output.c_str());
}

/**
 * @brief send an error event to the server if a connection has been established
 * @param err error number that identifies the type of error encountered
 * @param msg error message detailing what went wrong (if possible)
 * @note calls to this function before a connection is established will cause the error to be discarded. Events before authentication are sent, but likely ignored by the server.
 */
void SocketModule::sendError(componentError err, String msg) {
    DynamicJsonDocument outgoingEvent(1024);
    JsonArray payload = outgoingEvent.to<JsonArray>();

    payload.add("error");
    JsonObject parameters = payload.createNestedObject();
    parameters["errnr"] = err;
    parameters["errmsg"] = msg;

    sendEvent(payload);
}

/**
 * @brief send a binary attachment to the server
 * @param binaryLeadFrame text for the first leading websocket frame, may include additional info regarding the attachment
 * @param payload pointer to the binary attachment to send
 * @param length length of the binary attachment to send
 * @note structure of the text frame: 451-<payload as JsonArray>
 */
void SocketModule::sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length) {
    socket->sendBIN(binaryLeadFrame, payload, length);
}

void timeSyncCallback(timeval *tv) {
    SocketModule *socketInstance = SocketModule::lastModule;
    socketInstance->notify(time_synced);
}

/**
 * @brief incomming socket events are handled through this function and fed into the event pipeline
 * @param type integer indentifying socket event, see socket.io protocole for details
 * @param payload pointer to the payload
 * @param length length of the payload
 * @note the callback function needs to be static, which is why the socket class has a static pointer storing the last initialized socket instance. Hence, only one socket instance is supported. More than one socket can be initialized without error, but only the latest is ever called.
*/
void socketHandler(socketIOmessageType_t type, uint8_t *payload, size_t length) {
    SocketModule *socketInstance = SocketModule::lastModule;

    switch (type) {
    case sIOtype_DISCONNECT: {
        socketInstance->notify(socket_disconnected);
        return;
    }
    case sIOtype_CONNECT: {
        socketInstance->debug("connected to <%s>", payload);
        socketInstance->notify(socket_connected);

        String token = "{\"token\":\"";
        token += socketInstance->createJWT();
        token += "\"}";
        socketInstance->debug("sending token: %s", token.c_str());
        socketInstance->socket->send(sIOtype_CONNECT, token);
        socketInstance->debug("waiting for authentication");
        return;
    }
    case sIOtype_EVENT: {
        socketInstance->debug("got event: %s", payload);
        DynamicJsonDocument incommingEvent(1024);
        DeserializationError error = deserializeJson(incommingEvent, payload, length);
        if (error) {
            String errormsg = "deserialization failed: ";
            errormsg += error.c_str();
            socketInstance->sendError(deserialize_failed, errormsg);
            socketInstance->debug("deserializeJson() failed to analyze event: <%s>", error.c_str());
        }

        socketInstance->handleEvent(incommingEvent);
        return;
    }
    case sIOtype_ACK: {
    }
    case sIOtype_ERROR: {
        socketInstance->debug("socket.io connection declined: %s", payload);
        // socketInstance->notify(socket_declined);
    }
    case sIOtype_BINARY_EVENT: {
    }
    case sIOtype_BINARY_ACK: {
    }
    }
}

/**
 * @brief receives the event after it has been deserialized, recognizes special events and feeds them into the appropriate pipeline
 * @param doc JsonDocument holding the event
 * @note array must have the event name as first entry. Currently supported events are "Auth", "command", and "status"
*/
void SocketModule::handleEvent(DynamicJsonDocument &doc) {
    // possibly unsave: operator[] will fail if doc is a JsonObject
    if (doc[0].isNull()) {
        String errormsg = "[";
        errormsg += id;
        errormsg += "] event rejected: <event name> is null";
        sendError(field_is_null, errormsg);
        return;
    }
    if (!doc[0].is<String>()) {
        String errormsg = "[";
        errormsg += id;
        errormsg += "] event rejected: <event name> was expected to be a String";
        sendError(wrong_type, errormsg);
        return;
    }

    String eventName = doc[0].as<String>();

    if (eventName == "Auth") {
        auto payloadCandidate = doc[1];
        if (!payloadCandidate.isNull() && payloadCandidate.is<JsonObject>()) {
            JsonObject payload = doc[1];
            uint64_t receivedTime = 0;
            if (getValue("time", payload, receivedTime, true) && receivedTime != 0) {
                debug("time received from server: %llu ms", receivedTime);

                time_t now;
                time(&now);

                if (abs((long long)receivedTime / 1000 - now) < 300) { // less than 5 minutes difference
                    debug("approximate time match");
                } else {
                    debug("time mismatch, syncing to server time");
                    timeval receivedEpoch;
                    receivedEpoch.tv_sec = floor(receivedTime / 1000);
                    receivedEpoch.tv_usec = (receivedTime % 1000) * 1000;
                    sntp_set_system_time(receivedEpoch.tv_sec, receivedEpoch.tv_usec);
                }
            }
        }

        debug("authenticated by server");
        notify(socket_authed);
        return;
    }

    if (eventName == "command") {
        auto payloadCandidate = doc[1];
        if (payloadCandidate.isNull() || !payloadCandidate.is<JsonObject>()) {
            String errmsg = "[";
            errmsg += id;
            errmsg += "] command event is missing a payload";
            sendError(field_is_null, errmsg);
            return;
        }

        JsonObject payload = doc[1];
        String controlId;
        if (!getValue<String>("controlId", payload, controlId, true))
            return;
        pushCommand(controlId, payload);
    }

    if (eventName == "status")
    {
        auto payloadCandidate = doc[1];
        if (payloadCandidate.isNull() || !payloadCandidate.is<JsonObject>()) {
            String errmsg = "[";
            errmsg += id;
            errmsg += "] status event is missing a payload";
            sendError(field_is_null, errmsg);
            return;
        }

        JsonObject payload = doc[1];
        String controlId;
        if (!getValue("controlId", payload, controlId, true))
            return;
        JsonObject status;
        if (!getValue("status", payload, status, true))
            return;
        pushStatus(controlId, status);
    }
}

void SocketModule::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void SocketModule::loadSettings(JsonObject &settings) {
    parameters.load(settings);
    if (debugging && *debugging) parameters.print();
}

void SocketModule::setViaSerial() {
    parameters.setViaSerial();
}

String &SocketModule::getComponent() {
    return component;
}

void SocketModule::setup() {
    sntp_set_time_sync_notification_cb(timeSyncCallback);
    configTime(0, 0, "pool.ntp.org");

    // it seems to be important to initialize the client early
    // --> initialize with "empty" server details
    // if initialized "too late" it becomes very unresponsive and connection seems sluggish
    debug("preparing client");
    if (useSSL) {
        socket->beginSSL("0.0.0.0", 0);
    }
    else {
        socket->begin("0.0.0.0", 0);
    }
    
    socket->onEvent(socketHandler);
}

void SocketModule::loop() {
    socket->loop();

    if (clientStarted) return;

    if (esp_timer_get_time() > 300000000) { // 5 minutes after start -> WiFi not working?
        debug("unable to connect to server -- restarting device");
        // TODO: stop all modules?
        ESP.restart();
    }


}

void SocketModule::stop() {
    socket->disconnect();
}

void SocketModule::handleInternal(internalEvent eventId, String &sourceId) {
    switch (eventId) {
    case wifi_connected: {
        if (clientStarted) return;

        IPAddress serverIp;
        if (WiFiGenericClass::hostByName(ip.c_str(), serverIp) != 1) {
            debug("unable to resolve host <%s>", ip.c_str());
            return;
        }

        debug("trying to connect to server %s:%d", serverIp.toString().c_str(), port);
        if (useSSL) {
            socket->beginSSL(serverIp.toString().c_str(), port, url.c_str());
        }
        else {
            socket->begin(serverIp.toString().c_str(), port, url.c_str());
        }

        clientStarted = true;

        return;
    }
    case socket_disconnected: {
        failedConnectionCount++; // TODO: is there a way to avoid the 5s BLOCKING!!! timeout?
        debug("disconnected, connection attempt: %i", failedConnectionCount);
        if (failedConnectionCount > 55) {
            debug("unable to connect -- restarting device");
            ESP.restart();
        }
        return;
    }
    case socket_connected: {
        failedConnectionCount = 0;

        debug("succesfully connected to server");
        return;
    }
    case socket_authed: {
        sendAllStatus();
        return;
    }
    // case socket_declined: {
    //     return;
    // }

    case time_synced: {

        if (!debugging) return; // only need to print time if debugging

        uint32_t sec;
        uint32_t uSec;
        sntp_get_system_time(&sec, &uSec);

        debug("time synced: %lu.%lu s", sec, uSec / 1000);
        return;
    }
    }
}