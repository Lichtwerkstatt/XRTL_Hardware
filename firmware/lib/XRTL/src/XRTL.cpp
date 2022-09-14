#include "XRTL.h"

void XRTL::setup(){ 
  Serial.begin(115200);

  debug("[core] starting setup");

  loadConfig();
  loadSettings();

  for (int i = 0; i < moduleCount; i++){
    module[i]->setup();
  }
  
  debug("[%s] setup complete", "core");
}

void XRTL::loop(){
  if ( Serial.available() ) {
    String input = Serial.readStringUntil('\n');
    if ( strcmp(input.c_str(), "debug") == 0 ) {
      debugging = !debugging;
      if (debugging) notify(debug_on);
      else notify(debug_off);
    }

    if (debugging) {
      if (strcmp(input.c_str(),"setup") == 0){
        setViaSerial();
      }
      else if (strcmp(input.c_str(), "config") == 0) {
        editConfig();
      }
      else if (strcmp(input.c_str(), "debug") == 0) {} // do not interprete debug as event
      else {
        DynamicJsonDocument serialEvent(1024);
        DeserializationError error = deserializeJson(serialEvent,input);
        if (error) {
          Serial.printf("[core] deserializeJson() failed on serial input: %s\n", error.c_str());
          Serial.printf("[core] input: %s\n", input.c_str());
        }
        else if (socketIO != NULL) {
          socketIO->handleEvent(serialEvent);
        }
      }
    }
  }

  for (int i = 0; i < moduleCount; i++) {
    module[i]->loop();
  }
}

