#include "CameraModule.h"

CameraModule::CameraModule(String moduleName) {
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");
}

moduleType CameraModule::getType() {
    return xrtl_camera;
}

camera_config_t CameraModule::camera_config = {
    .pin_pwdn = 32,
    .pin_reset = -1,
    .pin_xclk = 0,
    .pin_sscb_sda = 26,
    .pin_sscb_scl = 27,

    .pin_d7 = 35,
    .pin_d6 = 34,
    .pin_d5 = 39,
    .pin_d4 = 36,
    .pin_d3 = 21,
    .pin_d2 = 19,
    .pin_d1 = 18,
    .pin_d0 = 5,
    .pin_vsync = 25,
    .pin_href = 23,
    .pin_pclk = 22,

    .xclk_freq_hz = 20000000, // EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG, // YUV422,GRAYSCALE,RGB565,JPEG
    // IMPORTANT: frame size must be initialized with a high value
    // frame buffer might otherwise be initialized too small and _WILL_ fail silently if the size is increased later
    .frame_size = FRAMESIZE_UXGA, // UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 10,             // 0-63 lower number means higher quality
    .fb_count = 2,                  // if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_LATEST // CAMERA_GRAB_WHEN_EMPTY. Sets when buffers should be filled
};

void CameraModule::setup() {
    // initialize the camera
    initStatus = esp_camera_init(&camera_config);
    if (initStatus != ESP_OK) {
        // debug("camera init failed: %s", err); // TODO: investigate core panic if init failed after restart
        debug("camera init failed");
        String errmsg = "[";
        errmsg += id.c_str();
        errmsg += "] unable to initialize camera hardware";
        sendError(hardware_failure, errmsg);

        return;
    }

    cameraSettings = esp_camera_sensor_get();
    cameraSettings->set_framesize(cameraSettings, FRAMESIZE_QVGA);
    cameraSettings->set_gain_ctrl(cameraSettings, 0);
    cameraSettings->set_exposure_ctrl(cameraSettings, 0);
    cameraSettings->set_aec_value(cameraSettings, exposure);
    debug("camera initialized");
}

void CameraModule::loop() {
    if (!isStreaming || initStatus != ESP_OK) return;

    int64_t now = esp_timer_get_time();
    if (now < nextFrame) return;

    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
        debug("buffer invalid");
        String errmsg = "[";
        errmsg += id.c_str();
        errmsg += "] unable to obtain camera buffer, closing stream";
        sendError(hardware_failure, errmsg);

        stopStreaming();
        return;
    }

    sendBinary(binaryLeadFrame, fb->buf, fb->len);
    esp_camera_fb_return(fb);
    // debug("time needed to send image: %f ms", (double) (esp_timer_get_time() - now)/1000);

    nextFrame = now + frameTimeMicroSeconds;
}

bool CameraModule::getStatus(JsonObject &status) {
    if (initStatus != ESP_OK) {
        String errmsg = "[";
        errmsg += id;
        errmsg += "] camera not initialized, check connectors";
        sendError(hardware_failure, errmsg);
        
        return false;
    }

    status["stream"] = isStreaming;
    status["frameSize"] = frameSize;
    status["gray"] = isGray;
    // status["brightness"] = brightness;
    status["exposure"] = exposure;
    status["contrast"] = contrast;

    return true;
}

void CameraModule::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void CameraModule::loadSettings(JsonObject &settings) {
    parameters.load(settings);
    if (debugging && *debugging) parameters.print();
}

void CameraModule::setViaSerial() {
    parameters.setViaSerial();
}

void CameraModule::startStreaming() {
    if (isStreaming) {
        debug("stream already active");
        sendStatus();
        return;
    }
    debug("starting camera stream");

    DynamicJsonDocument doc(512);
    JsonArray event = doc.to<JsonArray>();

    event.add("data");
    JsonObject payload = event.createNestedObject();
    payload["controlId"] = id;
    payload["type"] = "image";
    // payload["dataId"] = id;

    JsonObject data = payload.createNestedObject("data");
    data["_placeholder"] = true;
    data["num"] = 0;

    binaryLeadFrame.clear();
    binaryLeadFrame = "451-";
    serializeJson(doc, binaryLeadFrame);

    debug("websocket frame for camera created: %s", binaryLeadFrame.c_str());
    isStreaming = true;
    sendStatus();

    nextFrame = esp_timer_get_time();
}

void CameraModule::stopStreaming() {
    if (!isStreaming) {
        debug("stream already inactive");
        return;
    }

    debug("stopping camera stream");
    isStreaming = false;
    sendStatus();
}

