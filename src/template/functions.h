float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

enum componentError {
  deserialize_failed,
  field_is_null,
  wrong_type,
  out_of_bounds,
  unknown_command,
  disconnected,
  is_busy
};

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
  Serial.print("error sent: ");
  Serial.println(output);
}

void reportState() {
  DynamicJsonDocument outgoingEvent(1024);
  JsonArray payload = outgoingEvent.to<JsonArray>();
  payload.add("status");
  JsonObject parameters = payload.createNestedObject();
  parameters["component"] = settings.componentID;
  JsonObject state = parameters.createNestedObject("status");
  state["busy"] = busyState;
  
  #if ISCAMERA
    state["streaming"] = streamRunning;
  #endif
  
  #if STEPPERCOUNT > 0
    JsonObject stepperFieldOne = state.createNestedObject(settings.stepperOneName);
    stepperFieldOne["absolute"] = stepperOne.currentPosition();
    stepperFieldOne["relative"] = mapFloat(stepperOne.currentPosition(), settings.stepperOneMin, settings.stepperOneMax, 0, 100);
  #endif
  #if STEPPERCOUNT > 1
    JsonObject stepperFieldTwo = state.createNestedObject(settings.stepperTwoName);
    stepperFieldTwo["absolute"] = stepperTwo.currentPosition();
    stepperFieldTwo["relative"] = mapFloat(stepperTwo.currentPosition(), settings.stepperTwoMin, settings.stepperTwoMax, 0, 100);
  #endif
  
  #if SERVOCOUNT > 0
    JsonObject servoFieldOne = state.createNestedObject(settings.servoOneName);
    servoFieldOne["absolute"] = map(servoOne.read(),0,180,settings.servoOneMinAngle,settings.servoOneMaxAngle);
    servoFieldOne["relative"] = mapFloat(servoOne.read(),0,180,0,100);
  #endif
  #if SERVOCOUNT > 1
    JsonObject servoFieldTwo = state.createNestedObject(settings.servoTwoName);
    servoFieldTwo["absolute"] =  map(servoTwo.read(),0,180,settings.servoTwoMinAngle,settings.servoTwoMaxAngle);
    servoFieldTwo["relative"] = mapFloat(servoTwo.read(),0,180,0,100);
  #endif

  #if RELAISCOUNT > 0
    state[settings.relaisOneName] = relaisOne.relaisState;
  #endif
  #if RELAISCOUNT > 1
    state[settings.relaisTwoName] = relaisTwo.relaisState;
  #endif
  
  String output;
  serializeJson(payload, output);
  socketIO.sendEVENT(output);
  Serial.print("report sent: ");
  Serial.println(output);
}

#if ISCAMERA
void startStreaming() {
  strcpy(binaryLeadFrame, "451-[\"pic\",{\"componentId\":\"");
  strcat(binaryLeadFrame, settings.componentID.c_str());
  strcat(binaryLeadFrame,"\",\"image\":{\"_placeholder\":true,\"num\":0}}]");
  streamRunning = true;
  reportState();
  lastFrameTime = esp_timer_get_time();
}

void stopStreaming() {
  streamRunning = false;
  reportState();
}

void setMaxFrameRate(float frameRate) {
  frameIntervalCap = 1000000 / frameRate;
  Serial.printf("frame interval cap set to %i µs\n", frameIntervalCap);
}
#endif

