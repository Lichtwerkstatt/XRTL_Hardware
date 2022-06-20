struct {
  String ssid;
  String password;
  String socketIP;
  int socketPort;
  String socketURL;
  String componentID;
  String componentAlias;

  #if STEPPERCOUNT > 0 
  String stepperOneName;
  int stepperOneAcc;
  int stepperOneSpeed;
  long stepperOnePos;
  long stepperOneMin;
  long stepperOneMax;
  long stepperOneInit;
  bool stepperOneSlider;
  #endif

  #if STEPPERCOUNT > 1
  String stepperTwoName;
  int stepperTwoAcc;
  int stepperTwoSpeed;
  long stepperTwoPos;
  long stepperTwoMin;
  long stepperTwoMax;
  long stepperTwoInit;
  bool stepperTwoSlider;
  #endif

  #if SERVOCOUNT > 0
  String servoOneName;
  int servoOneFreq;
  int servoOneMinDuty;
  int servoOneMaxDuty;
  int servoOneMinAngle;
  int servoOneMaxAngle;
  int servoOneInit;
  #endif

  #if SERVOCOUNT > 1
  String servoTwoName;
  int servoTwoFreq;
  int servoTwoMinDuty;
  int servoTwoMaxDuty;
  int servoTwoMinAngle;
  int servoTwoMaxAngle;
  int servoTwoInit;
  #endif

  #if RELAISCOUNT > 0
  String relaisOneName;
  #endif

  #if RELAISCOUNT > 1
  String relaisTwoName;
  #endif
} settings;

void saveSettings() {
  DynamicJsonDocument doc(1024);

  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["socketIP"] = settings.socketIP;
  doc["socketPort"] = settings.socketPort;
  doc["socketURL"] = settings.socketURL;
  doc["componentID"] = settings.componentID;
  doc["componentAlias"] = settings.componentAlias;

  #if STEPPERCOUNT > 0
  doc["stepperOneName"] = settings.stepperOneName;
  doc["stepperOneAcc"] = settings.stepperOneAcc;
  doc["stepperOneSpeed"] = settings.stepperOneSpeed;
  doc["stepperOnePos"] = stepperOne.currentPosition();
  doc["stepperOneMin"] = settings.stepperOneMin;
  doc["stepperOneMax"] = settings.stepperOneMax;
  doc["stepperOneInit"] = settings.stepperOneInit;
  doc["stepperOneSlider"] = settings.stepperOneSlider;
  #endif

  #if STEPPERCOUNT > 1
  doc["stepperTwoName"] = settings.stepperTwoName;
  doc["stepperTwoAcc"] = settings.stepperTwoAcc;
  doc["stepperTwoSpeed"] = settings.stepperTwoSpeed;
  doc["stepperTwoPos"] = stepperTwo.currentPosition();
  doc["stepperTwoMin"] = settings.stepperTwoMin;
  doc["stepperTwoMax"] = settings.stepperTwoMax;
  doc["stepperTwoInit"] = settings.stepperTwoInit;
  doc["stepperTwoSlider"] = settings.stepperTwoSlider;
  #endif

  #if SERVOCOUNT > 0
  doc["servoOneName"] = settings.servoOneName;
  doc["servoOneFreq"] = settings.servoOneFreq;
  doc["servoOneMinDuty"] = settings.servoOneMinDuty;
  doc["servoOneMaxDuty"] = settings.servoOneMaxDuty;
  doc["servoOneMinAngle"] = settings.servoOneMinAngle;
  doc["servoOneMaxAngle"] = settings.servoOneMaxAngle;
  doc["servoOneInit"] = settings.servoOneInit;
  #endif

  #if SERVOCOUNT > 1
  doc["servoTwoName"] = settings.servoTwoName;
  doc["servoTwoFreq"] = settings.servoTwoFreq;
  doc["servoTwoMinDuty"] = settings.servoTwoMinDuty;
  doc["servoTwoMaxDuty"] = settings.servoTwoMaxDuty;
  doc["servoTwoMinAngle"] = settings.servoTwoMinAngle;
  doc["servoTwoMaxAngle"] = settings.servoTwoMaxAngle;
  doc["servoTwoInit"] = settings.servoTwoInit;
  #endif

  #if RELAISCOUNT > 0
  doc["relaisOneName"] = settings.relaisOneName;
  #endif

  #if RELAISCOUNT > 1
  doc["relaisTwoName"] = settings.relaisTwoName;
  #endif
  
  if (!FILESYSTEM.begin(false)) {
    Serial.println("failed to mount LittleFS");
    Serial.println("trying to format LittleFS");
    if (!FILESYSTEM.begin(true)) {
      Serial.println("failed to mount LittleFS again");
      Serial.println("unable to format LittleFS");
      return;
    }
  }

  File file = FILESYSTEM.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("Failed to write file");
  }
  else {
    Serial.println("successfully saved settings");
  }
  file.close();
}

