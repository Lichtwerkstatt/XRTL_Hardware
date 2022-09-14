#ifndef CAMERAMODULE_H
#define CAMERAMODULE_H

#include "XRTLmodule.h"
#include <esp_camera.h>

class CameraModule : public XRTLmodule {
    private:
    bool isStreaming = false;
    int64_t nextFrame = 0;

    static camera_config_t camera_config;
    sensor_t* settings = NULL;

    //virtual PTZ
    uint8_t panStage = 4;
    uint8_t tiltStage = 4;
    uint8_t zoomStage = 0;

    public:
    CameraModule(String moduleName, XRTL* source);

    void setup();

    //void loop();

    //void getStatus();

    void virtualPTZ();

    template<typename... Args>
    void debug(Args... args);
};

#endif