void eventCommandNested(JsonObject command) {
  Serial.println("identified nested command structure");
  auto controlId = command["controlId"];
  if (!controlId.is<String>()) {
    sendError(wrong_type, "<controlId> is no string"  );
    return;
  }
  String control = controlId;
  Serial.printf("identified controlID: %s\n", control);

  if (strcmp(control.c_str(), "null") == 0) {
    sendError(field_is_null,"controlId is null");
    return;
  }
        
  #if STEPPERCOUNT > 0
    if (strcmp(control.c_str(),settings.stepperOneName.c_str()) == 0) {
      auto val = command["val"];
      if (!val.is<int>()) {
        sendError(wrong_type, "<val> is no integer");
        return;
      }
      busyState = true;
      wasRunning = true;
      stepperOne.enableOutputs();
      int target;
      if (settings.stepperOneSlider) {
        if ( (val.as<int>() > 100) or (val.as<int>() < 0) ) {
          sendError(out_of_bounds, "<val is out of bounds, target set to limit");
        }
        target = map(constrain(val.as<int>(), 0, 100), 0, 100, settings.stepperOneMin, settings.stepperOneMax);
      }
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
    }
  #endif
  #if STEPPERCOUNT > 1
    if (strcmp(control.c_str(),settings.stepperTwoName.c_str()) == 0) {
      auto val = command["val"];
      if (!val.is<int>()) {
        sendError(wrong_type, "<val> is no integer but was expected to be");
        return;
      }
      busyState = true;
      wasRunning = true;
      stepperTwo.enableOutputs();
      int target;
      if (settings.stepperTwoSlider) {
        if ( (val.as<int>() > 100) or (val.as<int>() < 0) ) {
          sendError(out_of_bounds, "<val is out of bounds, target set to limit");
        }
        target = map(val.as<int>(), 0, 100, settings.stepperTwoMin, settings.stepperTwoMax);
      }
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
    }
  #endif
        
  #if SERVOCOUNT > 0
    if (strcmp(control.c_str(),settings.servoOneName.c_str()) == 0) {
      auto val = command["val"];
      if (!val.is<int>()) {
        sendError(wrong_type, "<val> is no integer but was expected to be");
        return;
      }
      Serial.printf("found value: %i\n", val.as<int>());
      int target = val.as<int>();
      if ( (target < settings.servoOneMinAngle) or (target > settings.servoOneMaxAngle) ) {
        target = constrain(target, settings.servoOneMinAngle, settings.servoOneMaxAngle);
        sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
      }
      servoOne.write(map(target,settings.servoOneMinAngle,settings.servoOneMaxAngle,0,180));
      reportState();
    }
  #endif
  #if SERVOCOUNT > 1
    if (strcmp(control.c_str(),settings.servoTwoName.c_str()) == 0) {
      auto val = command["val"];
      if (!val.is<int>()) {
        sendError(wrong_type, "<val> is no integer but was expected to be");
        return;
      }
      Serial.printf("found value: %i\n", val.as<int>());
      int target = val.as<int>();
      if ( (target < settings.servoTwoMinAngle) or (target > settings.servoTwoMaxAngle) ) {
        target = constrain(target, settings.servoTwoMinAngle, settings.servoTwoMaxAngle);
        sendError(out_of_bounds, "<val> is out of bounds, target set to limit");
      }
      servoTwo.write(map(target,settings.servoTwoMinAngle,settings.servoTwoMaxAngle,0,180));
      reportState();
    }
  #endif
        
  #if RELAISCOUNT > 0
    if (strcmp(control.c_str(),settings.relaisOneName.c_str()) == 0) {
      if (!socketIO.isConnected()) {
        return;
      }
      auto val = command["val"];
      if (!val.is<bool>()) {
        sendError(wrong_type, "<val> is no boolean but was expected to be");
        return;
      }
      relaisOne.toggle(val);
      reportState();
    }
  #endif
  #if RELAISCOUNT > 1
    if (strcmp(control.c_str(),settings.relaisTwoName.c_str()) == 0) {
      if (!socketIO.isConnected()) {
        return;
      }
      auto val = command["val"];
      if (!val.is<bool>()) {
        sendError(wrong_type, "<val> is no boolean but was expected to be");
        return;
      }
      relaisTwo.toggle(val);
      reportState();
    }
  #endif

  #if ISCAMERA
    sensor_t * s = esp_camera_sensor_get();
    if (strcmp(control.c_str(),"frame size") == 0) {
      Serial.println("trying to change frame size");
      String val = command["val"];  // to do: check for type
      if (strcmp(val.c_str(),"UXGA") == 0) {
        s->set_framesize(s, FRAMESIZE_UXGA);
        Serial.println("changed frame size to UXGA");// 1600 x 1200 px
      }
      else if (strcmp(val.c_str(),"QVGA") == 0) {
        s->set_framesize(s, FRAMESIZE_QVGA);
        Serial.println("changed frame size to QVGA");// 320 x 240 px
      }
      else if (strcmp(val.c_str(),"CIF") == 0) {
        s->set_framesize(s, FRAMESIZE_CIF);
        Serial.println("changed frame size to CIF");// 352 x 288 px
      }
      else if (strcmp(val.c_str(),"VGA") == 0) {
        s->set_framesize(s, FRAMESIZE_VGA);
        Serial.println("changed frame size to VGA");// 640 x 480 px
      }
      else if (strcmp(val.c_str(),"SVGA") == 0) {
        s->set_framesize(s, FRAMESIZE_SVGA);
        Serial.println("changed frame size to SVGA");// 800 x 600 px
      }
      else if (strcmp(val.c_str(),"XGA") == 0) {
        s->set_framesize(s, FRAMESIZE_XGA);
        Serial.println("changed frame size to XGA");// 1024 x 768 px
      }
      else if (strcmp(val.c_str(),"SXGA") == 0) {
        s->set_framesize(s, FRAMESIZE_SXGA);
        Serial.println("changed frame size to SXGA");// 1280 x 1024 px
      }
      else {
        Serial.println("could not identify frame size");
      }
    }

    if (strcmp(control.c_str(), "quality") == 0) {
      int val = constrain(command["val"],10,63); // to do: check for type
      s->set_quality(s,val);
      Serial.printf("changed jpeg quality to %i\n", val);
    }

    if (strcmp(control.c_str(), "brightness") == 0) {
      int val = constrain(command["val"],-2,2); // to do: check for type
      s->set_brightness(s,val);
      Serial.printf("set brightness to %i\n",val);
    }

    if (strcmp(control.c_str(), "contrast") == 0) {
      int val = constrain(command["val"],-2,2); // to do: check for type
      s->set_contrast(s,val);
      Serial.printf("set contrast to %i\n",val);
    }

    if (strcmp(control.c_str(), "saturation") == 0) {
      int val = constrain(command["val"],2,2); // to do: check for type
      s->set_saturation(s,val);
      Serial.printf("set saturation to %i\n",val);
    }

    if (strcmp(control.c_str(), "frame rate") == 0) {
      float val = constrain(command["val"].as<float>(),0,120); // to do: check for type
      setMaxFrameRate(val);
      Serial.printf("set maximum frame rate to %f fps\n", val);
    }

    if (strcmp(control.c_str(), "gain") == 0) {
      auto val = command["val"];
      if (val.is<String>()) {
        if (strcmp(val, "auto") == 0) {
          s->set_gain_ctrl(s,1);
          Serial.println("auto gain on");
        }
      }
      else if (val.is<int>()) {
        //val = constrain(val,0,30);
        s->set_gain_ctrl(s,0);
        s->set_agc_gain(s,constrain(val,0,30));
        Serial.printf("auto gain off, gain set to %i\n",constrain(val,0,30));
      }
    }

    if (strcmp(control.c_str(), "exposure") == 0) {
      auto val = command["val"];
      if (val.is<String>()) {
        if (strcmp(val, "auto") == 0) {
          s->set_exposure_ctrl(s,1);
          Serial.println("auto exposure on");
        }
      }
      else if (val.is<int>()) {
        //val = constrain(val,0,1200);
        s->set_gain_ctrl(s,0);
        s->set_aec_value(s,constrain(val,0,1200));
        Serial.printf("auto exposure off, set exposure to %i\n",constrain(val,0,1200));
      }
    }

    if (strcmp(control.c_str(), "sharpness") == 0) {
      int val = constrain(command["val"],-2,2); // to do: check for type
      s->set_sharpness(s,val);
      Serial.printf("set sharpness to %i\n", val);
    }

    if (strcmp(control.c_str(), "gray") == 0) {
      bool val = command["val"]; // to do: check for type // also TO DO: check pixelformat for native gray -- save data!
      if (val) {
        s->set_special_effect(s, 2);
        Serial.println("gray filter on");
      }
      else if (!val) {
        s->set_special_effect(s,0);
        Serial.println("gray filter off");
      }
    }
  #endif
}

