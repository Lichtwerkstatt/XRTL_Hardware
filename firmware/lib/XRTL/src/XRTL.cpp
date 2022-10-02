#include "XRTL.h"

void XRTL::setup(){ 
  Serial.begin(115200);

  debug("starting setup");
  loadSettings();

  // execute setup of all modules
  for (int i = 0; i < moduleCount; i++){
    module[i]->setup();
  }
  
  debug("setup complete");
}

void XRTL::loop(){
  if ( Serial.available() ) {
    //allow to switch into debug mode
    String input = Serial.readStringUntil('\n');
    if ( strcmp(input.c_str(), "debug") == 0 ) {
      debugging = !debugging;
      if (debugging) notify(debug_on);
      else notify(debug_off);
    }

    //only allow inputs if debugging
    if (debugging) {
      if (strcmp(input.c_str(),"setup") == 0){
        setViaSerial();
      }
      else if (strcmp(input.c_str(), "debug") == 0) {} // do not interprete debug as event
      else {
        DynamicJsonDocument serialEvent(1024);
        DeserializationError error = deserializeJson(serialEvent,input);
        if (error) {
          Serial.printf("[core] deserializeJson() failed on serial input: %s\n", error.c_str());
          Serial.printf("[core] input: %s\n", input.c_str());
        }
        else if (socketIO != NULL) { // make sure the socket is initialized
          socketIO->handleEvent(serialEvent);
        }
      }
    }
  }

  // execute loop of each module
  for (int i = 0; i < moduleCount; i++) {
    module[i]->loop();
  }
}

void XRTL::addModule(String moduleName, moduleType category) {
  switch(category) {
    case xrtl_socket: {
      socketIO = new SocketModule(moduleName,this);
      module[moduleCount++] = socketIO;
      debug("socket module added: <%s>", moduleName);
      return;
    }
    case xrtl_wifi: {
      module[moduleCount++] = new WifiModule(moduleName,this);
      debug("wifi module added: <%s>", moduleName);
      return;
    }
    case xrtl_infoLED: {
      module[moduleCount++] = new InfoLEDModule(moduleName,this);
      debug("infoLED module added: <%s>", moduleName);
      return;
    }
    case xrtl_stepper: {
      module[moduleCount++] = new StepperModule(moduleName,this);
      debug("stepper module added: <%s>", moduleName);
      return;
    }
    case xrtl_servo: {
      module[moduleCount++] = new ServoModule(moduleName,this);
      debug("servo module added: <%s>", moduleName);
      return;
    }
    case xrtl_camera: {
      module[moduleCount++] = new CameraModule(moduleName,this);
      debug("camera module added: <%s>", moduleName);
      return;
    }
    case xrtl_output: {
      module[moduleCount++] = new OutputModule(moduleName,this);
      debug("output module added: <%s>", moduleName);
      return;
    }
    case xrtl_input: {
      module[moduleCount++] = new InputModule(moduleName,this);
      debug("input module added: <%s>", moduleName);
      return;
    }
  }
}

void XRTL::listModules() {
  for (int i= 0; i < moduleCount; i++) {
    Serial.printf("%d: %s\n", i, module[i]->getID().c_str());
  }
}

void XRTL::delModule(uint8_t number) {
  if ((number >= 0) and (number < moduleCount)){
    Serial.printf("[core] deleting <%s> ... ", module[number]->getID());
    for (int i = number; i < moduleCount - 1; i++) {
      module[i] = module[i+1];
    }
    moduleCount--;
    Serial.println("done");
  }
}

void XRTL::swapModules(uint8_t numberX, uint8_t numberY) {
  if ( (numberX != numberY)
      and (numberX >= 0) and (numberX <= moduleCount)
      and (numberY >= 0) and (numberY <= moduleCount) ) {
        XRTLmodule* temp = module[numberX];
        module[numberX] = module[numberY];
        module[numberY] = temp;

        debug("swapped <%s> and <%s>", module[numberX]->getID(), module[numberY]->getID());
      }
  
}

XRTLmodule* XRTL::operator[](String moduleName) {
  for (int i = 0; i < moduleCount; i++) {
    if (module[i]->isModule(moduleName)) {
      return module[i];
    }
  }
  return NULL;
}

