#include "ServoModule.h"

ServoModule::ServoModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

moduleType ServoModule::getType() {
  return xrtl_servo;
}

void ServoModule::saveSettings(JsonObject& settings){
  //JsonObject saving = settings.createNestedObject(id);
  settings["frequency"] = frequency;
  settings["minDuty"] = minDuty;
  settings["maxDuty"] = maxDuty;
  settings["minAngle"] = minAngle;
  settings["maxAngle"] = maxAngle;
  settings["initial"] = initial;
  settings["relativeCtrl"] = relativeCtrl;
  settings["pin"] = pin;
}

void ServoModule::loadSettings(JsonObject& settings){
  //JsonObject loaded = settings[id];

  frequency = loadValue<uint16_t>("frequency", settings, 50);
  minDuty = loadValue<uint16_t>("minDuty", settings, 1000);
  maxDuty = loadValue<uint16_t>("maxDuty", settings, 2000);
  minAngle = loadValue<int16_t>("minAngle", settings, 0);
  maxAngle = loadValue<int16_t>("maxAngle", settings, 90);
  initial = loadValue<int16_t>("initial", settings, 0);
  relativeCtrl = loadValue<bool>("relativeCtrl", settings, 0);
  pin = loadValue<uint8_t>("pin", settings, 32);
  
  if (!debugging) return;

  Serial.printf("controlId: %s\n", id.c_str());
  Serial.printf("frequency: %d Hz\n", frequency);
  Serial.printf("minimum duty time: %d µs\n", minDuty);
  Serial.printf("maximum duty time: %d µs\n", maxDuty);
  Serial.printf("minimum angle: %d\n", minAngle);
  Serial.printf("maximum angle: %d\n", maxAngle);
  Serial.printf("initial position: %d\n", initial);

  Serial.println("");
  Serial.printf(relativeCtrl ? "servo controle is relative\n" : "servo controle is absolute\n");

  Serial.println("");
  Serial.printf("control pin: %d\n", pin);
}

void ServoModule::setViaSerial(){
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  id = serialInput("controlId: ");
  frequency = serialInput("Frequency (Hz): ").toInt();
  minDuty = serialInput("minimum duty time (µs): ").toInt();
  maxDuty = serialInput("maximum tuty time (µs): ").toInt();
  minAngle = serialInput("minimum angle (deg): ").toInt();
  maxAngle = serialInput("maximum angle (deg): ").toInt();
  initial = serialInput("initial angle (deg): ").toInt();
  relativeCtrl = ( serialInput("relative control (y/n): ") == "y" );
  Serial.println("");

  if ( serialInput("change pin binding (y/n): ") == "y" ) {
    pin = serialInput("pin: ").toInt();
  }
}

void ServoModule::getStatus(JsonObject& payload, JsonObject& status){
  JsonObject position = status.createNestedObject(id);

  int16_t angle = read();
  position["absolute"] = angle;
  position["relative"] = mapFloat(angle,minAngle,maxAngle,0,100);
}

void ServoModule::setup(){
  servo = new Servo;
  servo->setPeriodHertz(frequency);
  servo->attach(pin,minDuty,maxDuty);
  write(initial);
}

void ServoModule::loop(){
  return;
}

bool ServoModule::handleCommand(String& command) {
  if (strcmp(command.c_str(), "init") == 0) {
    initial = read();
    return true;
  }

  if (strcmp(command.c_str(), "reset") == 0) {
    write(initial);
    return true;
  }

  return false;
}

bool ServoModule::handleCommand(String& controlId, JsonObject& command) {
  if (!isModule(controlId)) return false;

  driveServo(command);
  return true;
}

int16_t ServoModule::read() {
  if (servo == NULL) return 0;

  return round(mapFloat(servo->readMicroseconds(), minDuty, maxDuty, minAngle, maxAngle));
}

void ServoModule::write(int16_t target) {
  if (servo == NULL) return;

  servo->writeMicroseconds(round(mapFloat(target,minAngle,maxAngle,minDuty,maxDuty)));
}

void ServoModule::driveServo(JsonObject& command) {

  int16_t target;
  float dummy;
  switch(getValue<int16_t,float>("val", command, target, dummy)) {
    case is_second: {
      target = (int16_t) round(dummy);
    }
    case is_wrong_type: {
      return;
    }
  }

  if (relativeCtrl) {
    target += read();
  }

  if ( (target < minAngle) or (target > maxAngle) ) {
    target = constrain(target,minAngle,maxAngle);

    String error = "[";
    error += id;
    error += "] target position was constrained to (";
    error += minAngle;
    error += ",";
    error += maxAngle;
    error += ")";

    sendError(out_of_bounds,error);
  }

  write(target);
  sendStatus();
}