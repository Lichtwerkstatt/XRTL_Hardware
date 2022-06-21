// analog of map but accepts floats
float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

enum componentError {
  deserialize_failed,
  field_is_null,
  wrong_type,
  out_of_bounds,
  unknown_key,
  disconnected,
  is_busy
};

// signal unexpected behavior to the server
void sendError(componentError err, String msg){
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  payload.add("error");
  JsonObject parameters = payload.createNestedObject();
  parameters["errnr"] = err;
  parameters["errmsg"] = msg;

  String output;
  serializeJson(payload, output);
  socketIO.sendEVENT(output);
  Serial.print("error reported: ");
  Serial.println(output);
}

// report the state of all connected hardware to the server
void reportState() {
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  payload.add("status");
  JsonObject parameters = payload.createNestedObject();
  parameters["componentId"] = settings.componentID;
  JsonObject state = parameters.createNestedObject("status");
  state["busy"] = busyState;
  
  #if ISCAMERA
    state["streaming"] = streamRunning;
  #endif
  
  #if STEPPERCOUNT > 0
    if ((settings.stepperOneName != 0) and (strcmp(settings.stepperOneName.c_str(),"null") !=0) ) {
      JsonObject stepperFieldOne = state.createNestedObject(settings.stepperOneName);
      stepperFieldOne["absolute"] = stepperOne.currentPosition();
      stepperFieldOne["relative"] = mapFloat(stepperOne.currentPosition(), settings.stepperOneMin, settings.stepperOneMax, 0, 100);
    }
  #endif
  #if STEPPERCOUNT > 1
    if ((settings.stepperTwoName != 0) and (strcmp(settings.stepperTwoName.c_str(),"null") !=0) ) {
      JsonObject stepperFieldTwo = state.createNestedObject(settings.stepperTwoName);
      stepperFieldTwo["absolute"] = stepperTwo.currentPosition();
      stepperFieldTwo["relative"] = mapFloat(stepperTwo.currentPosition(), settings.stepperTwoMin, settings.stepperTwoMax, 0, 100);
    }
  #endif
  
  #if SERVOCOUNT > 0
    if ((settings.servoOneName != 0) and (strcmp(settings.servoOneName.c_str(),"null") !=0) ) {
      JsonObject servoFieldOne = state.createNestedObject(settings.servoOneName);
      servoFieldOne["absolute"] = map(servoOne.read(),0,180,settings.servoOneMinAngle,settings.servoOneMaxAngle);
      servoFieldOne["relative"] = mapFloat(servoOne.read(),0,180,0,100);
    }
  #endif
  #if SERVOCOUNT > 1
    if ((settings.servoTwoName != 0) and (strcmp(settings.servoTwoName.c_str(),"null") !=0) ) {
      JsonObject servoFieldTwo = state.createNestedObject(settings.servoTwoName);
      servoFieldTwo["absolute"] =  map(servoTwo.read(),0,180,settings.servoTwoMinAngle,settings.servoTwoMaxAngle);
      servoFieldTwo["relative"] = mapFloat(servoTwo.read(),0,180,0,100);
    }
  #endif

  #if RELAISCOUNT > 0
    if ((settings.relaisOneName != 0) and (strcmp(settings.relaisOneName.c_str(),"null") !=0) ) {
      state[settings.relaisOneName] = relaisOne.relaisState;
    }
  #endif
  #if RELAISCOUNT > 1
    if ((settings.relaisTwoName != 0) and (strcmp(settings.relaisTwoName.c_str(),"null") !=0) ) {
      state[settings.relaisTwoName] = relaisTwo.relaisState;
    }
  #endif
  
  String output;
  serializeJson(payload, output);
  socketIO.sendEVENT(output);
  Serial.print("report sent: ");
  Serial.println(output);
}

#if STEPPERCOUNT > 0
  void moveStepperOne(JsonObject &command) {
    auto val = command["val"];
    if ( !( val.is<int>() or val.is<float>() ) )  {// TO DO: implement float
      sendError(wrong_type, "<val> is missing or no float or integer but was expected to be");
      return;
    }

    //passed availability checks, prepare for movement
    busyState = true;
    wasRunning = true;
    stepperOne.enableOutputs();
    int target;

    // control by slider
    if (settings.stepperOneSlider) {
      if ( (val.as<int>() > 100) or (val.as<int>() < 0) ) {
        sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
      }
      target = map(constrain(val.as<int>(), 0, 100), 0, 100, settings.stepperOneMin, settings.stepperOneMax);
    }

    // control by steps
    else {
      stepperOne.move(val.as<int>());
      target = stepperOne.targetPosition();
      // check if boundaries would be violated:
      if ( (target < settings.stepperOneMin) or (target > settings.stepperOneMax) ) {
        target = constrain(target, settings.stepperOneMin, settings.stepperOneMax);
        sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
      }
    }
      
    stepperOne.moveTo(target);
      
    #if HASINFOLED
      infoLED.pulse(0, 40, 100);
    #endif
      
    reportState(); 
    return;
  }
#endif