void loadSettings() {
  if(!FILESYSTEM.begin(false)){
    Serial.println("failed to mount LittleFs");
    Serial.println("could not load settings");
    Serial.println("trying to format file system");
    if (!FILESYSTEM.begin(true)) {
      Serial.println("failed to mount LittleFs again");
      Serial.println("unable to format LittleFS");
      return;
    }
    else {
      Serial.println("successfully formated file system");
      Serial.println("saving current settings structure");
      saveSettings();
    }
  }
  File file = FILESYSTEM.open("/settings.txt","r");
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    Serial.printf("deserializeJson() while loading settings: ");
    Serial.println(error.c_str());
  }
  file.close();

  settings.ssid = doc["ssid"].as<String>();
  settings.password = doc["password"].as<String>();
  settings.socketIP = doc["socketIP"].as<String>();
  settings.socketPort = doc["socketPort"];
  settings.socketURL = doc["socketURL"].as<String>();
  settings.componentID = doc["componentID"].as<String>();
  settings.componentAlias = doc["componentAlias"].as<String>();

  Serial.println("");
  Serial.println("======================================");
  Serial.println("           loading settings           ");
  Serial.println("======================================");
  Serial.println("");
  Serial.printf("SSID: %s\n", settings.ssid.c_str());
  Serial.printf("password: %s\n", settings.password.c_str());
  Serial.printf("socket IP: %s\n", settings.socketIP.c_str());
  Serial.printf("Port: %i\n", settings.socketPort);
  Serial.printf("URL: %s\n", settings.socketURL.c_str());
  Serial.printf("component ID: %s\n", settings.componentID.c_str());
  Serial.printf("component alias: %s\n", settings.componentAlias.c_str());
  Serial.println("");

  #if STEPPERCOUNT > 0
  settings.stepperOneName = doc["stepperOneName"].as<String>();
  settings.stepperOneAcc = doc["stepperOneAcc"];
  settings.stepperOneSpeed = doc["stepperOneSpeed"];
  settings.stepperOnePos = doc["stepperOnePos"];
  settings.stepperOneMin = doc["stepperOneMin"];
  settings.stepperOneMax = doc["stepperOneMax"];
  settings.stepperOneInit = doc["stepperOneInit"];
  settings.stepperOneSlider = doc["stepperOneSlider"];

  stepperOne.setCurrentPosition(settings.stepperOnePos);

  Serial.printf("stepper one name: %s\n", settings.stepperOneName.c_str());
  Serial.printf("stepper one accelleration: %i\n", settings.stepperOneAcc);
  Serial.printf("stepper one speed: %i\n", settings.stepperOneSpeed);
  Serial.printf("stepper one position: %i\n", settings.stepperOnePos);
  Serial.printf("stepper one minimum: %i\n", settings.stepperOneMin);
  Serial.printf("stepper one maximum: %i\n", settings.stepperOneMax);
  Serial.printf("stepper one initialization: %i\n", settings.stepperOneInit);
  Serial.printf(settings.stepperOneSlider ? "stepper one is slider controled\n" : "stepper one is step controled\n");
  Serial.println("");
  #endif

  #if STEPPERCOUNT > 1
  settings.stepperTwoName = doc["stepperTwoName"].as<String>();
  settings.stepperTwoAcc = doc["stepperTwoAcc"];
  settings.stepperTwoSpeed = doc["stepperTwoSpeed"];
  settings.stepperTwoPos = doc["stepperTwoPos"];
  settings.stepperTwoMin = doc["stepperTwoMin"];
  settings.stepperTwoMax = doc["stepperTwoMax"];
  settings.stepperTwoInit = doc["stepperTwoInit"];
  settings.stepperTwoSlider = doc["stepperTwoSlider"];

  stepperTwo.setCurrentPosition(settings.stepperTwoPos);

  Serial.printf("stepper two name: %s\n", settings.stepperTwoName.c_str());
  Serial.printf("stepper two accelleration: %i\n", settings.stepperTwoAcc);
  Serial.printf("stepper two speed: %i\n", settings.stepperTwoSpeed);
  Serial.printf("stepper two position: %i\n", settings.stepperTwoPos);
  Serial.printf("stepper two minimum: %i\n", settings.stepperTwoMin);
  Serial.printf("stepper two maximum: %i\n", settings.stepperTwoMax);
  Serial.printf("stepper two initialization: %i\n",settings.stepperTwoInit);
  Serial.printf(settings.stepperTwoSlider ? "stepper two is slider controled\n" : "stepper two is step controled\n");
  Serial.println("");
  #endif

  #if SERVOCOUNT > 0 
  settings.servoOneName = doc["servoOneName"].as<String>();
  settings.servoOneFreq = doc["servoOneFreq"];
  settings.servoOneMinDuty = doc["servoOneMinDuty"];
  settings.servoOneMaxDuty = doc["servoOneMaxDuty"];
  settings.servoOneMinAngle = doc["servoOneMinAngle"];
  settings.servoOneMaxAngle = doc["servoOneMaxAngle"];
  settings.servoOneInit = doc["servoOneInit"];
  
  Serial.printf("servo one name: %s\n", settings.servoOneName.c_str());
  Serial.printf("servo one frequency: %i Hz\n", settings.servoOneFreq);
  Serial.printf("servo one minimum duty time: %i µs\n", settings.servoOneMinDuty);
  Serial.printf("servo one maximum duty time: %i µs\n", settings.servoOneMaxDuty);
  Serial.printf("servo one minimum angle: %i\n", settings.servoOneMinAngle);
  Serial.printf("servo one maximum angle: %i\n", settings.servoOneMaxAngle);
  Serial.printf("servo one initialization: %i\n", settings.servoOneInit);
  Serial.println("");
  #endif

  #if SERVOCOUNT > 1
  settings.servoTwoName = doc["servoTwoName"].as<String>();
  settings.servoTwoFreq = doc["servoTwoFreq"];
  settings.servoTwoMinDuty = doc["servoTwoMinDuty"];
  settings.servoTwoMaxDuty = doc["servoTwoMaxDuty"];
  settings.servoTwoMinAngle = doc["servoTwoMinAngle"];
  settings.servoTwoMaxAngle = doc["servoTwoMaxAngle"];
  settings.servoTwoInit = doc["servoTwoInit"];
  
  Serial.printf("servo two name: %s\n", settings.servoTwoName.c_str());
  Serial.printf("servo two frequency: %i Hz\n", settings.servoTwoFreq);
  Serial.printf("servo two minimum duty time: %i µs\n", settings.servoTwoMinDuty);
  Serial.printf("servo two maximum duty time: %i µs\n", settings.servoTwoMaxDuty);
  Serial.printf("servo two minimum angle: %i\n", settings.servoTwoMinAngle);
  Serial.printf("servo two maximum angle: %i\n", settings.servoTwoMaxAngle);
  Serial.printf("servo two initialization: %i\n", settings.servoTwoInit);
  Serial.println("");
  #endif

  #if RELAISCOUNT > 0
  settings.relaisOneName = doc["relaisOneName"].as<String>();
  Serial.printf("relais one name: %s\n\n", settings.relaisOneName.c_str());
  #endif

  #if RELAISCOUNT > 1
  settings.relaisTwoName = doc["relaisTwoName"].as<String>();
  Serial.printf("relais two name: %s\n\n", settings.relaisTwoName.c_str());
  #endif

  Serial.println("======================================");
  Serial.println("");
}