void XRTL::addModule(String moduleName, moduleType category) {
  switch(category) {
    case socket: {
      socketIO = new SocketModule(moduleName,this);
      module[moduleCount++] = socketIO;
      debug("[core] socket module added: <%s>", moduleName);
      return;
    }
    case wifi: {
      module[moduleCount++] = new WifiModule(moduleName,this);
      debug("[core] wifi module added: <%s>", moduleName);
      return;
    }
    case infoLED: {
      module[moduleCount++] = new InfoLEDModule(moduleName,this);
      debug("[core] infoLED module added: <%s>", moduleName);
      return;
    }
    case stepper: {
      module[moduleCount++] = new StepperModule(moduleName,this);
      debug("[core] stepper module added: <%s>", moduleName);
      return;
    }
    case servo: {
      module[moduleCount++] = new ServoModule(moduleName,this);
      debug("[core] servo module added: <%s>", moduleName);
      return;
    }
    case camera: {
      module[moduleCount++] = new CameraModule(moduleName,this);
      debug("[core] camera module added: <%s>", moduleName);
      return;
    }
    case output: {
      module[moduleCount++] = new OutputModule(moduleName,this);
      debug("[core] output module added: <%s>", moduleName);
      return;
    }
    case input: {
      module[moduleCount++] = new InputModule(moduleName,this);
      debug("[core] input module added: <%s>", moduleName);
      return;
    }
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
  DynamicJsonDocument doc(1024);

  for (int i = 0; i < moduleCount; i++) {
    module[i]->saveSettings(doc);
  }

  if (!LittleFS.begin(false)) {
    debug("[core] failed to mount LittleFS");
    debug("[core] trying to format LittleFS");
    if (!LittleFS.begin(true)) {
      debug("[core] failed to mount LittleFS again");
      debug("[core] unable to format LittleFS");
      debug("[core] unable to save settings");
      return;
    }
  }

  File file = LittleFS.open("/settings.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    debug("[core] failed to write file");
    debug("[core] could not save settings");
  }
  else {
    debug("[core] settings successfully saved");
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
    debug("[core] failed to mount LittleFS");
    debug("[core] trying to format LittleFS");
    if(!LittleFS.begin(true)) {
      debug("[core] failed to mount LittleFS again");
      debug("[core] unable to format LittleFS");
      debug("[core] could not load settings");
      return;
    }
    else {
      debug("[core] successfully formated file system");
      debug("[core] creating new settings file");

      saveSettings();
    }
  }

  File file = LittleFS.open("/settings.txt","r");
  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    debug("[core] deserializeJson() failed while loading settings: <%s>", error.c_str());
  }
  file.close();
  LittleFS.end();

  for (int i = 0; i < moduleCount; i++) {
    module[i]->loadSettings(doc);
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
  Serial.println("0: complete setup");
  for (int i = 0; i < moduleCount; i++) {
    Serial.printf("%i: %s\n",i+1,module[i]->getID().c_str());
  }
  Serial.print("\n");
  uint8_t choice = serialInput("choose setup routine: ").toInt();

  if (choice > 0) {
    module[choice-1]->setViaSerial();
  }
  else {
    for (int i = 0; i < moduleCount; i++) {
      module[i]->setViaSerial();
    }
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
  debug("[core] stopping all modules");

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

  socketIO->sendEvent(event);
}

void XRTLmodule::sendStatus(){
  xrtl->getStatus();
}

void XRTL::sendEvent(JsonArray& event){
  socketIO->sendEvent(event);
}

void XRTLmodule::sendEvent(JsonArray& event){
  xrtl->sendEvent(event);
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

void XRTL::pushCommand(JsonObject& command){
  bool ret = false;
  auto controlIdField = command["controlId"];

  if (!controlIdField.is<String>()) {
    sendError(wrong_type, "command rejected: <controlId> is no string");
    return;
  }

  String controlId = controlIdField.as<String>();
  if (strcmp(controlId.c_str(), "null") == 0) {
    sendError(field_is_null, "command rejected: <controlId> is null");
    return;
  }

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

void SocketModule::pushCommand(JsonObject& command) {
  xrtl->pushCommand(command);
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
  socketIO->sendError(err,msg);
}

void XRTLmodule::sendError(componentError err, String msg) {
  xrtl->sendError(err,msg);
}

void XRTL::loadConfig() {
  if (debugging) {
    Serial.println("");
    Serial.println(centerString("",39,'='));
    Serial.println(centerString("loading config",39,' '));
    Serial.println(centerString("",39,'='));
    Serial.println("");
  }
  
  if (!LittleFS.begin(false)) {
    debug("[core] failed to mount LittleFS");
    debug("[core] trying to format LittleFS");

    if(!LittleFS.begin(true)) {
      debug("[core] failed to mount LittleFS again");
      debug("[core] unable to format LittleFS");
      debug("[core] could not load config");
      return;
    }
    else {
      debug("[core] successfully formated file system");
      debug("[core] creating new config file");
    }
  }

  File file = LittleFS.open("/config.txt","r");
  DynamicJsonDocument doc(1024);
  DeserializationError error = deserializeJson(doc, file);
  if (error) {
    debug("[core] deserializeJson() failed while loading config: <%s>", error.c_str());
  }
  file.close();
  LittleFS.end();

  JsonObject config = doc.as<JsonObject>();

  for (JsonObject::iterator it=config.begin(); it!=config.end(); ++it) {
    addModule(it->key().c_str(), it->value());
  }

  debug("[core] config loaded, modules: %d", moduleCount);
}

void XRTL::editConfig() {
  Serial.println("");
  Serial.println(centerString("",39,'='));
  Serial.println(centerString("writing config",39,' '));
  Serial.println(centerString("",39,'='));
  Serial.println("");

  stop();

  Serial.println("module type is determined by number, available types:");
  for (int i = 0; i < 8; i++) {
    Serial.printf("%d: %s\n",i,moduleNames[i]);
  }

  DynamicJsonDocument doc(512);
  Serial.println("");
  Serial.println(centerString("",39,'-'));
  String input = serialInput("module name (\"exit\" to leave): ");
  while (strcmp(input.c_str(), "exit") != 0) {
    doc[input] = serialInput("module type: ").toInt();
    Serial.println(moduleNames[doc[input].as<int>()]);
    Serial.println(centerString("",39,'-'));
    Serial.println("");
    input = serialInput("module name (\"exit\" to leave): ");
  }

  if (!LittleFS.begin(false)) {
    debug("[core] failed to mount LittleFS");
    debug("[core] trying to format LittleFS");
    if (!LittleFS.begin(true)) {
      debug("[core] failed to mount LittleFS again");
      debug("[core] unable to format LittleFS");
      debug("[core] unable to save settings");
      return;
    }
  }

  File file = LittleFS.open("/config.txt", "w");
  if (serializeJsonPretty(doc, file) == 0) {
    debug("[core] failed to write file");
    debug("[core] could not save settings");
  }
  else {
    debug("[core] config successfully saved");
  }
  file.close();
  LittleFS.end();
  Serial.println("");
  Serial.println(centerString("",39,'='));
  Serial.println(centerString("config saved",39,' '));
  Serial.println(centerString("",39,'='));
}

template<typename... Args>
void XRTL::debug(Args... args) {
  if (!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};