void eventCommandString(String command) {
  Serial.printf("identified simple command: %s\n", command.c_str());
  
  if (strcmp(command.c_str(),"getStatus") == 0) {
   reportState();
  }
  
  #if STEPPERCOUNT > 0
    if (strcmp(command.c_str(),"stop") == 0) {
      #if STEPPERCOUNT == 1
        stepperOne.stop();//issue stop
        while (stepperOne.isRunning()) {// motor needs to decelerate
          stepperOne.run();
        }
        stepperOne.disableOutputs();
      #endif

      #if STEPPERCOUNT == 2
        stepperOne.stop();
        stepperTwo.stop();//stop both motors
        while ( (stepperOne.isRunning()) or (stepperTwo.isRunning()) ) {// motors need to decelerate
          stepperOne.run();
          stepperTwo.run();
        }
        stepperOne.disableOutputs();
        stepperTwo.disableOutputs();
      #endif
    }
  #endif
  
  if (strcmp(command.c_str(),"reset") == 0) {
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
          
    saveSettings();
    socketIO.disconnect();//  TO DO: check implementation
    ESP.restart();
  }

  if (strcmp(command.c_str(),"init") == 0) {
    Serial.println("received init command");
    #if STEPPERCOUNT > 0
      Serial.printf("stepper one position: %i steps\n", stepperOne.currentPosition());
      // TO DO: set stepperOne.currentPosition() as init position
    #endif
    #if STEPPERCOUNT > 1
      Serial.printf("stepper two position: %i steps\n", stepperTwo.currentPosition());
      // TO DO: set stepperTwo.currentPosition() as init position
    #endif
    #if SERVOCOUNT > 0
      settings.servoOneInit = map(servoOne.read(),0,180,settings.servoOneMinAngle,settings.servoOneMaxAngle);
    #endif
    #if SERVOCOUNT > 1
      settings.servoTwoInit = map(servoTwo.read(),0,180,settings.servoTwoMinAngle,settings.servoTwoMaxAngle);
    #endif
    saveSettings();
  }
  
  #if ISCAMERA
    if (strcmp(command.c_str(),"startStreaming") == 0) {
      startStreaming();
    }

    if (strcmp(command.c_str(),"stopStreaming") == 0) {
      stopStreaming();
    }
  #endif
}