String serialInput(String question) {
  Serial.print(question);
  while (!Serial.available()) {
    yield();
  }
  String answer = Serial.readStringUntil('\n');
  Serial.println(answer);
  return answer;
}

// used to handle recconects, deregistered in serial setup
void WiFiStationDisconnected(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.printf("[WiFi] lost connection. Trying to reconnect.\n");
  WiFi.reconnect();
}

// informs when an IP is aquired, deregistered in serial setup
void WiFiStationGotIP(WiFiEvent_t event, WiFiEventInfo_t info) {
  Serial.print("[WiFi] connected to WiFi with local IP: ");
  Serial.println(WiFi.localIP());
}

WiFiEventId_t eventIdDisconnected; // use this when attaching the disconnected handler
WiFiEventId_t eventIdGotIP; // use this when attaching the gotIP handler

void setViaSerial() {
  WiFi.removeEvent(eventIdGotIP); // get rid of autoamtic reconnect to avoid error messages
  WiFi.removeEvent(eventIdDisconnected);
  WiFi.disconnect(true); // disconnecting from WiFi should prevent serial output messing up the process
  WiFi.mode(WIFI_OFF);
  int64_t startTime = esp_timer_get_time();
  while (esp_timer_get_time() - startTime < 200000) {
    yield();
  }
  DynamicJsonDocument doc(1024);

  //load doc with current values as failsafe

  doc["ssid"] = settings.ssid;
  doc["password"] = settings.password;
  doc["socketIP"] = settings.socketIP;
  doc["socketPort"] = settings.socketPort;
  doc["socketURL"] = settings.socketURL;
  doc["componentID"] = settings.componentID;
  doc["componentAlias"] = settings.componentAlias;

  #if STEPPERCOUNT > 0
  doc["stepperOneName"] = settings.stepperOneName;
  doc["stepperOneAcc"] = settings.stepperOneAcc;
  doc["stepperOneSpeed"] = settings.stepperOneSpeed;
  doc["stepperOnePos"] = stepperOne.currentPosition();
  doc["stepperOneMin"] = settings.stepperOneMin;
  doc["stepperOneMax"] = settings.stepperOneMax;
  doc["stepperOneInit"] = settings.stepperOneInit;
  doc["stepperOneSlider"] = settings.stepperOneSlider;
  #endif

  #if STEPPERCOUNT > 1
  doc["stepperTwoName"] = settings.stepperTwoName;
  doc["stepperTwoAcc"] = settings.stepperTwoAcc;
  doc["stepperTwoSpeed"] = settings.stepperTwoSpeed;
  doc["stepperTwoPos"] = stepperTwo.currentPosition();
  doc["stepperTwoMin"] = settings.stepperTwoMin;
  doc["stepperTwoMax"] = settings.stepperTwoMax;
  doc["stepperTwoInit"] = settings.stepperTwoInit;
  doc["stepperTwoSlider"] = settings.stepperTwoSlider;
  #endif

  #if SERVOCOUNT > 0
  doc["servoOneName"] = settings.servoOneName;
  doc["servoOneFreq"] = settings.servoOneFreq;
  doc["servoOneMinDuty"] = settings.servoOneMinDuty;
  doc["servoOneMaxDuty"] = settings.servoOneMaxDuty;
  doc["servoOneMinAngle"] = settings.servoOneMinAngle;
  doc["servoOneMaxAngle"] = settings.servoOneMaxAngle;
  doc["servoOneInit"] = settings.servoOneInit;
  #endif

  #if SERVOCOUNT > 1
  doc["servoTwoName"] = settings.servoTwoName;
  doc["servoTwoFreq"] = settings.servoTwoFreq;
  doc["servoTwoMinDuty"] = settings.servoTwoMinDuty;
  doc["servoTwoMaxDuty"] = settings.servoTwoMaxDuty;
  doc["servoTwoMinAngle"] = settings.servoTwoMinAngle;
  doc["servoTwoMaxAngle"] = settings.servoTwoMaxAngle;
  doc["servoTwoInit"] = settings.servoTwoInit;
  #endif

  #if RELAISCOUNT > 0
  doc["relaisOneName"] = settings.relaisOneName;
  #endif

  #if RELAISCOUNT > 1
  doc["relaisTwoName"] = settings.relaisTwoName;
  #endif

  Serial.println("");
  Serial.println("======================================");
  Serial.println("           setup via serial           ");
  Serial.println("======================================");
  Serial.println("");
  Serial.println("availabe settings:");
  Serial.println("0: edit all");
  Serial.println("1: general info");
  #if STEPPERCOUNT > 0
    Serial.println("2: stepper one");
  #endif
  #if STEPPERCOUNT > 1
    Serial.println("3: stepper two");
  #endif
  #if SERVOCOUNT > 0
    Serial.println("4: servo one");
  #endif
  #if SERVOCOUNT > 1
    Serial.println("5: servo two");
  #endif
  #if RELAISCOUNT > 0
    Serial.println("6: relais one");
  #endif
  #if RELAISCOUNT > 1
    Serial.println("7: relais two");
  #endif
  Serial.println("");

  Serial.readStringUntil('\n');

  int choice = serialInput("choose setting: ").toInt();
  Serial.println("");

  if (choice == 0) {// complete setup
    doc["ssid"] = serialInput("WiFi SSID: ");
    doc["password"] = serialInput("WiFi password: ");;
    doc["socketIP"] = serialInput("socket IP: ");
    doc["socketPort"] = serialInput("socket Port: ").toInt();
    doc["socketURL"] = serialInput("socket URL: ");
    doc["componentID"] = serialInput("component ID: ");
    doc["componentAlias"] = serialInput("component alias: ");
    Serial.println("");

    #if STEPPERCOUNT > 0
      doc["stepperOneName"] = serialInput("first stepper ID: ");
      doc["stepperOneAcc"] = serialInput("first stepper accelleration (steps/s²): ").toInt();
      doc["stepperOneSpeed"] = serialInput("first stepper speed (steps/s): ").toInt();
      doc["stepperOnePos"] = serialInput("first stepper current position (steps): ").toInt();
      doc["stepperOneMin"] = serialInput("first stepper minimum position (steps): ").toInt();
      doc["stepperOneMax"] = serialInput("first stepper maximum position (steps): ").toInt();
      doc["stepperOneInit"] = serialInput("first stepper initialization (steps): ").toInt();
      doc["stepperOneSlider"] = (serialInput("first stepper slider control (bool): ") == "true");
      Serial.println("");
    #endif

    #if STEPPERCOUNT > 1
      doc["stepperTwoName"] = serialInput("second stepper ID: ");
      doc["stepperTwoAcc"] = serialInput("second stepper accelleration (steps/s²): ").toInt();
      doc["stepperTwoSpeed"] = serialInput("second stepper maximum speed (steps/s): ").toInt();
      doc["stepperTwoPos"] = serialInput("second stepper current position (steps): ").toInt();
      doc["stepperTwoMin"] = serialInput("second stepper minimum position (steps): ").toInt();
      doc["stepperTwoMax"] = serialInput("second stepper maximum position (steps): ").toInt();
      doc["stepperTwoInit"] = serialInput("second stepper initialization (steps): ").toInt();
      doc["stepperTwoSlider"] = (serialInput("second stepper slider control (bool): ") == "true");
      Serial.println("");
    #endif

    #if SERVOCOUNT > 0
      doc["servoOneName"] = serialInput("first servo ID: ");
      doc["servoOneFreq"] = serialInput("first servo frequency (Hz): ").toInt();
      doc["servoOneMinDuty"] = serialInput("first servo minimum duty (µs): ").toInt();
      doc["servoOneMaxDuty"] = serialInput("first servo maximum duty (µs): ").toInt();
      doc["servoOneMinAngle"] = serialInput("first servo minimum (int): ").toInt();
      doc["servoOneMaxAngle"] = serialInput("first servo maximum (int): ").toInt();
      doc["servoOneInit"] = serialInput("first servo initialization value (int): ").toInt();
      Serial.println("");
    #endif

    #if SERVOCOUNT > 1
      doc["servoTwoName"] = serialInput("second servo ID: ");
      doc["servoTwoFreq"] = serialInput("second servo frequency (Hz): ").toInt();
      doc["servoTwoMinDuty"] = serialInput("second servo minimum duty (µs): ").toInt();
      doc["servoTwoMaxDuty"] = serialInput("second servo maximum duty (µs): ").toInt();
      doc["servoTwoMinAngle"] = serialInput("second servo minimum (int): ").toInt();
      doc["servoTwoMaxAngle"] = serialInput("second servo maximum (int): ").toInt();
      doc["servoTwoInit"] = serialInput("second servo initialization (int): ").toInt();
      Serial.println("");
    #endif

    #if RELAISCOUNT > 0
      doc["relaisOneName"] = serialInput("first relais ID: ");
      Serial.println("");
    #endif

    #if RELAISCOUNT > 1
      doc["relaisTwoName"] = serialInput("second relais ID: ");
      Serial.println("");
    #endif
  }
  
  else if (choice == 1) {//general setup
    doc["ssid"] = serialInput("WiFi SSID: ");
    doc["password"] = serialInput("WiFi password: ");;
    doc["socketIP"] = serialInput("socket IP: ");
    doc["socketPort"] = serialInput("socket Port: ").toInt();
    doc["socketURL"] = serialInput("socket URL: ");
    doc["componentID"] = serialInput("component ID: ");
    doc["componentAlias"] = serialInput("component alias: ");
    Serial.println("");
  }
  
  else if (choice == 2) {//stepper one setup
    #if STEPPERCOUNT < 1
      Serial.println("stepper one not available");
      return;
    #endif
    #if STEPPERCOUNT > 0
      doc["stepperOneName"] = serialInput("first stepper ID: ");
      doc["stepperOneAcc"] = serialInput("first stepper accelleration (steps/s²): ").toInt();
      doc["stepperOneSpeed"] = serialInput("first stepper speed (steps/s): ").toInt();
      doc["stepperOnePos"] = serialInput("first stepper current position (steps): ").toInt();
      doc["stepperOneMin"] = serialInput("first stepper minimum position (steps): ").toInt();
      doc["stepperOneMax"] = serialInput("first stepper maximum position (steps): ").toInt();
      doc["stepperOneInit"] = serialInput("first stepper initialization (steps): ").toInt();
      doc["stepperOneSlider"] = (serialInput("first stepper slider control (bool): ") == "true");
      Serial.println("");
    #endif
  }
  
  else if (choice == 3) {//stepper two setup
    #if STEPPERCOUNT < 2
      Serial.println("stepper two not available");
      return;
    #endif
    #if STEPPERCOUNT > 1
      doc["stepperTwoName"] = serialInput("second stepper ID: ");
      doc["stepperTwoAcc"] = serialInput("second stepper accelleration (steps/s²): ").toInt();
      doc["stepperTwoSpeed"] = serialInput("second stepper maximum speed (steps/s): ").toInt();
      doc["stepperTwoPos"] = serialInput("second stepper current position (steps): ").toInt();
      doc["stepperTwoMin"] = serialInput("second stepper minimum position (steps): ").toInt();
      doc["stepperTwoMax"] = serialInput("second stepper maximum position (steps): ").toInt();
      doc["stepperTwoInit"] = serialInput("second stepper initialization (steps): ").toInt();
      doc["stepperTwoSlider"] = (serialInput("second stepper slider control (bool): ") == "true");
      Serial.println("");
    #endif
  }

  else if (choice == 4) {//servo one setup
    #if SERVOCOUNT < 1
      Serial.println("servo one not available");
      return;
    #endif
    #if SERVOCOUNT > 0
      doc["servoOneName"] = serialInput("first servo ID: ");
      doc["servoOneFreq"] = serialInput("first servo frequency (Hz): ").toInt();
      doc["servoOneMinDuty"] = serialInput("first servo minimum duty (µs): ").toInt();
      doc["servoOneMaxDuty"] = serialInput("first servo maximum duty (µs): ").toInt();
      doc["servoOneMinAngle"] = serialInput("first servo minimum (int): ").toInt();
      doc["servoOneMaxAngle"] = serialInput("first servo maximum (int): ").toInt();
      doc["servoOneInit"] = serialInput("first servo initialization value (int): ").toInt();
      Serial.println("");
    #endif
  }

  else if (choice == 5) {//servo two setup
    #if SERVOCOUNT < 2
      Serial.println("servo two not available");
      return;
    #endif
    #if SERVOCOUNT > 1
      doc["servoTwoName"] = serialInput("second servo ID: ");
      doc["servoTwoFreq"] = serialInput("second servo frequency (Hz): ").toInt();
      doc["servoTwoMinDuty"] = serialInput("second servo minimum duty (µs): ").toInt();
      doc["servoTwoMaxDuty"] = serialInput("second servo maximum duty (µs): ").toInt();
      doc["servoTwoMinAngle"] = serialInput("second servo minimum (int): ").toInt();
      doc["servoTwoMaxAngle"] = serialInput("second servo maximum (int): ").toInt();
      doc["servoTwoInit"] = serialInput("second servo initialization (int): ").toInt();
      Serial.println("");
    #endif
  }

  else if (choice == 6) {//relais one setup
    #if RELAISCOUNT < 1
      Serial.println("relais one not available");
      return;
    #endif
    #if RELAISCOUNT > 0
      doc["relaisOneName"] = serialInput("first relais ID: ");
      Serial.println("");
    #endif
  }

  else if (choice == 7) {//relais two setup
    #if RELAISCOUNT < 2
      Serial.println("relais two not available");
      return;
    #endif
    #if RELAISCOUNT > 1
      doc["relaisTwoName"] = serialInput("second relais ID: ");
      Serial.println("");
    #endif
  }

  else {
    Serial.println("unable to choose setup routine");
    return;
  }

  Serial.println("======================================");

  if (!FILESYSTEM.begin(false)) {
    Serial.println("failed to mount LittleFS");
    Serial.println("trying to format LittleFS");
    if (!FILESYSTEM.begin(true)) {
      Serial.println("failed to mount LittleFS again");
      Serial.println("unable to format LittleFS");
      return;
    }
  }

  File file = FILESYSTEM.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    Serial.println("");
    Serial.println("failed to write file");
    file.close();
  }
  else {
    file.close();
    Serial.println("");
    Serial.println("successfully saved settings");
    Serial.println("restarting now");
    ESP.restart();
  }
}
