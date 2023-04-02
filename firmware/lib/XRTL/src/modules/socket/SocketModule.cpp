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

bool base64url_encodeCharArray(const unsigned char *in, unsigned int inlen, char *out)
{
    unsigned int i, j;

    for (i = j = 0; i < inlen; i++)
    {
        int s = i % 3; // from 8 to 6 bit

        switch (s)
        {
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
    i -= 1;

    if ((i % 3) == 0)
    {
        out[j++] = base64url_en[(in[i] & 0x3) << 4];
    }
    else if ((i % 3) == 1)
    {
        out[j++] = base64url_en[(in[i] & 0xF) << 2];
    }

    // end string
    out[j++] = 0;

    return true;
}

String base64url_encode(const uint8_t *data, size_t dataLength)
{
    size_t outputLength = ((((4 * dataLength) / 3) + 3) & ~3);
    char *buffer = (char *)malloc(outputLength + 1);
    if (buffer)
    {
        base64url_encodeCharArray((const unsigned char *)&data[0], dataLength, &buffer[0]);
        String output = String(buffer);
        free(buffer);
        return output;
    }
    return String("-FAIL-");
}

String base64url_encode(String &text)
{
    return base64url_encode((uint8_t *)text.c_str(), text.length());
}

String SocketModule::createJWT()
{
    DynamicJsonDocument document(512);

    JsonObject header = document.to<JsonObject>();
    header["alg"] = "HS256";
    header["typ"] = "JWT";

    String encoding;
    serializeJson(header, encoding);
    String headerBase64 = base64url_encode(encoding);

    JsonObject payload = document.to<JsonObject>();
    time_t now;
    time(&now);
    payload["sub"] = component;
    payload["component"] = "component";
    payload["iat"] = now;
    payload["exp"] = now + 605000; // ~ 1 week

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

SocketModule *SocketModule::lastModule = NULL;

SocketModule::SocketModule(String moduleName)
{
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

moduleType SocketModule::getType()
{
    return xrtl_socket;
}

void SocketModule::sendEvent(JsonArray &event)
{
    if (!socket->isConnected())
    {
        if (!debugging)
            return;
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

void SocketModule::sendError(componentError err, String msg)
{
    DynamicJsonDocument outgoingEvent(1024);
    JsonArray payload = outgoingEvent.to<JsonArray>();

    payload.add("error");
    JsonObject parameters = payload.createNestedObject();
    parameters["errnr"] = err;
    parameters["errmsg"] = msg;

    sendEvent(payload);
}

void SocketModule::sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length)
{
    socket->sendBIN(binaryLeadFrame, payload, length);
}

void socketHandler(socketIOmessageType_t type, uint8_t *payload, size_t length)
{
    SocketModule *socketInstance = SocketModule::lastModule;
    switch (type)
    {
    case sIOtype_DISCONNECT:
    {
        socketInstance->notify(socket_disconnected);
        return;
    }
    case sIOtype_CONNECT:
    {
        socketInstance->debug("connected to <%s>", payload);
        socketInstance->failedConnectionCount = 0;
        socketInstance->notify(socket_connected);

        String token = "{\"token\":\"";
        token += socketInstance->createJWT();
        token += "\"}";
        socketInstance->debug("sending token: %s", token.c_str());
        socketInstance->socket->send(sIOtype_CONNECT, token);
        socketInstance->debug("waiting for authentication");
        return;
    }
    case sIOtype_EVENT:
    {
        socketInstance->debug("got event: %s", payload);
        DynamicJsonDocument incommingEvent(1024);
        DeserializationError error = deserializeJson(incommingEvent, payload, length);
        if (error)
        {
            String errormsg = "deserialization failed: ";
            errormsg += error.c_str();
            socketInstance->sendError(deserialize_failed, errormsg);
            socketInstance->debug("deserializeJson() failed to analyze event: <%s>", error.c_str());
        }

        socketInstance->handleEvent(incommingEvent);
        return;
    }
    case sIOtype_ACK:
    {
    }
    case sIOtype_ERROR:
    {
    }
    case sIOtype_BINARY_EVENT:
    {
    }
    case sIOtype_BINARY_ACK:
    {
    }
    }
}

void SocketModule::handleEvent(DynamicJsonDocument &doc)
{
    // possibly unsave: operator[] will fail if doc is a JsonObject
    if (doc[0].isNull())
    {
        String errormsg = "[";
        errormsg += id;
        errormsg += "] event rejected: <event name> is null";
        sendError(field_is_null, errormsg);
        return;
    }
    if (!doc[0].is<String>())
    {
        String errormsg = "[";
        errormsg += id;
        errormsg += "] event rejected: <event name> was expected to be a String";
        sendError(wrong_type, errormsg);
        return;
    }

    String eventName = doc[0].as<String>();

    if (eventName == "Auth")
    {
        debug("authenticated by server");
        notify(socket_authed);
        return;
    }

    if (eventName != "command")
        return; // currently only command events are analyzed

    JsonObject payload = doc[1];
    String controlId;
    if (!getValue<String>("controlId", payload, controlId, true))
        return;
    pushCommand(controlId, payload);
}

void SocketModule::saveSettings(JsonObject &settings)
{
    /*settings["ip"] = ip;
    settings["port"] = port;
    settings["url"] = url;
    settings["key"] = key;
    settings["component"] = component;
    settings["useSSL"] = useSSL;

    return;*/
    parameters.save(settings);
}

void SocketModule::loadSettings(JsonObject &settings)
{
    /*ip = loadValue<String>("ip", settings, "192.168.1.1");
    port = loadValue<uint16_t>("port", settings, 3000);
    url = loadValue<String>("url", settings, "/socket.io/?EIO=4");
    key = loadValue<String>("key", settings, "");
    component = loadValue<String>("component", settings, "NAME ME!");
    useSSL = loadValue<bool>("useSSL", settings, false);

    if (!debugging) return;

    Serial.printf("ip: %s\n", ip.c_str());
    Serial.printf("port: %i\n", port);
    Serial.printf("url: %s\n", url.c_str());
    Serial.printf("key: %s\n", key.c_str());
    Serial.printf("componentId: %s\n", component.c_str());
    Serial.printf("%s SSL\n", useSSL ? "" : "no");*/
    parameters.load(settings);
    if (debugging)
        parameters.print();
}

void SocketModule::setViaSerial()
{
    /*Serial.println("");
    Serial.println(centerString("",39, '-'));
    Serial.println(centerString(id,39, ' '));
    Serial.println(centerString("",39, '-'));
    Serial.println("");

    ip = serialInput("IP: ");
    port = serialInput("port: ").toInt();
    url = serialInput("URL: ");
    key = serialInput("key: ");
    component = serialInput("componentId: ");
    useSSL = (serialInput("use SSL (y/n): ") == "y");*/
    parameters.setViaSerial();
}

String &SocketModule::getComponent()
{
    return component;
}

void SocketModule::setup()
{
    configTime(0, 0, "pool.ntp.org");
}

void SocketModule::loop()
{
    socket->loop();

    if (timeSynced)
        return; // check if a restart might be needed

    time_t now;
    time(&now);

    // check if time is 2022 or later
    if (now > 1640991600)
    {
        // time probably synced, stop checking and push state to modules
        timeSynced = true;
        notify(time_synced);
        return;
    }

    if (esp_timer_get_time() > 300000000)
    { // 5 minutes after start -> WiFi not working?
        debug("unable to sync time -- restating device");
        ESP.restart();
    }
}

void SocketModule::stop()
{
    socket->disconnect();
}

void SocketModule::handleInternal(internalEvent eventId, String &sourceId)
{
    switch (eventId)
    {
    case socket_disconnected:
    {
        failedConnectionCount++; // TODO: is there a way to avoid the 5s BLOCKING!!! timeout?
        debug("disconnected, connection attempt: %i", failedConnectionCount);
        if (failedConnectionCount > 49)
        {
            debug("unable to connect -- restarting device");
            ESP.restart();
        }
        return;
    }
    case socket_connected:
    {
        failedConnectionCount = 0;

        debug("succesfully connected to server");
        return;
    }
    case socket_authed:
    {
        // wait randomly between 100 and 300 ms to decrease number of simultaneous request after power outage
        int64_t waitTime = esp_timer_get_time() + random(100000, 300000);
        while (esp_timer_get_time() < waitTime)
        {
            yield();
        }
        sendAllStatus();

        return;
    }

    case time_synced:
    {
        // collected all information needed for JWT, starting socket client and registering event handler

        if (useSSL)
        {
            socket->beginSSL(ip.c_str(), port, url.c_str());
        }
        else
        {
            socket->begin(ip.c_str(), port, url.c_str());
        }

        socket->onEvent(socketHandler);

        if (!debugging)
            return; // only need to print time if debugging
        time_t now;
        time(&now);
        Serial.printf("[%s] time synced: %d\n", id.c_str(), now);
        Serial.printf("[%s] socket client started\n", id.c_str());
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