void eventHandler(uint8_t * eventPayload, size_t eventLength) {
  char * sptr = NULL;
  int id = strtol((char*) eventPayload, &sptr, 10);
  Serial.printf("[IOc] got event: %s     (id: %d)\n", eventPayload, id);
  DynamicJsonDocument incomingEvent(1024);
  DeserializationError error = deserializeJson(incomingEvent, eventPayload, eventLength);
  if (error) {
    sendError(deserialize_failed, error.c_str());
    //Serial.printf("deserializeJson failed: ");
    //Serial.println(error.c_str());
    return;
  }

  String eventName = incomingEvent[0];
  Serial.printf("[IOc] event name: %s\n", eventName.c_str());
  
  if (strcmp(eventName.c_str(),"null") == 0) {
    sendError(field_is_null,"missing event type");
    //Serial.println("unable to identify event");
    if (strcmp((char *)eventPayload, "serial setup") == 0) {
      setViaSerial();
    }
    if (strcmp((char *)eventPayload, "identify") == 0) {
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
    }
  }

  if (strcmp(eventName.c_str(),"command") == 0) {
    // store and analyze input
    JsonObject receivedPayload = incomingEvent[1];
    String component = receivedPayload["componentId"]; // to do: check for type
    Serial.println("identified command event");

    // act only when this comoponent is involved
    if ( (strcmp(component.c_str(),settings.componentID.c_str()) == 0) or (strcmp(component.c_str(),"*") == 0) ) {
      Serial.printf("componentId identified: %s (me)\n", component.c_str());
      if (busyState) {
        sendError(is_busy,"component currently busy");
        return;
      }
      auto command = receivedPayload["command"];
      // check for simple or extended command structure
      if (command.is<JsonObject>()) {
        JsonObject command = receivedPayload["command"];
        eventCommandNested(command);
      }
      else if (command.is<String>()) {
        String command = receivedPayload["command"];
        eventCommandString(command);
      }
      else {
        Serial.println("could not identify command");
      }
    }
  }
}

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
    case sIOtype_CONNECT:
      Serial.printf("[IOc] Connected to URL: %s\n", payload);

      // join default namespace
      socketIO.send(sIOtype_CONNECT, "/");
      reportState();
      #if HASINFOLED
        infoLED.hsv(20000, 255, 40);
      #endif
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
      Serial.println("[WARNING] servo one not initialized. Set up frequency as well as duty range and reboot");
    }
    else {
      servoOne.setPeriodHertz(settings.servoOneFreq);
      servoOne.attach(SERVOONEPIN,settings.servoOneMinDuty,settings.servoOneMaxDuty);
      servoOne.write(map(constrain(settings.servoOneInit,settings.servoOneMinAngle,settings.servoOneMaxAngle),settings.servoOneMinAngle,settings.servoOneMaxAngle,0,180));
    }
  #endif
  #if SERVOCOUNT > 1
    if ((settings.servoTwoFreq == 0) or (settings.servoTwoMinDuty == 0) or (settings.servoTwoMaxDuty == 0)) {
      Serial.println("[WARNING] servo two not initialized. Set up frequency as well as duty range and reboot");
    }
    else {
      servoTwo.setPeriodHertz(settings.servoTwoFreq);
      servoTwo.attach(SERVOTWOPIN,settings.servoTwoMinDuty,settings.servoTwoMaxDuty);
      servoTwo.write(map(constrain(settings.servoTwoInit,settings.servoTwoMinAngle,settings.servoTwoMaxAngle),settings.servoTwoMinAngle,settings.servoTwoMaxAngle,0,180));
    }
  #endif

  #if RELAISCOUNT > 0
    if (strcmp(settings.relaisOneName.c_str(),"null") == 0) {
      Serial.println("[WARNING] relais one not initialized. Set controlId and reboot");
    }
    relaisOne.attach(RELAISONEPIN);
  #endif
  #if RELAISCOUNT > 1
    if (strcmp(settings.relaisTwoName.c_str(),"null") == 0) {
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

void XRTLloop() {
  socketIO.loop();
  
  #if ISCAMERA
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
    infoLED.loop();
  #endif
  if ( (Serial) && (Serial.available() > 0) ) {
    String serialEvent=Serial.readStringUntil('\n');
    eventHandler((uint8_t *)serialEvent.c_str(), serialEvent.length());
  }
}
