#include "ServoModule.h"

ServoModule::ServoModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

void ServoModule::saveSettings(DynamicJsonDocument& settings){
  JsonObject saving = settings.createNestedObject(id);
  saving["control"] = control;
  saving["frequency"] = frequency;
  saving["minDuty"] = minDuty;
  saving["maxDuty"] = maxDuty;
  saving["minAngle"] = minAngle;
  saving["maxAngle"] = maxAngle;
  saving["initial"] = initial;
  saving["relativeCtrl"] = relativeCtrl;
  saving["pin"] = pin;
}

void ServoModule::loadSettings(DynamicJsonDocument& settings){
  JsonObject loaded = settings[id];
  control = loaded["control"].as<String>();
  frequency = loaded["frequency"].as<uint16_t>();
  minDuty = loaded["minDuty"].as<uint16_t>();
  maxDuty = loaded["maxDuty"].as<uint16_t>();
  minAngle = loaded["minAngle"].as<int16_t>();
  maxAngle = loaded["maxAngle"].as<int16_t>();
  initial = loaded["initial"].as<int16_t>();
  relativeCtrl = loaded["relativeCtrl"].as<bool>();
  pin = loaded["pin"].as<uint8_t>();

  if (pin < 1) {
    pin = 32;
  }
  
  if (!debugging) return;

  Serial.println("");
  Serial.println(centerString("",39,'-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  Serial.print("controlId: ");
  Serial.println(control.c_str());

  Serial.print("frequency: ");
  Serial.println(frequency);

  Serial.print("minmum duty time: ");
  Serial.println(minDuty);

  Serial.print("maximum duty time: ");
  Serial.println(maxDuty);

  Serial.print("minmum angle: ");
  Serial.println(minAngle);

  Serial.print("maximum angle: ");
  Serial.println(maxAngle);

  Serial.print("initial position: ");
  Serial.println(initial);

  if (relativeCtrl) {
    Serial.println("servo control is relative");
  }
  else {
    Serial.println("servo controled by angle");
  }

  Serial.println("");
  Serial.print("control pin: ");
  Serial.println(pin);
}

void ServoModule::setViaSerial(){
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  control = serialInput("controlId: ");
  frequency = serialInput("Frequency (Hz): ").toInt();
  minDuty = serialInput("minimum duty time (µs): ").toInt();
  maxDuty = serialInput("maximum tuty time (µs): ").toInt();
  minAngle = serialInput("minimum angle (deg): ").toInt();
  maxAngle = serialInput("maximum angle (deg): ").toInt();
  initial = serialInput("initial angle (deg): ").toInt();
  relativeCtrl = (strcmp(serialInput("relative control (y/n): ").c_str(),"y") == 0);
  Serial.println("");

  if (strcmp(serialInput("change pin binding (y/n)?   ").c_str(),"y") == 0) {
    pin = serialInput("pin: ").toInt();
  }
}

void ServoModule::getStatus(JsonObject& payload, JsonObject& status){
  JsonObject position = status.createNestedObject(control);

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
  if (strcmp(controlId.c_str(),control.c_str()) == 0) {
    driveServo(command);
    return true;
  }

  return false;
}

int16_t ServoModule::read() {
  if (servo == NULL) {
    return 0;
  }
  return round(mapFloat(servo->readMicroseconds(), minDuty, maxDuty, minAngle, maxAngle));
}

void ServoModule::write(int16_t target) {
  if (servo == NULL) {
    return;
  }
  servo->writeMicroseconds(round(mapFloat(target,minAngle,maxAngle,minDuty,maxDuty)));
}

void ServoModule::driveServo(JsonObject& command) {
  auto valField = command["val"];
  if ( (!valField.is<String>()) and (!valField.is<float>()) ) {
    sendError(wrong_type, "command rejected: <val> is neither int nor float");
    return;
  }

  int16_t target = valField.as<int>();
  if (relativeCtrl) {
    target += read();
  }

  if ( (target < minAngle) or (target > maxAngle) ) {
    target = constrain(target,minAngle,maxAngle);

    String error = "[";
    error += control;
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

template<typename... Args>
void ServoModule::debug(Args... args) {
  if(!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};