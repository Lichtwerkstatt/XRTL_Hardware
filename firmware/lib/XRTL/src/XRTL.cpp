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
    if ( input == "debug" ) {
      debugging = !debugging;
      if (debugging) notify(debug_on, id);
      else notify(debug_off, id);
    }

    //only allow inputs if debugging
    if (debugging) {
      if ( input == "setup" ){
        setViaSerial();
      }
      else if ( input == "debug" ) {} // do not interprete debug as event
      else {
        DynamicJsonDocument serialEvent(1024);
        DeserializationError error = deserializeJson(serialEvent,input);
        if (error) {
          Serial.printf("[%s] deserializeJson() failed on serial input: %s\n", id.c_str(), error.c_str());
          Serial.printf("[%s] input: %s\n", id.c_str(), input.c_str());
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
  if (moduleCount == 16) {
    debug("unable to add module: maximum number of modules reached");
    return;
  }
  if ( (moduleName == 0) or (moduleName == "")) {
    debug("unable to add module: ID must not be empty");
    return;
  }
  if (this->operator[](moduleName) != NULL) {
    debug("unable to add module <%s>: ID already in use", moduleName);
    return;  
  }
  if ( (moduleName == "*") or (moduleName == "none") or (moduleName == "core") ) {
    debug("unable to add module <%s>: ID restricted to internal use", moduleName);
    return;
  }
  switch(category) {
    case xrtl_socket: {
      socketIO = new SocketModule(moduleName,this);
      module[moduleCount++] = socketIO;
      debug("socket module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_wifi: {
      module[moduleCount++] = new WifiModule(moduleName,this);
      debug("wifi module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_infoLED: {
      module[moduleCount++] = new InfoLEDModule(moduleName,this);
      debug("infoLED module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_stepper: {
      module[moduleCount++] = new StepperModule(moduleName,this);
      debug("stepper module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_servo: {
      module[moduleCount++] = new ServoModule(moduleName,this);
      debug("servo module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_camera: {
      module[moduleCount++] = new CameraModule(moduleName,this);
      debug("camera module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_output: {
      module[moduleCount++] = new OutputModule(moduleName,this);
      debug("output module added: <%s>", moduleName.c_str());
      return;
    }
    case xrtl_input: {
      module[moduleCount++] = new InputModule(moduleName,this);
      debug("input module added: <%s>", moduleName.c_str());
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
    Serial.printf("[%s] deleting <%s> ... ", id.c_str(), module[number]->getID().c_str());
    for (int i = number; i < moduleCount - 1; i++) {
      module[i] = module[i+1];
    }
    moduleCount--;
    Serial.println("done");
  }// any number not in range defaults as exit methode
}

void XRTL::swapModules(uint8_t numberX, uint8_t numberY) {
  if ( (numberX != numberY)
      and (numberX >= 0) and (numberX <= moduleCount)
      and (numberY >= 0) and (numberY <= moduleCount) ) {
        XRTLmodule* temp = module[numberX];
        module[numberX] = module[numberY];
        module[numberY] = temp;

        debug("swapped <%s> and <%s>", module[numberX]->getID().c_str(), module[numberY]->getID().c_str());
      }
  
}

XRTLmodule* XRTL::operator[](String moduleName) {
  for (int i = 0; i < moduleCount; i++) {
    if (module[i]->isModule(moduleName)) {
      return module[i];
    }
  }
  return NULL;// probably not a good idea as default, return dummy module instead?
}

void XRTL::saveSettings() {
  DynamicJsonDocument doc(2048);

  for (int i = 0; i < moduleCount; i++) {
    JsonObject settings = doc.createNestedObject(module[i]->getID());
    settings["type"] = module[i]->getType();
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

void XRTL::settingsDialogue() {
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString("available settings", 39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");
  listModules();
  Serial.println("");
  Serial.println("a: add module");
  Serial.println("d: delete module");
  Serial.println("s: swap modules");
  Serial.println("r: return");
  Serial.println("");
  String choice = serialInput("choose setup routine: ");
  uint8_t choiceInt = choice.toInt();
  Serial.println("");
  Serial.println(centerString("", 39, '-'));

  if (choice == "a") {
    Serial.println(centerString("add module", 39, ' '));
    Serial.println(centerString("", 39, '-'));
    Serial.println("");
    Serial.println("module type is determined by number, available types:");
    Serial.println("");
    for (int i = 0; i < 8; i++) {
      Serial.printf("%d: %s\n",i,moduleNames[i]);
    }
    Serial.println("");
    Serial.println("r: return");
    Serial.println("");

    choice.clear();
    choice = serialInput("new module type: ");
    choiceInt = choice.toInt();

    if (choice != "r" && choiceInt < 8) {
      moduleType newModuleType = (moduleType) choiceInt;
      String newModuleName = serialInput("new module name: ");
      addModule(newModuleName, newModuleType);

      JsonObject emptySettings;
      module[moduleCount - 1]->loadSettings(emptySettings); // initialize with default parameters
      module[moduleCount - 1]->setup(); // run setup if necessary
    }
  }
  else if (choice == "d") {
    Serial.println(centerString("remove module", 39, ' '));
    Serial.println(centerString("", 39, '-'));
    Serial.println("");
    Serial.println("choose module to delete: ");
    Serial.println("");
    listModules();
    Serial.println("");
    Serial.println("r: return");
    Serial.println("");

    choice.clear();
    choice = serialInput("delete: ");

    if (choice != "r") {
      uint8_t deleteChoice = choice.toInt();
      delModule(deleteChoice);
    }
  }
  else if (choice == "s") {
    Serial.println(centerString("swap modules", 39, ' '));
    Serial.println(centerString("", 39, '-'));
    Serial.println("choose two modules to swap:");
    Serial.println("");
    listModules();
    Serial.println("");
    Serial.println("r: exit");
    Serial.println("");

    String firstModule = serialInput("first module: ");

    if (firstModule != "r") {
      String secondModule = serialInput("second module: ");
      if (secondModule != "r") {
        swapModules((uint8_t) firstModule.toInt(), (uint8_t) secondModule.toInt());
      }
    }
  }
  else if (choice == "r") {
    Serial.println("returning");
  }
  else if (choiceInt < moduleCount) {
    module[choiceInt]->setViaSerial();
  }
  else {
    Serial.printf("setup routine <%s> unknown\n", choice);
  }
}

void XRTL::setViaSerial() {
  stop();

  Serial.println("");
  Serial.println(centerString("",39,'='));
  Serial.println(centerString("serial setup",39,' '));
  Serial.println(centerString("",39,'='));
  Serial.println("");

  settingsDialogue(); 
  while (serialInput("more changes? (y/n): ") == "y") {
    settingsDialogue();
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

/*void XRTL::getStatus(){
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
}*/

void XRTLmodule::sendStatus(){
  DynamicJsonDocument doc(1024);
  JsonArray event = doc.to<JsonArray>();
  event.add("status");

  JsonObject payload = event.createNestedObject();
  payload["controlId"] = id;
  
  JsonObject status = payload.createNestedObject("status");
  if (!getStatus(status)) return;
  xrtl->sendEvent(event);
}

void XRTL::sendStatus(){
  for (int i = 0; i < moduleCount; i++) {
    module[i]->sendStatus();
  }
}

void SocketModule::sendAllStatus() {
  xrtl->sendStatus();
}

/*void XRTLmodule::sendStatus(){
  xrtl->getStatus();
}*/

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

void XRTL::notify(internalEvent eventId, String& sourceId) {
  // notify modules
  for (int i = 0; i < moduleCount; i++) {
    module[i]->handleInternal(eventId, sourceId);
  }

  // core event handler
  switch(eventId) {

  }
}

void XRTLmodule::notify(internalEvent state) {
  xrtl->notify(state, id);
}

String& XRTL::getComponent() {
  if (socketIO == NULL) {
    debug("WARNING: no socket module present");
    return id;
  }
  return socketIO->getComponent();
}

String& XRTLmodule::getComponent() {
  return xrtl->getComponent();
}

void XRTL::pushCommand(String& controlId, JsonObject& command){
  //bool ret = false;

  for (int i = 0; i < moduleCount; i++) {
    module[i]->handleCommand(controlId, command);
  }

  /*if (!ret){
    String error = "unknown controlId <";
    error += controlId;
    error += ">";
    sendError(unknown_key, error);
  }*/
}

void SocketModule::pushCommand(String& controlId, JsonObject& command) {
  xrtl->pushCommand(controlId, command);
}

void XRTL::pushCommand(String& command){
  bool ret = false;
  if ( (command == 0 ) or (command == "null") or (command == "") ) {
    String errormsg = "[";
    errormsg += id;
    errormsg += "] command field is null";
    sendError(field_is_null, errormsg);
    return;
  }

  /*if ( command == "getStatus" ) {
    getStatus();
    return;
  }*/

  // offering command to modules, register if one or more respond true
  for (int i = 0; i < moduleCount; i++) {
    ret = module[i]->handleCommand(command) or ret;
  }

  // stuff happening after modules were informed
  if ( command == "init" ) {
    saveSettings();
    ret = true;
  }

  if ( command == "reset" ) {
    stop();
    saveSettings();
    ESP.restart();
  }

  if (!ret) {
    String error = "[";
    error += id;
    error += "] unknown command: <";
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