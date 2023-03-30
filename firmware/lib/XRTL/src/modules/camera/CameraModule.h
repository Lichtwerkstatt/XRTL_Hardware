#ifndef CAMERAMODULE_H
#define CAMERAMODULE_H

#include "modules/XRTLmodule.h"
#include <esp_camera.h>

class CameraModule : public XRTLmodule {
    private:
    bool isStreaming = false;
    int64_t nextFrame = 0;// stores when the next frame should be send; µs
    uint32_t frameTimeMicroSeconds = 100000;// minimum time interval between frames in µs; time might be higher due to load
    String binaryLeadFrame;// content of websocket text frame to be send prior to binary data

    static camera_config_t camera_config;
    sensor_t* cameraSettings = NULL;

    //virtual PTZ
    uint8_t panStage = 4;
    uint8_t tiltStage = 4;
    uint8_t zoomStage = 0;

    // camera settings
    bool isGray = false;
    int brightness = 0;
    int contrast = 0;
    int exposure = 200;
    framesize_t frameSize = FRAMESIZE_QVGA;

    public:
    CameraModule(String moduleName);
    moduleType type = xrtl_camera;
    moduleType getType();

    void setup();
    void loop();

    bool getStatus(JsonObject& status);
    void saveSettings(JsonObject& settings);
    void loadSettings(JsonObject& settings);
    void setViaSerial();
    
    void startStreaming();
    void stopStreaming();
    bool handleCommand(String& command);
    void handleCommand(String& controlId, JsonObject& command);
    void handleInternal(internalEvent eventId, String& sourceId);

    //void getStatus();

    void virtualPTZ();
};

#endif