#if STEPPERCOUNT > 1
  void moveStepperTwo(JsonObject &command) {
    auto val = command["val"];
    if ( !( val.is<int>() or val.is<float>() ) )  {// TO DO: implement float
      sendError(wrong_type, "<val> is missing or no float or integer but was expected to be");
      return;
    }

    // passed availability checks, prepare for movement
    busyState = true;
    wasRunning = true;
    stepperTwo.enableOutputs();
    int target;

    // control as slider
    if (settings.stepperTwoSlider) {
      if ( (val.as<int>() > 100) or (val.as<int>() < 0) ) {
        sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
      }
      target = map(val.as<int>(), 0, 100, settings.stepperTwoMin, settings.stepperTwoMax);
    }

    // control by steps
    else {
      stepperTwo.move(val.as<int>());
      target = stepperTwo.targetPosition();
      if ( (target < settings.stepperTwoMin) or (target > settings.stepperTwoMax) ) {
        target = constrain(target, settings.stepperTwoMin, settings.stepperTwoMax);
        sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
      }
    }
      
    stepperTwo.moveTo(target);
    
    #if HASINFOLED
      infoLED.pulse(0, 40, 100);
    #endif
      
    reportState();
    return;
  }
#endif

#if SERVOCOUNT > 0
  void moveServoOne(JsonObject &command) {
    auto val = command["val"];
    if ( !( val.is<int>() or val.is<float>() ) )  {// TO DO: implement float
      sendError(wrong_type, "<val> is missing or no float or integer but was expected to be");
      return;
    }
    Serial.printf("found value: %i\n", val.as<int>());
    int target = val.as<int>();

    // check boundaries
    if ( (target < settings.servoOneMinAngle) or (target > settings.servoOneMaxAngle) ) {
      target = constrain(target, settings.servoOneMinAngle, settings.servoOneMaxAngle);
      sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
    }

    // passed all checks, map to target interval and move servo
    servoOne.write(map(target,settings.servoOneMinAngle,settings.servoOneMaxAngle,0,180));
    reportState();
    return;
  }
#endif

#if SERVOCOUNT > 1
  void moveServoTwo(JsonObject &command) {
    auto val = command["val"];
    if ( !( val.is<int>() or val.is<float>() ) )  {// TO DO: implement float
      sendError(wrong_type, "<val> is missing or no float or integer but was expected to be");
      return;
    }
    Serial.printf("found value: %i\n", val.as<int>());
    int target = val.as<int>();

    // check boundaries
    if ( (target < settings.servoTwoMinAngle) or (target > settings.servoTwoMaxAngle) ) {
      target = constrain(target, settings.servoTwoMinAngle, settings.servoTwoMaxAngle);
      sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
    }

    // passed all checks, map to target interval and move servo
    servoTwo.write(map(target,settings.servoTwoMinAngle,settings.servoTwoMaxAngle,0,180));
    reportState();
    return;
  }
#endif

#if RELAISCOUNT > 0
  
  void moveRelayOne(JsonObject &command) {
  // safety check: socket still connected?
    if (!socketIO.isConnected()) {
      Serial.println("[WARNING] refused to switch relay: no connection to server");
      return;
    }

    auto val = command["val"];
    if (!val.is<bool>()) {
      if (val.is<int>()) { // check if 0 or 1 is used instead
        if ( (val != 1) and (val != 0) ) {
          sendError(wrong_type,"<val> is no bool but was expected to be");
          return;
        }
      }
      else {
        sendError(wrong_type,"<val> is no bool but was expected to be");
        return;
      }
    }

    // all checks passed, switch relay:
    relaisOne.toggle(val);
    reportState();
    return;
  }
#endif

#if RELAISCOUNT > 1
  void moveRelayTwo(JsonObject &command) {
  // safety check: socket still connected?
    if (!socketIO.isConnected()) {
      Serial.println("[WARNING] refused to switch relay: no connection to server");
      return;
    }
      
    auto val = command["val"];
    if (!val.is<bool>()) {
      if (val.is<int>()) { // check if 0 or 1 is used instead
        if ( (val != 1) and (val != 0) ) {
          sendError(wrong_type,"<val> is no bool but was expected to be");
          return;
        }
      }
      else {
        sendError(wrong_type,"<val> is no bool but was expected to be");
        return;
      }
    }

    // all checks passed, switch relay:
    relaisTwo.toggle(val);
    reportState();
    return;
  }
#endif

