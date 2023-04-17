#include "StepperModule.h"
// when motor running:
// infoLED.pulse(0, 40, 100);

StepperModule::StepperModule(String moduleName)
{
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(pin[0], "pin1", "int");
    parameters.add(pin[1], "pin2", "int");
    parameters.add(pin[2], "pin3", "int");
    parameters.add(pin[3], "pin4", "int");
    parameters.add(accel, "accel", "steps/s²");
    parameters.add(speed, "speed", "steps/s");
    parameters.add(position, "position", "int");
    parameters.add(minimum, "minimum", "int");
    parameters.add(maximum, "maximum", "int");
    parameters.add(initial, "initial", "int");
    parameters.add(infoLED, "infoLED", "String");
}

moduleType StepperModule::getType()
{
    return xrtl_stepper;
}

void StepperModule::saveSettings(JsonObject &settings)
{
    // JsonObject saving = settings.createNestedObject(id);
    /*
    settings["accel"] = accel;
    settings["speed"] = speed;
    if (stepper == NULL) {
      settings["position"] = position;
    }
    else {
      settings["position"] = stepper->currentPosition();
    }
    settings["minimum"] = minimum;
    settings["maximum"] = maximum;
    settings["initial"] = initial;
    settings["pin1"] = pin[0];
    settings["pin2"] = pin[1];
    settings["pin3"] = pin[2];
    settings["pin4"] = pin[3];*/

    parameters.save(settings);
}

void StepperModule::loadSettings(JsonObject &settings)
{
    /*accel = loadValue<uint16_t>("accel", settings, 500);
    speed = loadValue<uint16_t>("speed", settings, 500);
    position = loadValue<int32_t>("position", settings, 0);
    minimum = loadValue<int32_t>("minimum", settings, -2048);
    maximum = loadValue<int32_t>("maximum", settings, 2048);
    initial = loadValue<int32_t>("initial", settings, 0);

    pin[0] = loadValue<uint8_t>("pin1", settings, 19);
    pin[1] = loadValue<uint8_t>("pin2", settings, 22);
    pin[2] = loadValue<uint8_t>("pin3", settings, 21);
    pin[3] = loadValue<uint8_t>("pin4", settings, 23);

    if (!debugging) return;

    Serial.printf("controlId: %s\n",id.c_str());
    Serial.printf("acceleration: %i\n", accel);
    Serial.printf("speed: %i\n", speed);
    Serial.printf("position: %i\n", position);
    Serial.printf("minimum: %i\n", minimum);
    Serial.printf("maximum: %i\n", maximum);
    Serial.printf("initial steps: %i\n", initial);

    Serial.println("");

    Serial.printf("pin 1: %d\n", pin[0]);
    Serial.printf("pin 2: %d\n", pin[1]);
    Serial.printf("pin 3: %d\n", pin[2]);
    Serial.printf("pin 4: %d\n", pin[3]);*/
    parameters.load(settings);
    if (debugging)
        parameters.print();
}

void StepperModule::setViaSerial()
{
    /*Serial.println("");
    Serial.println(centerString("",39, '-'));
    Serial.println(centerString(id,39, ' '));
    Serial.println(centerString("",39, '-'));
    Serial.println("");

    id = serialInput("controlId: ");
    accel = serialInput("acceleration (steps/s²): ").toInt();
    speed = serialInput("speed (steps/s): ").toInt();
    position = serialInput("current position (steps): ").toInt();
    stepper->setCurrentPosition(position);
    minimum = serialInput("minimum position (steps): ").toInt();
    maximum = serialInput("maximum position (steps): ").toInt();
    initial = serialInput("initial steps (steps): ").toInt();

    Serial.print("\n");

    if ( serialInput("change pin bindings (y/n): ") != "y" ) return;

    pin[0] = serialInput("pin 1: ").toInt();
    pin[1] = serialInput("pin 2: ").toInt();
    pin[2] = serialInput("pin 3: ").toInt();
    pin[3] = serialInput("pin 4: ").toInt();*/
    parameters.setViaSerial();
}

void StepperModule::setup()
{
    stepper = new AccelStepper(AccelStepper::HALF4WIRE, pin[0], pin[1], pin[2], pin[3]);
    stepper->setCurrentPosition(position);
    stepper->setMaxSpeed(speed);
    stepper->setAcceleration(accel);
    if (initial != 0)
    {
        stepper->move(initial);
        wasRunning = true;
        isInitialized = false;
        notify(busy);
    }
}

