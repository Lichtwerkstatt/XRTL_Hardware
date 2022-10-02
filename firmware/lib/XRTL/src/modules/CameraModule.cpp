#include "CameraModule.h"

CameraModule::CameraModule(String moduleName, XRTL* source){
    id = moduleName;
    xrtl = source;
}

moduleType CameraModule::getType() {
  return xrtl_camera;
}

camera_config_t CameraModule::camera_config = {
    .pin_pwdn  = 32,
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

    .xclk_freq_hz = 20000000,//EXPERIMENTAL: Set to 16MHz on ESP32-S2 or ESP32-S3 to enable EDMA mode
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,

    .pixel_format = PIXFORMAT_JPEG,//YUV422,GRAYSCALE,RGB565,JPEG
    .frame_size = FRAMESIZE_XGA,//UXGA,//QQVGA-QXGA Do not use sizes above QVGA when not JPEG

    .jpeg_quality = 10,//0-63 lower number means higher quality
    .fb_count = 2, //if more than one, i2s runs in continuous mode. Use only with JPEG
    .grab_mode = CAMERA_GRAB_LATEST//CAMERA_GRAB_WHEN_EMPTY. Sets when buffers should be filled
};

void CameraModule::setup() {
  //initialize the camera
  esp_err_t err = esp_camera_init(&camera_config);
  if (err != ESP_OK) {
    debug("camera init failed: %s", err);
    return;
  }
  cameraSettings = esp_camera_sensor_get();
  debug("camera initialized");
}

void CameraModule::loop() {
  if (!isStreaming) return;
  
  int64_t now = esp_timer_get_time();
  if (now < nextFrame) return;

  camera_fb_t *fb = esp_camera_fb_get();
  sendBinary(binaryLeadFrame, fb->buf,fb->len);
  esp_camera_fb_return(fb);
  //debug("time needed to send image: %f ms", (double) (esp_timer_get_time() - now)/1000);

  nextFrame = now + frameTimeMicroSeconds;
}

void CameraModule::getStatus(JsonObject& payload, JsonObject& status) {
  if (isStreaming) {
    auto busyField = status["busy"];
    if (!busyField.as<bool>()) {// don't waste JSsonObject memory
      busyField = true;
    }
  }

  JsonObject moduleStatus = status.createNestedObject(id);
  moduleStatus["stream"] = isStreaming;
}

void CameraModule::saveSettings(JsonObject& settings) {

}

void CameraModule::loadSettings(JsonObject& settings) {

}

void CameraModule::setViaSerial() {

}

void CameraModule::startStreaming() {
  if (isStreaming) {
    debug("stream already active");
    return;
  }
  debug("starting camera stream");

  DynamicJsonDocument doc(512);
  JsonArray event = doc.to<JsonArray>();

  event.add("data");
  JsonObject payload = event.createNestedObject();
  payload["componentId"] = getComponent();
  payload["type"] = "image";
  payload["dataId"] = id;

  JsonObject data = payload.createNestedObject("data");
  data["_placeholder"] = true;
  data["num"] = 0;

  binaryLeadFrame = "451-";
  serializeJson(doc,binaryLeadFrame);

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
  uint16_t xOffset = round(zoomFactor * 177.78 * (float) panStage);
  uint16_t yOffset = round(zoomFactor * 133.33 * (float) tiltStage);

  cameraSettings->set_res_raw(cameraSettings, 0, 0, 0, 0, xOffset, yOffset, xLength, yLength, xLength, yLength, true, true);
}

bool CameraModule::handleCommand(String& command) {
  if (strcmp(command.c_str(),"startStreaming") == 0) {
    startStreaming();
    return true;
  }
  if (strcmp(command.c_str(),"stopStreaming") == 0) {
    stopStreaming();
    return true;
  }

  return false;
}

bool CameraModule::handleCommand(String& controlId, JsonObject& command) {
  //if (strcmp(!isModule(controlId)) return false;
  //remove these later?
  if (strcmp(controlId.c_str(),"gray") == 0) {
    bool val;
    if (getValue<bool>("val", command, val)) {
      command["gray"] = val;
    }
  }
  else if (strcmp(controlId.c_str(),"brightness") == 0) {
    int val;
    if (getValue<int>("val", command, val)) {
      command["brightness"] = val;
    }
  }
  else if (strcmp(controlId.c_str(),"contrast") == 0) {
    int val;
    if (getValue<int>("val", command, val)) {
      command["contrast"] = val;
    }
  }

  bool stream;
  if (getValue<bool>("stream", command, stream)) {
    if (stream) startStreaming();
    else stopStreaming();
  }

  bool gray;
  if (getValue<bool>("gray", command, gray)) {
    if (gray) {
      cameraSettings->set_special_effect(cameraSettings, 2);
      debug("gray filter enabled");
    }
    else {
      cameraSettings->set_special_effect(cameraSettings, 0);
      debug("gray filter disabled");
    }
  }

  int brightness;
  if (getValue<int>("brightness", command, brightness, -2, 2)) {
    cameraSettings->set_brightness(cameraSettings, brightness);
    debug("brightness set to %d", brightness);
  }

  int contrast;
  if (getValue<int>("contrast", command, contrast, -2, 2)) {
    cameraSettings->set_contrast(cameraSettings, contrast);
    debug("contrast set to %d", contrast);
  }

  if (getValue<uint8_t>("pan", command, panStage, 0, 8)) {
    virtualPTZ();
    debug("pan stage set to %d", panStage);
  }

  if (getValue<uint8_t>("tilt", command, tiltStage, 0, 8)) {
    virtualPTZ();
    debug("tilt stage set to %d", tiltStage);
  }

  if (getValue<uint8_t>("zoom", command, zoomStage, 0, 4)){
    virtualPTZ();
    debug("zoom stage set to %d", zoomStage);
  }

  String frameSize;
  if (getValue<String>("frame size", command, frameSize)) {
    debug("setting frame size to %s", frameSize);
    if (strcmp(frameSize.c_str(),"UXGA") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_UXGA);
      // 1600 x 1200 px
    }
    else if (strcmp(frameSize.c_str(),"QVGA") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_QVGA);
      // 320 x 240 px
    }
    else if (strcmp(frameSize.c_str(),"CIF") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_CIF);
      // 352 x 288 px
    }
    else if (strcmp(frameSize.c_str(),"VGA") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_VGA);
      // 640 x 480 px
    }
    else if (strcmp(frameSize.c_str(),"SVGA") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_SVGA);
      // 800 x 600 px
    }
    else if (strcmp(frameSize.c_str(),"XGA") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_XGA);
      // 1024 x 768 px
    }
    else if (strcmp(frameSize.c_str(),"SXGA") == 0) {
      cameraSettings->set_framesize(cameraSettings, FRAMESIZE_SXGA);
      // 1280 x 1024 px
    }
    else {
      String errormsg = "[";
      errormsg += id;
      errormsg += "] <frame size> has unknown value, complete list: UXGA, QVGA, CIF, VGA, SVGA, XGA, SXGA";
      sendError(out_of_bounds,errormsg);
    }
  }

  return true;
}

void CameraModule::handleInternal(internalEvent event) {
  switch(event) {
    case socket_disconnected: {
      // TODO: suspend stream?
      return;
    }

    case debug_on:{
      debugging = true;
      return;
    }

    case debug_off:{
      debugging = false;
      return;
    }
  }
}