#if ISCAMERA
  // send frame if available
  void startStreaming() {
    strcpy(binaryLeadFrame, "451-[\"pic\",{\"componentId\":\"");
    strcat(binaryLeadFrame, settings.componentID.c_str());
    strcat(binaryLeadFrame,"\",\"image\":{\"_placeholder\":true,\"num\":0}}]");
    streamRunning = true;
    reportState();
    lastFrameTime = esp_timer_get_time();
  }

  // stop sending frames
  void stopStreaming() {
    streamRunning = false;
    reportState();
  }

  //set maximum frame rate, might be lower in reality
  void setMaxFrameRate(float frameRate) {
    frameIntervalCap = 1000000 / frameRate;
    Serial.printf("frame interval cap set to %i µs\n", frameIntervalCap);
  }
  
  void setFrameSize(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();
    Serial.println("trying to change frame size");
    String val = command["val"];  // to do: check for type
    if (strcmp(val.c_str(),"UXGA") == 0) {
      s->set_framesize(s, FRAMESIZE_UXGA);
      Serial.println("changed frame size to UXGA");// 1600 x 1200 px
      return;
    }
    else if (strcmp(val.c_str(),"QVGA") == 0) {
      s->set_framesize(s, FRAMESIZE_QVGA);
      Serial.println("changed frame size to QVGA");// 320 x 240 px
      return;
    }
    else if (strcmp(val.c_str(),"CIF") == 0) {
      s->set_framesize(s, FRAMESIZE_CIF);
      Serial.println("changed frame size to CIF");// 352 x 288 px
      return;
    }
    else if (strcmp(val.c_str(),"VGA") == 0) {
      s->set_framesize(s, FRAMESIZE_VGA);
      Serial.println("changed frame size to VGA");// 640 x 480 px
      return;
    }
    else if (strcmp(val.c_str(),"SVGA") == 0) {
      s->set_framesize(s, FRAMESIZE_SVGA);
      Serial.println("changed frame size to SVGA");// 800 x 600 px
      return;
    }
    else if (strcmp(val.c_str(),"XGA") == 0) {
      s->set_framesize(s, FRAMESIZE_XGA);
      Serial.println("changed frame size to XGA");// 1024 x 768 px
      return;
    }
    else if (strcmp(val.c_str(),"SXGA") == 0) {
      s->set_framesize(s, FRAMESIZE_SXGA);
      Serial.println("changed frame size to SXGA");// 1280 x 1024 px
      return;
    }
    else {
      sendError(unknown_key,"requested frame size unknown, complete list: UXGA, QVGA, CIF, VGA, SVGA, XGA, SXGA");
      return;
    }
  }

  void setWindow(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();
      
    auto val = command["xOffset"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<xOffset> is no int but was expected to be");
      return;
    }
    int xOffset = val.as<int>();
    if ( (xOffset < 0) or (xOffset > 1600) ) {
      xOffset = constrain(xOffset, 0, 1600);
      sendError(out_of_bounds,"<xOffset> was out of bounds and has been constrained to (0, 1600)");
    }

    val = command["yOffset"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<yOffset> is no int but was expected to be");
      return;
    }
    int yOffset = val.as<int>();
    if ( (yOffset < 0) or (yOffset > 1200) ) {
      yOffset = constrain(yOffset, 0 , 1200);
      sendError(out_of_bounds,"<yOffset> was out of bound and has been constrained to (0, 1200)");
    }

    val = command["xLength"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<xLength> is no int but was expected to be");
      return;
    }
    int xLength = val.as<int>();
    if ( (xLength < 0) or (xLength > 1600 - xOffset) ) {
      xLength = constrain(xLength, 0, 1600 - xOffset);
      sendError(out_of_bounds,"<xLength> was out of bounds and has been constrained to (0, 1600 - xOffset)");
    }
    val = command["yLength"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<yLength> is no int but was expected to be");
      return;
    }
    int yLength = val.as<int>();
    if ( (yLength < 0) or (yLength > 1200 - yOffset) ) {
      yLength = constrain(yLength, 0, 1200 - yOffset);
      sendError(out_of_bounds,"<yLength> was out of bounds and has been constrained to (0, 1200 - yOffset)");
    }
      
    setWindowRaw(0, xOffset, yOffset, xLength, yLength);
    Serial.printf("changed window to origin (%i, %i) and length (%i, %i)\n", xOffset, yOffset, xLength, yLength);
    return;
  }

  void setQuality(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();
     
    auto val = command["val"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<val> is no int but was expected to be");
      return;
    }
      
    int quality = val.as<int>();
    if ( (quality < 10) or (quality > 63) ) {
      quality = constrain(quality,10,63);
      sendError(out_of_bounds,"<quality> was out of bounds and has been constrained to (10, 63)");
    }
      
    s->set_quality(s,quality);
    Serial.printf("changed jpeg quality to %i\n", quality);
    return;
  }

  void setBrightness(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<val> is no int but was expected to be");
      return;
    }
      
    int brightness = val.as<int>();
    if ( (brightness < -2) or (brightness > 2) ){
      brightness = constrain(brightness,-2,2);
      sendError(out_of_bounds,"<brightness> was out of bound and has been constrained to (-2,2)");
    }
      
    s->set_brightness(s, brightness);
    Serial.printf("set brightness to %i\n", brightness);
    return;
  }

  void setContrast(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<val> is no int but was expected to be");
      return;
    }
  
    int contrast = val.as<int>();
    if ( (contrast < -2) or (contrast > 2) ) {
      contrast = constrain(contrast,-2,2); // to do: check for type
      sendError(out_of_bounds,"<contrast> was out of bound and has been constrained to (-2,2)");
    }
      
    s->set_contrast(s,val);
    Serial.printf("set contrast to %i\n",contrast);
    return;
  }

  void setSaturation(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<val> is no int but was expected to be");
      return;
    }

    int saturation = val.as<int>();
    if ( (saturation < -2) or (saturation > 2) ) {
      saturation = constrain(saturation,-2,2);
      sendError(out_of_bounds,"<saturation> was out of bound and has been constrained to (-2,2)");
    }
    s->set_saturation(s,val);
    Serial.printf("set saturation to %i\n",saturation);
    return;
  }

  void setFrameRate(JsonObject &command) {
    auto val = command["val"];
    if ( !((val.is<float>()) or (val.is<int>())) ) {
      sendError(wrong_type,"<val> is no float or int but was expected to be");
      return;
    }

    float frameRate = val.as<float>();
    if ( (frameRate <= 0) or (frameRate > 120) ) {
      frameRate = constrain(frameRate, 0.0001, 120);
      sendError(out_of_bounds,"<frame rate> was out of bound and has been constrained to (0.0001,120)");
    }
    setMaxFrameRate(frameRate);
    Serial.printf("set maximum frame rate to %f fps\n", frameRate);
    return;
  }

  void setGain(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();
    auto val = command["val"];

    // automatic control via string "auto"
    if (val.is<String>()) {
      if (strcmp(val, "auto") == 0) {
        s->set_gain_ctrl(s,1);
        Serial.println("auto gain on");
      }
      else {
        sendError(out_of_bounds,"only <auto> allowed if <gain> is a string");
      }
      return;
    }

    // manual control via integer
    else if (val.is<int>()) {
      int gain = val.as<int>();
      if ( (gain < 0) or (gain > 30) ) {
        gain = constrain(gain,0,30);
        sendError(out_of_bounds,"<gain> was out of bounds and has been constrained to (0,30)");
      }
      
      s->set_gain_ctrl(s,0);
      s->set_agc_gain(s,gain);
      Serial.printf("auto gain off, gain set to %i\n",gain);
      return;
    }

    else {
      sendError(wrong_type,"<val> is no String or int but was expected to be");
      return;
    }
  }

  void setExposure(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();
    auto val = command["val"];

    //
    if (val.is<String>()) {
      if (strcmp(val, "auto") == 0) {
        s->set_exposure_ctrl(s,1);
        Serial.println("auto exposure on");
      }
      else {
        sendError(out_of_bounds,"only <auto> allowed if <exposure> is a string");
      }
      return;
    }

    // 
    else if (val.is<int>()) {
      int exposure = val.as<int>();
      if ( (exposure < 0) or (exposure > 1200) ) {
        exposure = constrain(exposure, 0, 1200);
        sendError(out_of_bounds,"<exposure> was out of bounds and has been constrained to (0, 1200)"); 
      }
      
      s->set_gain_ctrl(s,0);
      s->set_aec_value(s,exposure);
      Serial.printf("auto exposure off, set exposure to %i\n",exposure);
      return;
    }

    else {
      sendError(wrong_type,"<val> is no String or int but was expected to be");
      return;
    }
  }

  void setSharpness(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<int>()) {
      sendError(wrong_type,"<val> is no int but was expected to be");
      return;
    }
      
    int sharpness = val.as<int>();
    if ( (sharpness < -2) or (sharpness > 2) ) {
      sharpness = constrain(sharpness,-2,2);
      sendError(out_of_bounds,"<sharpness> was out of bounds and has been constrained to (-2,2)");
    }
    s->set_sharpness(s,val);
    Serial.printf("set sharpness to %i\n", sharpness);
    return;
  }

  void setGrayFilter(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<bool>()) {
      if (val.is<int>()) { // check if 0 or 1 is used instead
        if ( (val != 1) and (val != 0) ) {
          sendError(wrong_type,"<val> is no bool but was expected to be");
          return;
        }
      }
      else {
        sendError(wrong_type,"<val> is no bool but was expected to be");
        return;
      }
    }

    bool gray = val.as<bool>(); // also TO DO: check pixelformat for native gray -- save data! 
    if (gray) {
      s->set_special_effect(s, 2);
      Serial.println("gray filter on");
    }
      
    else if (!gray) {
      s->set_special_effect(s,0);
      Serial.println("gray filter off");
    }
  }

  
  void mirrorImage(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<bool>()) {
      if (val.is<int>()) { // check if 0 or 1 is used instead
        if ( (val != 1) and (val != 0) ) {
          sendError(wrong_type,"<val> is no bool but was expected to be");
          return;
        }
      }
      else {
        sendError(wrong_type,"<val> is no bool but was expected to be");
        return;
      }
    }

    s->set_hmirror(s, val.as<bool>());
  }

  void flipImage(JsonObject &command) {
    // get address of camera settings
    sensor_t * s = esp_camera_sensor_get();

    auto val = command["val"];
    if (!val.is<bool>()) {
      if (val.is<int>()) { // check if 0 or 1 is used instead
        if ( (val != 1) and (val != 0) ) {
          sendError(wrong_type,"<val> is no bool but was expected to be");
          return;
        }
      }
      else {
        sendError(wrong_type,"<val> is no bool but was expected to be");
        return;
      }
    }

    s->set_vflip(s, val.as<bool>());
  }
  
