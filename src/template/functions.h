float mapFloat(float x, float in_min, float in_max, float out_min, float out_max)
{
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
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

void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[WiFi] lost connection. Trying to reconnect.\n");
  WiFi.reconnect();
}

void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("[WiFi] connected to WiFi with local IP: ");
  Serial.println(WiFi.localIP());
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
  String control = command["controlId"];
  Serial.printf("identified controlID: %s\n", control);

  if (strcmp(control.c_str(), "null") == 0) {
    Serial.println("control ID is null -- check settings");
  }
        
  #if STEPPERCOUNT > 0
    if (strcmp(control.c_str(),settings.stepperOneName.c_str()) == 0) {
      busyState = true;
      wasRunning = true;
      int val = command["val"];
      stepperOne.enableOutputs();
      stepperOne.move(val);
      // check if boundaries would be violated:
      stepperOne.moveTo(constrain(stepperOne.targetPosition(),settings.stepperOneMin, settings.stepperOneMax));
      reportState();       
    }
  #endif
  #if STEPPERCOUNT > 1
    if (strcmp(control.c_str(),settings.stepperTwoName.c_str()) == 0) {
      busyState = true;
      wasRunning = true;
      int val = command["val"];
      stepperTwo.enableOutputs();
      stepperTwo.move(val);
      stepperTwo.moveTo(constrain(stepperTwo.targetPosition(),settings.stepperTwoMin, settings.stepperTwoMax));
      reportState();
    }
  #endif
        
  #if SERVOCOUNT > 0
    if (strcmp(control.c_str(),settings.servoOneName.c_str()) == 0) {
      int val = command["val"];
      Serial.printf("found value: %i\n", val);
      servoOne.write(map(constrain(val,settings.servoOneMinAngle,settings.servoOneMaxAngle),settings.servoOneMinAngle,settings.servoOneMaxAngle,0,180));
      reportState();
    }
  #endif
  #if SERVOCOUNT > 1
    if (strcmp(control.c_str(),settings.servoTwoName.c_str()) == 0) {
      int val = command["val"];
      Serial.printf("found value: %i\n", val);
      servoTwo.write(map(constrain(val,settings.servoTwoMinAngle,settings.servoTwoMaxAngle),settings.servoTwoMinAngle,settings.servoTwoMaxAngle,0,180));
      reportState();
    }
  #endif
        
  #if RELAISCOUNT > 0
    if (strcmp(control.c_str(),settings.relaisOneName.c_str()) == 0) {
      auto val = command["val"];
      relaisOne.toggle(val);
      reportState();
    }
  #endif
  #if RELAISCOUNT > 1
    if (strcmp(control.c_str(),settings.relaisTwoName.c_str()) == 0) {
      auto val = command["val"];
      relaisTwo.toggle(val);
      reportState();
    }
  #endif

  #if ISCAMERA
    /*
    FRAMESIZE_UXGA (1600 x 1200)
    FRAMESIZE_QVGA (320 x 240)
    FRAMESIZE_CIF (352 x 288)
    FRAMESIZE_VGA (640 x 480)
    FRAMESIZE_SVGA (800 x 600)
    FRAMESIZE_XGA (1024 x 768)
    FRAMESIZE_SXGA (1280 x 1024)
    */
    if (strcmp(control.c_str(),"frame size") == 0) {
      Serial.println("trying to change frame size");
      String val = command["val"];
      if (strcmp(val.c_str(),"UXGA") == 0) {
        camera_config.frame_size = FRAMESIZE_UXGA;
        Serial.println("changed frame size to UXGA");
      }
      else if (strcmp(val.c_str(),"QVGA") == 0) {
        camera_config.frame_size = FRAMESIZE_QVGA;
        Serial.println("changed frame size to QVGA");
      }
      else if (strcmp(val.c_str(),"CIF") == 0) {
        camera_config.frame_size = FRAMESIZE_CIF;
        Serial.println("changed frame size to CIF");
      }
      else if (strcmp(val.c_str(),"VGA") == 0) {
        camera_config.frame_size = FRAMESIZE_VGA;
        Serial.println("changed frame size to VGA");
      }
      else if (strcmp(val.c_str(),"SVGA") == 0) {
        camera_config.frame_size = FRAMESIZE_SVGA;
        Serial.println("changed frame size to SVGA");
      }
      else if (strcmp(val.c_str(),"XGA") == 0) {
        camera_config.frame_size = FRAMESIZE_XGA;
        Serial.println("changed frame size to XGA");
      }
      else if (strcmp(val.c_str(),"SXGA") == 0) {
        camera_config.frame_size = FRAMESIZE_QVGA;
        Serial.println("changed frame size to QVGA");
      }
      else {
        Serial.println("could not identify frame size");
      }
    }

    if (strcmp(control.c_str(), "brightness") == 0) {
      int val = constrain(command["val"],-2,2);
      sensor_t * s = esp_camera_sensor_get();
      s->set_brightness(s,val);
      Serial.printf("set brightness to %i\n",val);
    }

    if (strcmp(control.c_str(), "contrast") == 0) {
      int val = constrain(command["val"],-2,2);
      sensor_t * s = esp_camera_sensor_get();
      s->set_contrast(s,val);
      Serial.printf("set contrast to %i\n",val);
    }

    if (strcmp(control.c_str(), "saturation") == 0) {
      int val = constrain(command["val"],2,2);
      sensor_t * s = esp_camera_sensor_get();
      s->set_saturation(s,val);
      Serial.printf("set saturation to %i\n",val);
    }

    if (strcmp(control.c_str(), "frame rate") == 0) {
      float val = constrain(command["val"].as<float>(),0,120);
      setMaxFrameRate(val);
      Serial.printf("set maximum frame rate to %f fps\n", val);
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
        stepperOne.stop();
        while (stepperOne.isRunning()) {
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
    }
  #endif
  
  if (strcmp(command.c_str(),"restart") == 0) {
    Serial.println("received restart command");
    Serial.println("disconnecting now");
          
    #if STEPPERCOUNT == 1
      stepperOne.stop();
      while (stepperOne.isRunning()) {
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
    ESP.restart();
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
    Serial.printf("deserializeJson failed: ");
    Serial.println(error.c_str());
    return;
  }

  String eventName = incomingEvent[0];
  Serial.printf("[IOc] event name: %s\n", eventName.c_str());
  
  if (strcmp(eventName.c_str(),"null") == 0) {
    Serial.println("unable to identify event");
    if (strcmp((char *)eventPayload, "serial setup") == 0) {
      setViaSerial();
    }
  }

  if (strcmp(eventName.c_str(),"command") == 0) {
    // store and analyze input
    JsonObject receivedPayload = incomingEvent[1];
    String component = receivedPayload["componentId"];
    Serial.println("identified command event");

    // act only when this comoponent is involved
    if (strcmp(component.c_str(),settings.componentID.c_str()) == 0) {
      Serial.printf("componentId identified: %s (me)\n", component);
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
  
  WiFi.onEvent(WiFiStationDisconnected, SYSTEM_EVENT_STA_DISCONNECTED);
  WiFi.onEvent(WiFiStationGotIP, SYSTEM_EVENT_STA_GOT_IP);
  WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, INADDR_NONE);
  WiFi.setHostname(settings.componentID.c_str());
  WiFi.begin(settings.ssid.c_str(), settings.password.c_str());

  #if HASINFOLED
    infoLED.begin();
    infoLED.hsv(0,255,40);
    infoLED.cycle(100,false);
    infoLED.start();
  #endif

  #if STEPPERCOUNT > 0
    stepperOne.setMaxSpeed(settings.stepperOneSpeed);
    stepperOne.setAcceleration(settings.stepperOneAcc);
  #endif
  #if STEPPERCOUNT > 1
    stepperTwo.setMaxSpeed(settings.stepperTwoSpeed);
    stepperTwo.setAcceleration(settings.stepperTwoAcc);
  #endif

  #if SERVOCOUNT > 0
    #if ISCAMERA
      trashOne.attach(2,1000,2000);
      trashTwo.attach(13,1000,2000);
    #endif
    servoOne.setPeriodHertz(settings.servoOneFreq);
    servoOne.attach(SERVOONEPIN,settings.servoOneMinDuty,settings.servoOneMaxDuty);
    servoOne.write(map(constrain(settings.servoOneInit,settings.servoOneMinAngle,settings.servoOneMaxAngle),settings.servoOneMinAngle,settings.servoOneMaxAngle,0,180));
  #endif
  #if SERVOCOUNT > 1
    servoTwo.setPeriodHertz(settings.servoTwoFreq);
    servoTwo.attach(SERVOTWOPIN,settings.servoTwoMinDuty,settings.servoTwoMaxDuty);
    servoTwo.write(map(constrain(settings.servoTwoInit,settings.servoTwoMinAngle,settings.servoTwoMaxAngle),settings.servoTwoMinAngle,settings.servoTwoMaxAngle,0,180));
  #endif

  #if RELAISCOUNT > 0 
    relaisOne.attach(RELAISONEPIN);
  #endif
  #if RELAISCOUNT > 1
    relaisTwo.attach(RELAISTWOPIN);
  #endif

  #if ISCAMERA
    init_camera();
    setMaxFrameRate(30);
  #endif

  socketIO.begin(settings.socketIP.c_str(), settings.socketPort, settings.socketURL.c_str());
  socketIO.onEvent(socketIOEvent);
  
  Serial.println("setup done");
}

void XRTLloop() {
  socketIO.loop();
  
  #if ISCAMERA
    if (streamRunning && (WiFi.status() == WL_CONNECTED) && (socketIO.isConnected())) {
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
      busyState = false;
      wasRunning = false;
      reportState();
    }
  #endif

  #if HASINFOLED
    infoLED.loop();
  #endif

  if (Serial.available() > 0) {
    String serialEvent=Serial.readStringUntil('\n');
    eventHandler((uint8_t *)serialEvent.c_str(), serialEvent.length());
  }
}
