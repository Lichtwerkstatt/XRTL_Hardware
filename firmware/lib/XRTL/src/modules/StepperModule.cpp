#include "StepperModule.h"
//when motor running: 
//infoLED.pulse(0, 40, 100);

StepperModule::StepperModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

void StepperModule::saveSettings(DynamicJsonDocument& settings) {
  JsonObject saving = settings.createNestedObject(id);
  saving["control"] = control;
  saving["accel"] = accel;
  saving["speed"] = speed;
  saving["position"] = stepper->currentPosition();
  saving["minimum"] = minimum;
  saving["maximum"] = maximum;
  saving["initial"] = initial;
  saving["relativeCtrl"] = relativeCtrl;
  saving["pin1"] = pin[0];
  saving["pin2"] = pin[1];
  saving["pin3"] = pin[2];
  saving["pin4"] = pin[3]; 
}

void StepperModule::loadSettings(DynamicJsonDocument& settings) {
  JsonObject loaded = settings[id];
  control = loaded["control"].as<String>();
  accel = loaded["accel"].as<uint16_t>();
  speed = loaded["speed"].as<uint16_t>();
  position = loaded["position"].as<int32_t>();
  minimum = loaded["minimum"].as<int32_t>();
  maximum = loaded["maximum"].as<int32_t>();
  initial = loaded["initial"].as<int32_t>();
  relativeCtrl = loaded["relativeCtrl"].as<bool>();

  pin[0] = loaded["pin1"].as<uint8_t>();
  pin[1] = loaded["pin2"].as<uint8_t>();
  pin[2] = loaded["pin3"].as<uint8_t>();
  pin[3] = loaded["pin4"].as<uint8_t>();

  //check validity where applicable, change violations to default
  if (!strcmp(control.c_str(),"null")) {control = id;}
  if (!accel) {accel = 500;}
  if (!speed) {speed = 500;}
  if (!pin[0]) {pin[0] = 19;}
  if (!pin[1]) {pin[1] = 22;}
  if (!pin[2]) {pin[2] = 21;}
  if (!pin[3]) {pin[3] = 23;}

  if (!debugging) return;

  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  Serial.printf("controlId: %s\n",control.c_str());
  Serial.printf("acceleration: %i\n", accel);
  Serial.printf("speed: %i\n", speed);
  Serial.printf("position: %i\n", position);
  Serial.printf("minimum: %i\n", minimum);
  Serial.printf("maximum: %i\n", maximum);
  Serial.printf("initial steps: %i\n", initial);

  if (relativeCtrl) {
    Serial.println("motor control is relative");
  }
  else {
    Serial.println("motor is step controled");
  }

  Serial.println("");

  Serial.print("pin 1: ");
  Serial.print(pin[0]);
  Serial.print("\n");

  Serial.print("pin 2: ");
  Serial.print(pin[1]);
  Serial.print("\n");

  Serial.print("pin 3: ");
  Serial.print(pin[2]);
  Serial.print("\n");

  Serial.print("pin 4: ");
  Serial.print(pin[3]);
  Serial.print("\n");
}

void StepperModule::setViaSerial() {
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  control = serialInput("controlId: ");
  accel = serialInput("acceleration (steps/s²): ").toInt();
  speed = serialInput("speed (steps/s): ").toInt();
  position = serialInput("current position (steps): ").toInt();
  minimum = serialInput("minimum position (steps): ").toInt();
  maximum = serialInput("maximum position (steps): ").toInt();
  initial = serialInput("initial steps (steps): ").toInt();
  relativeCtrl = (strcmp(serialInput("relative control (y/n): ").c_str(),"y") == 0);
  
  Serial.print("\n");

  if (strcmp(serialInput("change pin bindings (y/n)?   ").c_str(),"y") != 0) return;

  pin[0] = serialInput("pin 1: ").toInt();
  pin[1] = serialInput("pin 2: ").toInt();
  pin[2] = serialInput("pin 3: ").toInt();
  pin[3] = serialInput("pin 4: ").toInt();
}

void StepperModule::setup() {
  stepper = new AccelStepper(AccelStepper::HALF4WIRE, pin[0], pin[1], pin[2], pin[3]);
  stepper->setCurrentPosition(position);
  stepper->setMaxSpeed(speed);
  stepper->setAcceleration(accel);
  if (initial != 0) {
    stepper->move(initial);
    wasRunning = true;
    isInitialized = false;
    notify(busy);
  }
}

void StepperModule::loop() {
  if (stepper->isRunning()) {
    stepper->run();
  }
  else if (wasRunning) {
    stepper->disableOutputs();
    wasRunning = false;
    notify(ready);

    debug("[%s] done moving", id);
  }
}

void StepperModule::getStatus(JsonObject& payload, JsonObject& status) {
  if (stepper == NULL) return;// avoid errors: status might be called during setup

  if (stepper->isRunning()) {
    status["busy"] = true;
  }

  JsonObject position = status.createNestedObject(control);
  position["absolute"] = stepper->currentPosition();
  position["relative"] = mapFloat(stepper->currentPosition(), minimum, maximum, 0, 100);
  return;
}

void StepperModule::stop() {
  stepper->stop();
  while (stepper->isRunning()) {
    stepper->run();
    yield();
  }
  stepper->disableOutputs();
}

bool StepperModule::handleCommand(String& command){
  if (strcmp(command.c_str(),"stop") == 0) {
    stop();
    return true;
  }

  else if (strcmp(command.c_str(),"init") == 0) {
    stepper->setCurrentPosition(0);
    return true;
  }

  else if (strcmp(command.c_str(),"reset") == 0) {
    debug("[%s] reset: moving from %d to 0", id.c_str(), stepper->currentPosition());
    stepper->moveTo(0);
    stepper->enableOutputs();
    while (stepper->isRunning()) {
      stepper->run();
    }
    stepper->disableOutputs();
    debug("[%s] reset: done", id.c_str());
    return true;
  }

  return false;
}

bool StepperModule::handleCommand(String& controlId, JsonObject& command) {
  if (strcmp(control.c_str(),controlId.c_str()) != 0) return false;
  
  if (stepper->isRunning()) {
    String error = "[";
    error += id;
    error += "] command rejected: stepper already moving";
    sendError(is_busy, error);
    return true;
  }

  driveStepper(command);
  return true;
}

template<typename... Args>
void StepperModule::debug(Args... args) {
  if(!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};

void StepperModule::driveStepper(JsonObject& command) {
  auto valField = command["val"];
  if (valField.isNull()){
    sendError(field_is_null,"command rejected: <val> is NULL");
    return;
  }

  if ( !valField.is<int>() and !valField.is<float>() ) {
    sendError(wrong_type,"command rejected: <val> is neither int nor float");
    return;
  }

  // all functional checks passed, prepare for moving
  stepper->enableOutputs();

  if (relativeCtrl) {
    float val = valField.as<float>() / 100;//position in percent
    stepper->moveTo(minimum + round(val * (maximum - minimum) ) );
  }
  else {
    int val = valField.as<int>();
    stepper->move(val);
  }

  int32_t target = stepper->targetPosition();

  if ( (target > maximum) or (target < minimum) ) {
    target = constrain(target, minimum, maximum);
    stepper->moveTo(target);

    String error = "[";
    error += control;
    error += "] target position was constrained to (";
    error += minimum;
    error += ",";
    error += maximum;
    error += ")";

    sendError(out_of_bounds, error);
  }

  wasRunning = true;
  notify(busy);
  return;
}