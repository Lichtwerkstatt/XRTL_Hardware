#ifndef XRTL_H
#define XRTL_H

#include "common/XRTLinternalHook.h"
#include "modules/camera/CameraModule.h"
#include "modules/infoLED/InfoLEDModule.h"
#include "modules/input/InputModule.h"
#include "modules/macro/Macromodule.h"
#include "modules/output/OutputModule.h"
#include "modules/servo/ServoModule.h"
#include "modules/socket/SocketModule.h"
#include "modules/stepper/StepperModule.h"
#include "modules/wifi/WifiModule.h"

// core, module manager
class XRTL {
private:
    String id = "core";
    // store modules here
    uint8_t moduleCount;
    XRTLmodule *module[16];
    // endpoint for sending
    SocketModule *socketIO = NULL;

    // custom internal events
    uint8_t internalCount = 0;
    InternalHook *customInternal[16];

    // custom event handler
    // EventHook* customEvent[16];

    // enable serial debug output
    bool debugging = true;

    ParameterPack parameters;

public:
    ~XRTL();
    // manage Modules
    void listModules();
    bool addModule(String moduleName, moduleType category);
    void delModule(uint8_t number);
    void swapModules(uint8_t numberX, uint8_t numberY);
    XRTLmodule *operator[](String moduleName); // returns pointer to module with specified ID

    String &getComponent();

    // offer command and status events to modules
    void pushCommand(String &controlId, JsonObject &command);
    void pushStatus(String &controlId, JsonObject &status);

    // send stuff via endpoint
    void sendEvent(JsonArray &event);
    void sendError(componentError err, String msg);
    void sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length);
    void sendCommand(XRTLcommand &command, const String &userId);

    // custom internal hooks
    void manageInternal();
    void addInternal();
    void delInternal();
    void swpInternal();

    /**
     * @brief send a message over the serial monitor if in debug mode
     * @param ... uses printf syntax
     * @note automatically adds [controlId] at the start and \\n at the end of the message
     */
    template <typename... Args>
    void debug(Args... args) {
        if (!debugging) return;

        Serial.printf("[%s] ", id.c_str());
        Serial.printf(args...);
        Serial.print('\n');
    };

    // manage module settings
    void loadSettings();
    void saveSettings();
    void setViaSerial();
    bool settingsDialog();
    void sendStatus();

    // calls corresponding methodes of all modules
    void setup();
    void loop();
    void stop();

    // distribute event to all modules
    void notify(internalEvent eventId, String &sourceId);
};

#endif