#endif

// command handler for extended command structure
void eventCommandNested(JsonObject &command) {
  Serial.println("identified nested command structure");
  auto controlId = command["controlId"];
  String control = controlId.as<String>();

  if ( (strcmp(control.c_str(), "null") == 0) or (control == NULL) ) {
    sendError(field_is_null,"<controlId> is null");
    return;
  }
  Serial.printf("identified controlID: %s\n", control);
  
  if (!controlId.is<String>()) {
    sendError(wrong_type, "<controlId> is no string"  );
    return;
  }
        
  #if STEPPERCOUNT > 0
    else if (strcmp(control.c_str(),settings.stepperOneName.c_str()) == 0) {
      moveStepperOne(command);
      return;
    }
  #endif
  
  #if STEPPERCOUNT > 1
    else if (strcmp(control.c_str(),settings.stepperTwoName.c_str()) == 0) {
      moveStepperTwo(command);
      return;
    }
  #endif
        
  #if SERVOCOUNT > 0
    else if (strcmp(control.c_str(),settings.servoOneName.c_str()) == 0) {
      moveServoOne(command);
      return;
    }
  #endif
  
  #if SERVOCOUNT > 1
    else if (strcmp(control.c_str(),settings.servoTwoName.c_str()) == 0) {
      moveServoTwo(command);
      return;
    }
  #endif
        
  #if RELAISCOUNT > 0
    else if (strcmp(control.c_str(),settings.relaisOneName.c_str()) == 0) {
      moveRelayOne(command);
      return;
    }
  #endif
  
  #if RELAISCOUNT > 1
    else if (strcmp(control.c_str(),settings.relaisTwoName.c_str()) == 0) {
      moveRelayTwo(command);
      return;
    }
  #endif

  #if ISCAMERA
    // change frame size
    else if (strcmp(control.c_str(),"frame size") == 0) {
      setFrameSize(command);
      return;
    }

    // window camera output to avoid sending unneeded data
    else if (strcmp(control.c_str(), "window") == 0) {
      setWindow(command);
      return;
    }

    // jpeg quality: 10 = best
    else if (strcmp(control.c_str(), "quality") == 0) {
      setQuality(command);
      return;
    }

    // brightness
    else if (strcmp(control.c_str(), "brightness") == 0) {
      setBrightness(command);
      return;
    }

    // contrast
    else if (strcmp(control.c_str(), "contrast") == 0) {
      setContrast(command);
      return;
    }

    // saturation
    else if (strcmp(control.c_str(), "saturation") == 0) {
      setSaturation(command);
      return;
    }

    // upper limit for frame rate, actual rate might be lower due to network etc.
    else if (strcmp(control.c_str(), "frame rate") == 0) {
      setFrameRate(command);
      return;
    }

    // automatic gain control
    else if (strcmp(control.c_str(), "gain") == 0) {
      setGain(command);
      return;
    }

    // 
    else if (strcmp(control.c_str(), "exposure") == 0) {
      setExposure(command);
      return;
    }

    // sharpness
    else if (strcmp(control.c_str(), "sharpness") == 0) {
      setSharpness(command);
      return;
    }

    // flip image
    else if (strcmp(control.c_str(), "flip") == 0) {
      flipImage(command);
      return;
    }

    // mirror image
    else if (strcmp(control.c_str(), "mirror") == 0) {
      mirrorImage(command);
      return;
    }

    // gray filter
    else if (strcmp(control.c_str(), "gray") == 0) {
      setGrayFilter(command);
      return;      
    }
    
  #endif

  // no controlId could be found
  else {
    String message = "unknown controlId: <";
    message += control;
    message += ">";
    sendError(unknown_key,message);
  }
}


