#include "SocketIOclientMod.h"

/**
 * @brief send a binary attachment over socket.io
 * @param binaryLeadFrame String that gets used in leading websocket text frame, must obey socket.io and engine.io specifics* for attachments
 * @param payload pointer to the binary payload to send
 * @param length length of the binary attachment
 * @note *see https://github.com/socketio/socket.io-protocol: 451[{"_placeholder":true,"num":0}]
*/
bool SocketIOclientMod::sendBIN(String &binaryLeadFrame, uint8_t *payload, size_t length, bool headerToPayload)
{
    bool ret = false;

    if (length == 0)
    {
        length = strlen((const char *)payload);
    }

    ret = sendFrame(&_client, WSop_text, (uint8_t *)binaryLeadFrame.c_str(), binaryLeadFrame.length(), true, headerToPayload);

    if (ret)
    {
        ret = sendFrame(&_client, WSop_binary, payload, length, true, headerToPayload);
    }

    return ret;
}

/**
 * @brief send a binary attachment over socket.io
 * @param binaryLeadFrame String that gets used in leading websocket text frame, must obey socket.io and engine.io specifics* for attachments
 * @param payload pointer to the binary payload to send
 * @param length length of the binary attachment
 * @note *see https://github.com/socketio/socket.io-protocol: 451[{"_placeholder":true,"num":0}]
*/
bool SocketIOclientMod::sendBIN(String &binaryLeadFrame, const uint8_t *payload, size_t length)
{
    return sendBIN(binaryLeadFrame, (uint8_t *)payload, length);
}

/**
 * @brief send disconnect package to server
 * @note no disconnect message sent by default
*/
bool SocketIOclientMod::disconnect()
{
    return send(sIOtype_DISCONNECT, "");
}