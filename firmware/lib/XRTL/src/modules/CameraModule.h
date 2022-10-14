#ifndef CAMERAMODULE_H
#define CAMERAMODULE_H

#include "XRTLmodule.h"
#include <esp_camera.h>

class CameraModule : public XRTLmodule {
    private:
    bool isStreaming = false;
    int64_t nextFrame = 0;
    uint32_t frameTimeMicroSeconds = 100000;
    String binaryLeadFrame;

    static camera_config_t camera_config;
    sensor_t* cameraSettings = NULL;

    //virtual PTZ
    uint8_t panStage = 4;
    uint8_t tiltStage = 4;
    uint8_t zoomStage = 0;

    public:
    CameraModule(String moduleName, XRTL* source);
    moduleType getType();

    void setup();
    void loop();

    void getStatus(JsonObject& payload, JsonObject& status);
    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
    
    void startStreaming();
    void stopStreaming();
    bool handleCommand(String& command);
    bool handleCommand(String& controlId, JsonObject& command);
    void handleInternal(internalEvent eventId, String& sourceId);

    //void getStatus();

    void virtualPTZ();
};

#endif