// command handler for simple command structure
void eventCommandString(String &command) {
  Serial.printf("identified simple command: %s\n", command.c_str());

  // validity check
  if ( (strcmp(command.c_str(),"null") == 0) or (command == NULL) ) {
    sendError(field_is_null,"<command> is null");
    return;
  }

  // status request
  else if (strcmp(command.c_str(),"getStatus") == 0) {
   reportState();
  }
  
  #if STEPPERCOUNT > 0
    // abort rotation
    else if (strcmp(command.c_str(),"stop") == 0) {

      #if STEPPERCOUNT == 1
        stepperOne.stop();//issue stop

        // motor needs to decelerate
        while (stepperOne.isRunning()) {
          stepperOne.run();
        }
        stepperOne.disableOutputs();
        
      #endif

      #if STEPPERCOUNT == 2
        stepperOne.stop();
        stepperTwo.stop();//stop both motors

        // motors need to decelerate
        while ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {
          stepperOne.run();
          stepperTwo.run();
        }
        stepperOne.disableOutputs();
        stepperTwo.disableOutputs();
        
      #endif
    }
    
  #endif

  // restart ESP
  else if (strcmp(command.c_str(),"reset") == 0) {
    Serial.println("received reset command");
    Serial.println("disconnecting now");
          
    #if STEPPERCOUNT == 1
      stepperOne.stop();//make sure no motors are running
      while (stepperOne.isRunning()) {//motor needs to decelerate
        stepperOne.run();
      }
      stepperOne.disableOutputs();
    #endif

    #if STEPPERCOUNT == 2
      stepperOne.stop();
      stepperTwo.stop();
      while ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {
        stepperOne.run();
        stepperTwo.run();
      }
      stepperOne.disableOutputs();
      stepperTwo.disableOutputs();
    #endif

    #if ISCAMERA
      stopStreaming();
    #endif
          
    saveSettings();// remember current stepper positions
    socketIO.disconnect();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    ESP.restart();
  }

  // use current positions for calibration
  else if (strcmp(command.c_str(),"init") == 0) {
    Serial.println("received init command");
    
    #if STEPPERCOUNT > 0
      Serial.printf("stepper one position: %i steps\n", stepperOne.currentPosition());
      // TO DO: set stepperOne.currentPosition() as init position?
    #endif
    
    #if STEPPERCOUNT > 1
      Serial.printf("stepper two position: %i steps\n", stepperTwo.currentPosition());
      // TO DO: set stepperTwo.currentPosition() as init position?
    #endif
    
    #if SERVOCOUNT > 0
      settings.servoOneInit = map(servoOne.read(),0,180,settings.servoOneMinAngle,settings.servoOneMaxAngle);
    #endif
    
    #if SERVOCOUNT > 1
      settings.servoTwoInit = map(servoTwo.read(),0,180,settings.servoTwoMinAngle,settings.servoTwoMaxAngle);
    #endif
    
    saveSettings();//
  }

  // camera stream control
  #if ISCAMERA
  
    else if (strcmp(command.c_str(),"startStreaming") == 0) {
      startStreaming();
    }

    else if (strcmp(command.c_str(),"stopStreaming") == 0) {
      stopStreaming();
    }
    
  #endif

  // none of the commands was found
  else {
    String message = "unknown command: <";
    message += command;
    message += ">";
    sendError(unknown_key, message);
  }
}