void XRTL::saveSettings() {
  DynamicJsonDocument doc(2048);

  for (int i = 0; i < moduleCount; i++) {
    JsonObject settings = doc.createNestedObject(module[i]->getID());
    settings["type"] = module[i]->getType();
    //module[i]->saveSettings(doc);
    module[i]->saveSettings(settings);
  }

  if (!LittleFS.begin(false)) {
    debug("failed to mount LittleFS");
    debug("trying to format LittleFS");
    if (!LittleFS.begin(true)) {
      debug("failed to mount LittleFS again");
      debug("unable to format LittleFS");
      debug("unable to save settings");
      return;
    }
  }

  File file = LittleFS.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    debug("failed to write file");
    debug("could not save settings");
  }
  else {
    debug("settings successfully saved");
  }
  file.close();
  LittleFS.end();
}

void XRTL::loadSettings() {
  if (debugging) {
    Serial.println("");
    Serial.println(centerString("",39,'='));
    Serial.println(centerString("loading settings",39,' '));
    Serial.println(centerString("",39,'='));
    Serial.println("");
  }
  
  if (!LittleFS.begin(false)) {
    debug("failed to mount LittleFS");
    debug("trying to format LittleFS");
    if(!LittleFS.begin(true)) {
      debug("failed to mount LittleFS again");
      debug("unable to format LittleFS");
      debug("could not load settings");
      return;
    }
    else {
      debug("successfully formated file system");
      debug("creating new settings file");

      saveSettings();
    }
  }

  File file = LittleFS.open("/settings.txt","r");
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    debug("deserializeJson() failed while loading settings: <%s>", error.c_str());
  }
  file.close();
  LittleFS.end();

  JsonObject settings = doc.as<JsonObject>();

  for (JsonPair kv : settings) {
    moduleType type;
    if ( (!kv.value().isNull()) and (kv.value().is<JsonObject>()) ) {
      JsonObject moduleSettings = kv.value().as<JsonObject>();
      auto typeField = moduleSettings["type"];
      if ((!typeField.isNull()) and (typeField.is<int>()) ) {
        type = (moduleType) typeField.as<int>();

        if (debugging) Serial.println("");
        if (debugging) Serial.println(centerString("", 39, '-'));

        addModule(kv.key().c_str(), type);

        if (debugging) Serial.println(centerString("", 39, '-'));
        if (debugging) Serial.println("");

        module[moduleCount -1]->loadSettings(moduleSettings);
      }
    }
  }

  if (moduleCount == 0) { // 
    debug("WARNING: no modules found, adding socket and wifi module");
    addModule("socket", xrtl_socket);
    addModule("wifi", xrtl_wifi);
    saveSettings();
    ESP.restart();
  }

  if (!debugging) return;
  Serial.println("");
  Serial.println(centerString("",39,'='));
  Serial.println(centerString("loading successfull", 39, ' '));
  Serial.println(centerString("",39,'='));
  Serial.println("");
}

void XRTL::setViaSerial() {
  stop();

  Serial.println("");
  Serial.println(centerString("",39,'='));
  Serial.println(centerString("serial setup",39,' '));
  Serial.println(centerString("",39,'='));
  Serial.println("");

  Serial.println(centerString("available settings", 39, ' '));
  listModules();
  Serial.println("");
  Serial.printf("%d: complete setup\n", moduleCount);
  Serial.printf("%d: add module\n", moduleCount + 1);
  Serial.printf("%d: remove module\n", moduleCount + 2);
  Serial.printf("%d: swap modules\n", moduleCount + 3);
  Serial.println("");
  uint8_t choice = serialInput("choose setup routine: ").toInt();

  if (choice < moduleCount ) {
    module[choice]->setViaSerial();
  }
  else if (choice == moduleCount) {
    Serial.println("starting complete setup");
    for (int i = 0; i < moduleCount; i++) {
      module[i]->setViaSerial();
    }
  }
  else if (choice == moduleCount + 1) {
    Serial.println(centerString("", 39, '-'));
    Serial.println("adding module");
    Serial.println("module type is determined by number, available types:");
    Serial.println("");
    for (int i = 0; i < 8; i++) {
      Serial.printf("%d: %s\n",i,moduleNames[i]);
    }
    Serial.println("");

    moduleType newModuleType = (moduleType) serialInput("new module type: ").toInt();
    String newModuleName = serialInput("new module name: ");
    addModule(newModuleName, newModuleType);
    JsonObject emptySettings;
    module[moduleCount - 1]->loadSettings(emptySettings); // initialize with default parameters
  }
  else if (choice == moduleCount + 2) {
    Serial.println(centerString("", 39, '-'));
    Serial.println("available modules: ");
    Serial.println("");
    listModules();
    Serial.printf("%d: exit\n", moduleCount);
    Serial.println("");

    uint8_t deleteChoice = serialInput("delete: ").toInt();
    delModule(deleteChoice);
  }
  else if (choice == moduleCount + 3) {
    Serial.println(centerString("", 39, '-'));
    Serial.println("choose two modules to swap:");
    Serial.println("");
    listModules();
    Serial.printf("%d: exit\n", moduleCount);
    Serial.println("");

    uint8_t moduleX = serialInput("first module: ").toInt();
    uint8_t moduleY = serialInput("second module: ").toInt();
    swapModules(moduleX, moduleY);
  }
  else {
    Serial.printf("setup routine <%d> unknown\n", choice);
  }

  saveSettings();

  Serial.println("");
  Serial.println(centerString("",39,'='));
  Serial.println(centerString("setup complete", 39, ' '));
  Serial.println(centerString("",39,'='));
  Serial.println("");
  Serial.println("restarting now");
  ESP.restart();
}

