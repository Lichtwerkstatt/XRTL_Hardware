#include "StepperModule.h"
//when motor running: 
//infoLED.pulse(0, 40, 100);

StepperModule::StepperModule(String moduleName, XRTL* source) {
  id = moduleName;
  xrtl = source;
}

void StepperModule::saveSettings(DynamicJsonDocument& settings) {
  JsonObject saving = settings.createNestedObject(id);
  
  saving["controlId"] = controlId;
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

  controlId = loadValue<String>("controlId", loaded, id);
  accel = loadValue<uint16_t>("accel", loaded, 500);
  speed = loadValue<uint16_t>("speed", loaded, 500);
  position = loadValue<int32_t>("position", loaded, 0);
  minimum = loadValue<int32_t>("minimum", loaded, -2048);
  maximum = loadValue<int32_t>("maximum", loaded, 2048);
  initial = loadValue<int32_t>("initial", loaded, 0);
  relativeCtrl = loadValue<bool>("relativeCtrl", loaded, false);

  pin[0] = loadValue<uint8_t>("pin1", loaded, 19);
  pin[1] = loadValue<uint8_t>("pin2", loaded, 22);
  pin[2] = loadValue<uint8_t>("pin3", loaded, 21);
  pin[3] = loadValue<uint8_t>("pin4", loaded, 23);

  if (!debugging) return;

  Serial.printf("controlId: %s\n",controlId.c_str());
  Serial.printf("acceleration: %i\n", accel);
  Serial.printf("speed: %i\n", speed);
  Serial.printf("position: %i\n", position);
  Serial.printf("minimum: %i\n", minimum);
  Serial.printf("maximum: %i\n", maximum);
  Serial.printf("initial steps: %i\n", initial);

  Serial.println("");
  Serial.printf(relativeCtrl ? "motor control is relative\n" : "motor controle is absolute\n");
  Serial.println("");

  Serial.printf("pin 1: %d\n", pin[0]);
  Serial.printf("pin 2: %d\n", pin[1]);
  Serial.printf("pin 3: %d\n", pin[2]);
  Serial.printf("pin 4: %d\n", pin[3]);
}

void StepperModule::setViaSerial() {
  Serial.println("");
  Serial.println(centerString("",39, '-'));
  Serial.println(centerString(id,39, ' '));
  Serial.println(centerString("",39, '-'));
  Serial.println("");

  controlId = serialInput("controlId: ");
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

    debug("done moving");
  }
}

void StepperModule::getStatus(JsonObject& payload, JsonObject& status) {
  if (stepper == NULL) return;// avoid errors: status might be called during setup

  if (stepper->isRunning()) {
    status["busy"] = true;
  }

  JsonObject position = status.createNestedObject(controlId);
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
    debug("reset: moving from %d to 0", stepper->currentPosition());
    stepper->moveTo(0);
    stepper->enableOutputs();
    while (stepper->isRunning()) {
      stepper->run();
    }
    stepper->disableOutputs();
    debug("reset: done");
    return true;
  }

  return false;
}

bool StepperModule::handleCommand(String& control, JsonObject& command) {
  if (strcmp(control.c_str(),controlId.c_str()) != 0) return false;
  
  if (stepper->isRunning()) {
    String error = "[";
    error += controlId;
    error += "] command rejected: stepper already moving";
    sendError(is_busy, error);
    return true;
  }

  driveStepper(command);
  return true;
}

void StepperModule::driveStepper(JsonObject& command) {
  
  if (relativeCtrl) {
    float val;

    if ( !getValue<float, int>("val", command, val, true) ) return;
    stepper->enableOutputs();
    stepper->moveTo(minimum + round(val * (maximum - minimum) ) );
  }
  else {
    int val;

    if ( !getValue<int,float>("val", command, val, true) ) return;
    stepper->enableOutputs();
    stepper->move(val);
  }

  int32_t target = stepper->targetPosition();

  if ( (target > maximum) or (target < minimum) ) {
    target = constrain(target, minimum, maximum);
    stepper->moveTo(target);

    String error = "[";
    error += controlId;
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