// analyze and handle incoming events (both socket.io and serial interface)
void eventHandler(uint8_t * eventPayload, size_t eventLength) {
  // separate ID
  char * sptr = NULL;
  int id = strtol((char*) eventPayload, &sptr, 10);
  Serial.printf("[IOc] got event: %s     (id: %d)\n", eventPayload, id);

  // analyze payload
  DynamicJsonDocument incomingEvent(1024);
  DeserializationError error = deserializeJson(incomingEvent, eventPayload, eventLength);
  if (error) {
    char message[100] = "Deserialization failed: ";
    strcat(message,error.c_str());
    sendError(deserialize_failed, message);
    return;
  }

  auto eventField = incomingEvent[0];
  String eventName = eventField.as<String>();

  // check validity of event (also gets triggered for exclusive serial commands)
  if ( (strcmp(eventName.c_str(),"null") == 0) or (eventName == NULL) ) {

    // serial setup
    if (strcmp((char *)eventPayload, "serial setup") == 0) {
      setViaSerial();
      return;
    }

    // serial device info request
    else if (strcmp((char *)eventPayload, "identify") == 0) {
      
      #if STEPPERCOUNT > 0
        Serial.printf("I have a stepper on pins %i, %i, %i and %i\n", STEPPERONEPINA, STEPPERONEPINB, STEPPERONEPINC, STEPPERONEPIND);
      #endif
      
      #if STEPPERCOUNT > 1
        Serial.printf("I have another stepper on pins %i, %i, %i and %i\n", STEPPERTWOPINA, STEPPERTWOPINB, STEPPERTWOPINC, STEPPERTWOPIND);
      #endif
      
      #if SERVOCOUNT > 0
        Serial.printf("I have a servo on pin %i\n", SERVOONEPIN);
      #endif
      
      #if SERVOCOUNT > 1
        Serial.printf("I have another servo on pin %i\n", SERVOTWOPIN);
      #endif
      
      #if RELAISCOUNT > 0 
        Serial.printf("I have a relais on pin %i\n", RELAISONEPIN);
      #endif
      
      #if RELAISCOUNT > 1
        Serial.printf("I have another relais on pin %i\n", RELAISTWOPIN);
      #endif
      
      #if ISCAMERA
        Serial.println("I am a camera");
      #endif
      
      #if HASINFOLED
        Serial.printf("I have an info LED with %i pixel on pin %i\n", INFOLEDNUMBER, INFOLEDPIN);
      #endif
      return;
    }

    else {
      sendError(field_is_null,"<event name> is null");
      return; 
    }
  }

  if (!eventField.is<String>()) {
    sendError(wrong_type,"<event name> is no string");
    return;
  }

  // pic event will be recognized to surpress errors, but not handled
  else if (strcmp(eventName.c_str(),"pic") == 0){
    return;
  }

  // newLog event will be recognized to surpress errors, but not handled
  else if (strcmp(eventName.c_str(),"newLog") == 0) {
    return;
  }

  else if (strcmp(eventName.c_str(),"status") == 0) {
    return;
  }

  // change stuff on the component
  else if (strcmp(eventName.c_str(),"command") == 0) {
    Serial.println("identified command event");
    
    // store and analyze input
    JsonObject receivedPayload = incomingEvent[1];
    auto componentId = receivedPayload["componentId"];

    // check validity
    if (!componentId.is<String>()) {

      //if field is missing it might be parsed as 0 or "null"
      if ((componentId == NULL) or (strcmp(componentId.as<String>().c_str(),"null") == 0)) {
        sendError(field_is_null,"<componentId> is null");
        return;
      }
      
      else {
        sendError(wrong_type, "<componentId> is no string"  );
        return;
      }
    }
    String component = componentId;

    // componentId is a string, but is it empty?
    if ( (component == 0) or (strcmp(component.c_str(),"null") == 0) ) {
      sendError(field_is_null,"<componentId> is null");
      return;
    }
    
    Serial.printf("identified componentID: %s\n", component.c_str());
    
    // act only when this comoponent is involved: componentId, componentAlias or * (wildcard)
    if ( (strcmp(component.c_str(),settings.componentID.c_str()) == 0) or (strcmp(component.c_str(),"*") == 0) or (strcmp(component.c_str(),settings.componentAlias.c_str()) == 0) ) {
      Serial.printf("componentId recognized: %s (that's me)\n", component.c_str());
      auto command = receivedPayload["command"];

      if ( (command == 0) or (strcmp(command.as<String>().c_str(),"null") == 0) ) {
        sendError(field_is_null,"<command> is null");
        return;
      }
      
      // check for simple or extended command structure
      else if (command.is<JsonObject>()) {
        if (busyState) {//reject only for nested commands to allow for emergency stops
          sendError(is_busy,"component currently busy");
          return;
        }
        JsonObject command = receivedPayload["command"];
        eventCommandNested(command);
      }
      
      else if (command.is<String>()) {
        String command = receivedPayload["command"];
        eventCommandString(command);
      }
      
      else {
        sendError(wrong_type,"<command> is no String or JsonObject");
        return;
      }
      
    }
  }

  else {
    String message = "unknown event: <";
    message += eventName;
    message += ">";
    sendError(unknown_key,message);
  }
}