void StepperModule::loop()
{
    if (stepper->isRunning())
    {
        stepper->run();
    }
    else if (wasRunning)
    {
        if (!holdOn)
            stepper->disableOutputs();

        if (infoLED != "")
        {
            XRTLdisposableCommand command(infoLED);
            command.add("hold", false);
            sendCommand(command);
        }

        wasRunning = false;

        debug("done moving");
        sendStatus();
        notify(ready);
    }
}

bool StepperModule::getStatus(JsonObject &status)
{
    if (stepper == NULL)
        return true; // avoid errors: status might be called during setup

    status["busy"] = stepper->isRunning();
    status["absolute"] = stepper->currentPosition();
    status["relative"] = mapFloat(stepper->currentPosition(), minimum, maximum, 0, 100);
    return true;
}

void StepperModule::stop()
{
    stepper->stop();
    while (stepper->isRunning())
    {
        stepper->run();
        yield();
    }
    stepper->disableOutputs();
}

bool StepperModule::handleCommand(String &command)
{
    if (command == "stop")
    {
        stop();
        return true;
    }

    else if (command == "init")
    {
        stepper->setCurrentPosition(0);
        return true;
    }

    else if (command == "reset")
    {
        debug("reset: moving from %d to 0", stepper->currentPosition());
        stepper->moveTo(0);
        stepper->enableOutputs();
        while (stepper->isRunning())
        {
            stepper->run();
        }
        stepper->disableOutputs();
        debug("reset: done");
        return true;
    }

    return false;
}

void StepperModule::handleCommand(String &controlId, JsonObject &command)
{
    if (!isModule(controlId))
        return;

    bool getStatus = false;
    if (getValue<bool>("getStatus", command, getStatus) && getStatus)
    {
        sendStatus();
    }

    if (getValue<bool>("stop", command, getStatus) && getStatus)
    {
        stop();
    }

    if (getValue<bool>("reset", command, getStatus) && getStatus)
    {
        debug("reset: moving from %d to 0", stepper->currentPosition());
        stepper->moveTo(0);
        stepper->enableOutputs();
        while (stepper->isRunning())
        {
            stepper->run();
        }
        stepper->disableOutputs();
        debug("reset: done");
    }

    if (stepper->isRunning())
    {
        String error = "[";
        error += id;
        error += "] command rejected: stepper already moving";
        sendError(is_busy, error);
        return;
    }

    driveStepper(command);
    return;
}

void StepperModule::driveStepper(JsonObject &command)
{
    int32_t moveValue = 0;
    if (getAndConstrainValue<int32_t>("move", command, moveValue, minimum - maximum, maximum - minimum))
    { // full range: maximum - minimum; negative range: minimum - maximum
        stepper->move(moveValue);
    }

    if (getAndConstrainValue<int32_t>("moveTo", command, moveValue, minimum, maximum))
    {
        stepper->moveTo(moveValue);
    }

    int32_t target = stepper->targetPosition();

    if ((target > maximum) or (target < minimum))
    {
        target = constrain(target, minimum, maximum);
        stepper->moveTo(target);

        String error = "[";
        error += id;
        error += "] target position was constrained to (";
        error += minimum;
        error += ",";
        error += maximum;
        error += ")";

        sendError(out_of_bounds, error);
    }

    bool binaryCtrl;
    if (getValue<bool>("binaryCtrl", command, binaryCtrl))
    {
        if (binaryCtrl)
        {
            stepper->moveTo(maximum);
        }
        else
        {
            stepper->moveTo(minimum);
        }
    }

    if (getValue<bool>("hold", command, holdOn))
    {
        debug("hold %sactive", holdOn ? "" : "in");
    }

    if (!stepper->isRunning()) // check whether position is reached already
    {
        wasRunning = true;
        return;
    }

    // estimate travelTime in ms
    uint32_t distance = abs(stepper->targetPosition() - stepper->currentPosition());
    uint32_t travelTime = 0;
    if (distance > speed * speed / accel)
    {
        travelTime = 1000 * distance / speed;
        travelTime += 1000 * speed / accel;
    }
    else
    {
        travelTime = round(2000 * sqrt(distance / accel));
    }

    if (infoLED != "")
    {
        XRTLdisposableCommand ledCommand(infoLED);

        String color;
        if (getValue("color", command, color))
        {
            ledCommand.add("color", color);
        }

        ledCommand.add("hold", true);
        ledCommand.add("cycle", (int) travelTime);
        sendCommand(ledCommand);
    }

    stepper->enableOutputs();
    debug("moving from %d to %d", stepper->currentPosition(), stepper->targetPosition());
    wasRunning = true;
    sendStatus();
    notify(busy);
    return;
}