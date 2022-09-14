#include "XRTLmodule.h"

String serialInput(String query) {
  Serial.print(query);
  while (!Serial.available()) {
    yield();
  }
  String answer = Serial.readStringUntil('\n');
  Serial.println(answer);
  return answer;
}

String centerString(String str, uint8_t targetLength, char filler = ' ') {
  char output[targetLength];
  
  uint8_t startPoint = (targetLength - str.length())/2-1;
  memset(output,filler,targetLength-1);
  output[targetLength-1] = '\0';
  memcpy(output+startPoint, str.c_str(), str.length());
  
  return String(output);
}

float mapFloat(float x, float in_min, float in_max, float out_min, float out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

String XRTLmodule::getID(){
  return id;
}

bool XRTLmodule::isModule(String& moduleName){
  return (strcmp(id.c_str(),moduleName.c_str()) == 0);
}

void XRTLmodule::setup(){
  return;
}

void XRTLmodule::loop(){
  return;
}

void XRTLmodule::getStatus(JsonObject& payload, JsonObject& status) {
  return;
}

void XRTLmodule::saveSettings(DynamicJsonDocument& settings) {
  return;
}

void XRTLmodule::loadSettings(DynamicJsonDocument& settings) {
  return;
}

void XRTLmodule::setViaSerial() {
  return;
}

void XRTLmodule::stop() {
  return;
}

void XRTLmodule::handleInternal(internalEvent event){
  switch(event) {
    case debug_off:
    {
      debugging = false;
      break;
    }
    case debug_on:
    {
      debugging = true;
      break;
    }
  }
}

//
bool XRTLmodule::handleCommand(String& controlId, JsonObject& command){
  return false;
}

bool XRTLmodule::handleCommand(String& command) {
  //Serial.printf("[%s] got command: %s\n", id.c_str(), command.c_str());
  return false;
}