// handle socket.io events
void socketIOEvent(socketIOmessageType_t type, uint8_t * payload, size_t length) {
  switch(type) {
    case sIOtype_DISCONNECT:
      Serial.printf("[IOc] Disconnected!\n");
      #if HASINFOLED
        infoLED.hsv(0, 255, 40);
        if (WiFi.status() != WL_CONNECTED) {
          infoLED.cycle(100, false);
        }
        else if (WiFi.status() == WL_CONNECTED) {
          infoLED.pulse(0, 40, 100);
        }
      #endif
      #if ISCAMERA
        streamRunning = false;
      #endif
      #if RELAISCOUNT > 0
        relaisOne.stop();
      #endif
      #if RELAISCOUNT > 1
        relaisTwo.stop();
      #endif
      break;
    case sIOtype_CONNECT: {
      Serial.printf("[IOc] Connected to URL: %s\n", payload);

      // join default namespace
      socketIO.send(sIOtype_CONNECT, "/");
      // wait between 100 and 300 ms
      int64_t waitTime = esp_timer_get_time() + random(100000,300000);
      while ( esp_timer_get_time() < waitTime) {
        yield();
      }
      reportState();
      #if HASINFOLED
        infoLED.hsv(20000, 255, 40);
      #endif
      }
      break;
    case sIOtype_EVENT:
      eventHandler(payload, length);
      break;
    case sIOtype_ACK:
      Serial.printf("[IOc] get ack: %u\n", length);
      break;
    case sIOtype_ERROR:
      Serial.printf("[IOc] get error: %u\n", length);
      break;
    case sIOtype_BINARY_EVENT:
      Serial.printf("[IOc] get binary: %u\n", length);
      break;
    case sIOtype_BINARY_ACK:
      Serial.printf("[IOc] get binary ack: %u\n", length);
      break;
  }
}

// everything that needs to be run once on startup
void XRTLsetup(){
  Serial.begin(115200);
  Serial.println("starting setup");
  loadSettings();
  Serial.println("setting up WiFi");

  eventIdDisconnected = WiFi.onEvent(WiFiStationDisconnected, ARDUINO_EVENT_WIFI_STA_DISCONNECTED);//SYSTEM_EVENT_STA_DISCONNECTED for older version
  eventIdGotIP = WiFi.onEvent(WiFiStationGotIP, ARDUINO_EVENT_WIFI_STA_GOT_IP);//SYSTEM_EVENT_STA_GOT_IP for older version
  WiFi.setHostname(settings.componentID.c_str());
  WiFi.mode(WIFI_STA);
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());

  #if HASINFOLED
    infoLED.begin();
    infoLED.hsv(0,255,40);
    infoLED.cycle(100,false);
    infoLED.start();
  #endif

  #if STEPPERCOUNT > 0
    if ((settings.stepperOneSpeed == 0) or (settings.stepperOneSpeed == 0)) {
      Serial.println("[WARNING] stepper one not initialized. Set speed and accelaration and reboot");
    }
    else {
      stepperOne.setMaxSpeed(settings.stepperOneSpeed);
      stepperOne.setAcceleration(settings.stepperOneAcc);
      if (settings.stepperOneInit != 0) {
        stepperOne.move(settings.stepperOneInit);
        busyState = true;
        wasRunning = true;
        isInitialized = false;
      }
    }
  #endif
  
  #if STEPPERCOUNT > 1
    if ((settings.stepperTwoSpeed == 0) or (settings.stepperTwoSpeed == 0)) {
      Serial.println("[WARNING] stepper two not initialized. Set speed and acceleration and reboot");
    }
    else {
      stepperTwo.setMaxSpeed(settings.stepperTwoSpeed);
      stepperTwo.setAcceleration(settings.stepperTwoAcc);
      if (settings.stepperTwoInit != 0) {
        stepperTwo.move(settings.stepperTwoInit);
        busyState = true;
        wasRunning = true;
        isInitialized = false;
      }
    }
  #endif

  #if SERVOCOUNT > 0
    #if ISCAMERA
      trashOne.attach(2,1000,2000);
      trashTwo.attach(13,1000,2000);
    #endif
    if ((settings.servoOneFreq == 0) or (settings.servoOneMinDuty == 0) or (settings.servoOneMaxDuty == 0)) {
      Serial.println("[WARNING] servo one not initialized. Set frequency and duty range and reboot");
    }
    else {
      servoOne.setPeriodHertz(settings.servoOneFreq);
      servoOne.attach(SERVOONEPIN,settings.servoOneMinDuty,settings.servoOneMaxDuty);
      servoOne.write(map(constrain(settings.servoOneInit,settings.servoOneMinAngle,settings.servoOneMaxAngle),settings.servoOneMinAngle,settings.servoOneMaxAngle,0,180));
    }
  #endif
  
  #if SERVOCOUNT > 1
    if ((settings.servoTwoFreq == 0) or (settings.servoTwoMinDuty == 0) or (settings.servoTwoMaxDuty == 0)) {
      Serial.println("[WARNING] servo two not initialized. Set frequency and duty range and reboot");
    }
    else {
      servoTwo.setPeriodHertz(settings.servoTwoFreq);
      servoTwo.attach(SERVOTWOPIN,settings.servoTwoMinDuty,settings.servoTwoMaxDuty);
      servoTwo.write(map(constrain(settings.servoTwoInit,settings.servoTwoMinAngle,settings.servoTwoMaxAngle),settings.servoTwoMinAngle,settings.servoTwoMaxAngle,0,180));
    }
  #endif

  #if RELAISCOUNT > 0
    if ( (strcmp(settings.relaisOneName.c_str(),"null") == 0) or (settings.relaisOneName == 0) ) {
      Serial.println("[WARNING] relais one not initialized. Set controlId and reboot");
    }
    relaisOne.attach(RELAISONEPIN);
  #endif
  
  #if RELAISCOUNT > 1
    if ( (strcmp(settings.relaisTwoName.c_str(),"null") == 0) or (settings.relaisTwoName == 0) ) {
      Serial.println("[WARNING] relais two not initialized. Set controlId and reboot");
    }
    relaisTwo.attach(RELAISTWOPIN);
  #endif

  #if ISCAMERA
    init_camera();
    sensor_t * s = esp_camera_sensor_get();
    s->set_contrast(s, -2);//set high contrast
    setMaxFrameRate(15);
  #endif

  socketIO.begin(settings.socketIP.c_str(), settings.socketPort, settings.socketURL.c_str());
  socketIO.onEvent(socketIOEvent);
  
  Serial.println("setup done");
}