void XRTL::stop(){
  debug("stopping all modules");

  for (int i = 0; i < moduleCount; i++) {
    module[i]->stop();
  }
}

void XRTL::getStatus(){
  DynamicJsonDocument doc(1024);
  JsonArray event = doc.to<JsonArray>();
  event.add("status");

  JsonObject payload = event.createNestedObject();
  payload["componentId"] = "null";
  JsonObject status = payload.createNestedObject("status");
  status["busy"] = false;
  
  for (int i = 0; i < moduleCount; i++) {
    module[i]->getStatus(payload, status);
  }

  if (socketIO == NULL) {
    debug("unable to send event: no endpoint module");
    return;
  }
  socketIO->sendEvent(event);
}

void XRTLmodule::sendStatus(){
  xrtl->getStatus();
}

void XRTL::sendEvent(JsonArray& event){
  if (socketIO == NULL) {
    debug("unable to send event: no endpoint module");
    return;
  }
  socketIO->sendEvent(event);
}

void XRTLmodule::sendEvent(JsonArray& event){
  xrtl->sendEvent(event);
}

void XRTL::sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length) {
  if (socketIO == NULL) {
    debug("unable to send event: no endpoint module");
  }
  socketIO->sendBinary(binaryLeadFrame, payload, length);
}

void XRTLmodule::sendBinary(String& binaryLeadFrame, uint8_t* payload, size_t length) {
  xrtl->sendBinary(binaryLeadFrame, payload, length);
}

void XRTL::notify(internalEvent event) {
  // notify modules
  for (int i = 0; i < moduleCount; i++) {
    module[i]->handleInternal(event);
  }

  // core event handler
  switch(event) {

  }
}

void XRTLmodule::notify(internalEvent state) {
  xrtl->notify(state);
}

String& XRTL::getComponent() {// TODO: deprecated?
  return socketIO->getComponent();
}

String& XRTLmodule::getComponent() {
  return xrtl->getComponent();
}

void XRTL::pushCommand(String& controlId, JsonObject& command){
  bool ret = false;

  for (int i = 0; i < moduleCount; i++) {
    ret = module[i]->handleCommand(controlId, command) or ret;
  }

  if (!ret){
    String error = "unknown controlId <";
    error += controlId;
    error += ">";
    sendError(unknown_key, error);
  }
}

void SocketModule::pushCommand(String& controlId, JsonObject& command) {
  xrtl->pushCommand(controlId, command);
}

void XRTL::pushCommand(String& command){
  bool ret = false;
  if ( (command == 0 ) or (strcmp(command.c_str(),"null") == 0) ) {
    sendError(field_is_null,"[core] command field is null");
    return;
  }

  if (strcmp(command.c_str(), "getStatus") == 0) {
    getStatus();
    return;
  }

  // offering command to modules, register if one or more respond true
  for (int i = 0; i < moduleCount; i++) {
    ret = module[i]->handleCommand(command) or ret;
  }

  // stuff happening after modules were informed
  if (strcmp(command.c_str(), "init") == 0) {
    saveSettings();
    ret = true;
  }

  if (strcmp(command.c_str(), "reset") == 0) {
    stop();
    saveSettings();
    ESP.restart();
  }

  if (!ret) {
    String error = "[core] unknown command: <";
    error += command;
    error += ">";
    sendError(unknown_key, error);
  }
}

void SocketModule::pushCommand(String& command) {
  xrtl->pushCommand(command);
}

void XRTL::sendError(componentError err, String msg) {
  if (socketIO == NULL) {
    debug("unable to send event: no endpoint module");
    return;
  }
  socketIO->sendError(err,msg);
}

void XRTLmodule::sendError(componentError err, String msg) {
  xrtl->sendError(err,msg);
}