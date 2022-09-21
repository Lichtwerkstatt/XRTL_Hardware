#include "CameraModule.h"

CameraModule::CameraModule(String moduleName, XRTL* source){
    id = moduleName;
    xrtl = source;
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
    debug("[%s] camera init failed: %s", id.c_str(), err);
    return;
  }
  settings = esp_camera_sensor_get();
  debug("[%s] camera initialized", id.c_str());
}

void CameraModule::virtualPTZ() {
  // equations 
  // xOffset = panStage * [ width/(N-1) * (1- 1/zoomFactor) ]
  // yOffset = tiltStage * [ height/(N-1) * (1 - 1/zoomFactor) ]
  // xLength = width / zoomFactor
  // yLength = height / zoomFactor

  // set zoom factor, use to calculate width and height
  float zoomFactor = 1.0 + zoomStage * 1.0;
  uint16_t xLength = floor(1600.0 / zoomFactor);
  uint16_t yLength = floor(1200.0 / zoomFactor);
  
  // re-use zoomFactor for offset calculation (common factor of 1-1/zoomFactor) 
  zoomFactor = 1.0 - 1.0 / zoomFactor;
  uint16_t xOffset = round(zoomFactor * 177.78 * (float) panStage);
  uint16_t yOffset = round(zoomFactor * 133.33 * (float) tiltStage);

  settings->set_res_raw(settings, 0, 0, 0, 0, xOffset, yOffset, xLength, yLength, xLength, yLength, true, true);
}