//everything that needs to be run repeatedly
void XRTLloop() {
  socketIO.loop();
  
  #if ISCAMERA
    // send a frame if streaming is enabled and enough time elapsed since the last frame
    if ((streamRunning) && (WiFi.status() == WL_CONNECTED) && (socketIO.isConnected())) {
      currentTime = esp_timer_get_time();
      if (currentTime - lastFrameTime > frameIntervalCap) {
        camera_fb_t *fb = esp_camera_fb_get();
        socketIO.sendBIN(fb->buf,fb->len);
        esp_camera_fb_return(fb);
        Serial.printf("frames per second: %f\n",(float) 1000000 / (currentTime - lastFrameTime) );
        lastFrameTime = currentTime;
      }
    }
  #endif

  #if STEPPERCOUNT == 1
    // check if steps are due and handle initialization for one motor
    if (stepperOne.isRunning()) {
      stepperOne.run();
    }
    else if (wasRunning) {
      stepperOne.disableOutputs();
      busyState = false;
      wasRunning = false;
      #if HASINFOLED
        infoLED.constant();
      #endif
      if (!isInitialized) {
        if (settings.stepperOneInit < 0) {
          stepperOne.setCurrentPosition(settings.stepperOneMin);
        }
        else if (settings.stepperOneInit > 0) {
          stepperOne.setCurrentPosition(settings.stepperOneMax);
        }
        isInitialized = true;
        Serial.println("stepper initialized");
      }
      reportState();
    }
  #endif

  #if STEPPERCOUNT == 2
    // check if steps are due and handle initialization for two motors
    if ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {
      stepperOne.run();
      stepperTwo.run();
    }
    else if (wasRunning) {
      stepperOne.disableOutputs();
      stepperTwo.disableOutputs();
      busyState = false;
      wasRunning = false;
      #if HASINFOLED
        infoLED.constant();
      #endif
      if (!isInitialized) {
        if (settings.stepperOneInit < 0) {
          stepperOne.setCurrentPosition(settings.stepperOneMin);
        }
        else if (settings.stepperOneInit > 0) {
          stepperOne.setCurrentPosition(settings.stepperOneMax);
        }

        if (settings.stepperTwoInit < 0) {
          stepperTwo.setCurrentPosition(settings.stepperTwoMin);
        }
        else if (settings.stepperTwoInit > 0) {
          stepperTwo.setCurrentPosition(settings.stepperTwoMax);
        }
        isInitialized = true;
        Serial.println("stepper initialized");
      }
      reportState();
    }
  #endif

  #if HASINFOLED
    // check whether the infoLED needs to be updated
    infoLED.loop();
  #endif

  // serial interface for debugging, all socket.io commands can be passed via Serial as well
  if ( (Serial) && (Serial.available() > 0) ) {
    String serialEvent=Serial.readStringUntil('\n');
    eventHandler((uint8_t *)serialEvent.c_str(), serialEvent.length());
  }
}
