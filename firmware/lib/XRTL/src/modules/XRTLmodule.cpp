#include "XRTLmodule.h"

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

void XRTLmodule::saveSettings(JsonObject& settings) {
  return;
}

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