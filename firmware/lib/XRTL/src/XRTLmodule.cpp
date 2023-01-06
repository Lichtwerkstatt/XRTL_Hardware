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

moduleType XRTLmodule::getType() {
  return xrtl_socket;// TODO: add "xrtl_none" and set as default, adjust all setting handling functions accordingly 
}

bool XRTLmodule::isModule(String& moduleName){
  return ( id == moduleName);
}

void XRTLmodule::setup(){
  return;
}

void XRTLmodule::loop(){
  return;
}

bool XRTLmodule::getStatus(JsonObject& status) {
  return false;
}

//void XRTLmodule::saveSettings(DynamicJsonDocument& settings) {
void XRTLmodule::saveSettings(JsonObject& settings) {
  return;
}

//void XRTLmodule::loadSettings(DynamicJsonDocument& settings) {
void XRTLmodule::loadSettings(JsonObject& settings) {
  return;
}

void XRTLmodule::setViaSerial() {
  return;
}

void XRTLmodule::stop() {
  return;
}

void XRTLmodule::handleInternal(internalEvent eventId, String& sourceId){
  switch(eventId) {
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
void XRTLmodule::handleCommand(String& controlId, JsonObject& command){
  return;
}

bool XRTLmodule::handleCommand(String& command) {
  //Serial.printf("[%s] got command: %s\n", id.c_str(), command.c_str());
  return false;
}