void CameraModule::virtualPTZ() {
    // equations
    // xOffset = panStage * [ width/(N-1) * (1- 1/zoomFactor) ]
    // yOffset = tiltStage * [ height/(N-1) * (1 - 1/zoomFactor) ]
    // xLength = width / zoomFactor
    // yLength = height / zoomFactor

    // set zoom factor, use to calculate width and height
    float zoomFactor = 1.0 + zoomStage * 3.0;
    uint16_t xLength = floor(1600.0 / zoomFactor);
    uint16_t yLength = floor(1200.0 / zoomFactor);

    // re-use zoomFactor for offset calculation (common factor of 1-1/zoomFactor)
    zoomFactor = 1.0 - 1.0 / zoomFactor;
    uint16_t xOffset = round(zoomFactor * 177.78 * (float)panStage);
    uint16_t yOffset = round(zoomFactor * 133.33 * (float)tiltStage);

    cameraSettings->set_res_raw(cameraSettings, 0, 0, 0, 0, xOffset, yOffset, xLength, yLength, xLength, yLength, true, true);
}

void CameraModule::handleCommand(String &controlId, JsonObject &command) {
    if (!isModule(controlId) && controlId != "*")
        return;

    bool targetStreamState = false;
    if (getValue<bool>("stream", command, targetStreamState)) {
        if (targetStreamState)
            startStreaming();
        else
            stopStreaming();
    }

    bool getStatus = false;
    if (getValue<bool>("getStatus", command, getStatus) && getStatus) {
        sendStatus();
    }

    // camera settings follow here, should be inaccessible if cam not initialized
    if (initStatus != ESP_OK) {
        sendStatus(); // will issue error
        return;
    }

    // raw camera drivers exposed

    int driverInt;
    bool driverBool;

    if (getAndConstrainValue<int>("saturation", command, driverInt, -2, 2)) {
        cameraSettings->set_saturation(cameraSettings, driverInt);
        debug("saturation set to %d", driverInt);
    }

    if (getAndConstrainValue<int>("sharpness", command, driverInt, -2, 2)) {
        cameraSettings->set_sharpness(cameraSettings, driverInt);
        debug("sharpness set to %d", driverInt);
    }

    if (getAndConstrainValue<int>("denoise", command, driverInt, 0, 255)) {
        cameraSettings->set_denoise(cameraSettings, driverInt);
        debug("denoise set to %d", driverInt);
    }

    if (getAndConstrainValue<int>("gainceiling", command, driverInt, 0, 6)) {
        cameraSettings->set_gainceiling(cameraSettings, (gainceiling_t) driverInt);
        debug("gainceiling set to %dx", pow(2, driverInt));
    }

    if (getAndConstrainValue<int>("quality", command, driverInt, 0, 63)) {
        cameraSettings->set_quality(cameraSettings, driverInt);
        debug("quality set to %d", driverInt);
    }

    if (getValue<bool>("colorbar", command, driverBool)) {
        cameraSettings->set_colorbar(cameraSettings, driverBool);
        debug("colorbar %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("whitebal", command, driverBool)) {
        cameraSettings->set_whitebal(cameraSettings, driverBool);
        debug("whitebal %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("exposure_ctrl", command, driverBool)) {
        cameraSettings->set_exposure_ctrl(cameraSettings, driverBool);
        debug("exposure_ctrl %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("hmirror", command, driverBool)) {
        cameraSettings->set_hmirror(cameraSettings, driverBool);
        debug("hmirror %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("vflip", command, driverBool)) {
        cameraSettings->set_vflip(cameraSettings, driverBool);
        debug("vflip %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("aec2", command, driverBool)) {
        cameraSettings->set_aec2(cameraSettings, driverBool);
        debug("aec2 %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("awb_gain", command, driverBool)) {
        cameraSettings->set_awb_gain(cameraSettings, driverBool);
        debug("awb_gain %sabled", driverBool ? "en" : "dis");
    }

    if (getAndConstrainValue<int>("agc_gain", command, driverInt, 0, 30)) {
        cameraSettings->set_agc_gain(cameraSettings, driverInt);
        debug("agc_gain set to %d", driverInt);
    }

    if (getAndConstrainValue<int>("aec_value", command, driverInt, 0, 1200)) {
        cameraSettings->set_aec_value(cameraSettings, driverInt);
        debug("aec_value set to %d", driverInt);
    }

    if (getAndConstrainValue<int>("wb_mode", command, driverInt, 0, 4)) {
        cameraSettings->set_wb_mode(cameraSettings, driverInt);
        debug("wb_mode set to %d", driverInt);
    }

    if (getAndConstrainValue<int>("ae_level", command, driverInt, -2, 2)) {
        cameraSettings->set_ae_level(cameraSettings, driverInt);
        debug("ae_level set to %d", driverInt);
    }

    if (getValue<bool>("dcw", command, driverBool)) {
        cameraSettings->set_dcw(cameraSettings, driverBool);
        debug("dcw %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("bpc", command, driverBool)) {
        cameraSettings->set_bpc(cameraSettings, driverBool);
        debug("bpc %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("wpc", command, driverBool)) {
        cameraSettings->set_wpc(cameraSettings, driverBool);
        debug("wpc %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("raw_gma", command, driverBool)) {
        cameraSettings->set_raw_gma(cameraSettings, driverBool);
        debug("raw_gma %sabled", driverBool ? "en" : "dis");
    }

    if (getValue<bool>("lenc", command, driverBool)) {
        cameraSettings->set_lenc(cameraSettings, driverBool);
        debug("lenc %sabled", driverBool ? "en" : "dis");
    }

    if (getAndConstrainValue<int>("special_effect", command, driverInt, 0, 6)) {
        cameraSettings->set_special_effect(cameraSettings, driverInt);
        debug("special_effect set to %d", driverInt);
    }

    // old code

    if (getValue<bool>("gray", command, isGray)) {
        if (isGray) {
            cameraSettings->set_special_effect(cameraSettings, 2);
            debug("gray filter enabled");
        } else {
            cameraSettings->set_special_effect(cameraSettings, 0);
            debug("gray filter disabled");
        }
        sendStatus();
    }

    int gain;
    if (getAndConstrainValue<int>("gain", command, gain, 0, 30)) {
        cameraSettings->set_gain_ctrl(cameraSettings, 0);
        cameraSettings->set_agc_gain(cameraSettings, gain);
        debug("gain changed to %d", gain);
    }

    if (getAndConstrainValue<int>("exposure", command, exposure, 0, 1200)) {
        cameraSettings->set_gain_ctrl(cameraSettings, 0);
        cameraSettings->set_exposure_ctrl(cameraSettings, 0);
        cameraSettings->set_aec_value(cameraSettings, exposure);
        debug("exposure set to %d", exposure);
        sendStatus();
    }

    if (getAndConstrainValue<int>("brightness", command, brightness, -2, 2)) {
        cameraSettings->set_brightness(cameraSettings, brightness);
        debug("brightness set to %d", brightness);
        sendStatus();
    }

    if (getAndConstrainValue<int>("contrast", command, contrast, -2, 2)) {
        cameraSettings->set_contrast(cameraSettings, contrast);
        debug("contrast set to %d", contrast);
        sendStatus();
    }

    int8_t virtualPTZtarget;
    if (getValue<int8_t>("virtualPan", command, virtualPTZtarget)) {
        if (virtualPTZtarget > 0) {
            panStage++;
        } else if (virtualPTZtarget < 0) {
            panStage--;
        }
        panStage = constrain(panStage, 0, 8);
        virtualPTZ();
        debug("pan stage set to %d", panStage);
        sendStatus();
    }

    if (getValue<int8_t>("virtualTilt", command, virtualPTZtarget)) {
        if (virtualPTZtarget > 0) {
            tiltStage++;
        } else if (virtualPTZtarget < 0) {
            tiltStage--;
        }
        tiltStage = constrain(tiltStage, 0, 8);
        virtualPTZ();
        debug("tilt stage set to %d", tiltStage);
        sendStatus();
    }

    if (getValue<int8_t>("virtualZoom", command, virtualPTZtarget)) {
        if (virtualPTZtarget > 0) {
            zoomStage++;
        } else if (virtualPTZtarget < 0) {
            zoomStage--;
        }
        zoomStage = constrain(zoomStage, 0, 4);
        virtualPTZ();
        debug("zoom stage set to %d", zoomStage);
        sendStatus();
    }

    double targetFrameRate = 15;
    if (getAndConstrainValue<double>("frame rate", command, targetFrameRate, 0, 30)) {
        frameTimeMicroSeconds = round((double)1000000 / targetFrameRate);
        if (isStreaming) {
            nextFrame = esp_timer_get_time() + frameTimeMicroSeconds;
        }
    }

    uint8_t targetFrameSize = 0;
    if (getAndConstrainValue<uint8_t>("frameSize", command, targetFrameSize, 5, 13)) {
        if (targetFrameSize == 7 || targetFrameSize == 11) {
            String errormsg = "[";
            errormsg += id;
            errormsg += "] <frameSize> 7 and 11 are not permitted";
            sendError(out_of_bounds, errormsg);
            targetFrameSize = 10;
        }

        frameSize = (framesize_t)targetFrameSize;
        cameraSettings->set_framesize(cameraSettings, frameSize);
        debug("frame size changed to %d", targetFrameSize);
        sendStatus();
    }

}

void CameraModule::handleInternal(internalEvent eventId, String &sourceId) {
    switch (eventId) {
    case socket_disconnected: {
        // TODO: suspend stream instead of stopping?
        if (!isStreaming)
            return;
        stopStreaming();
        debug("stream stopped due to disconnect");
        return;
    }
    }
}