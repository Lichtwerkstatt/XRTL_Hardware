#include "StepperModule.h"

StepperModule::StepperModule(String moduleName) {
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

moduleType StepperModule::getType() {
    return xrtl_stepper;
}

void StepperModule::saveSettings(JsonObject &settings) {
    position = stepper->currentPosition(); // update position

    parameters.save(settings);
}

void StepperModule::loadSettings(JsonObject &settings) {
    parameters.load(settings);

    if (debugging)
        parameters.print();
}

void StepperModule::setViaSerial() {
    parameters.setViaSerial();
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
    } else if (wasRunning) {
        if (!holdOn)
            stepper->disableOutputs();

        if (infoLED != "") {
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

bool StepperModule::getStatus(JsonObject &status) {
    if (stepper == NULL)
        return true; // avoid errors: status might be called during setup

    position = stepper->currentPosition(); // update position

    status["busy"] = stepper->isRunning();
    status["absolute"] = position;
    status["relative"] = mapFloat(position, minimum, maximum, 0, 100);
    return true;
}

void StepperModule::stop() {
    stepper->stop();
    while (stepper->isRunning()) // allow deceleration
    {
        stepper->run();
        yield();
    }
    stepper->disableOutputs();
}

void StepperModule::handleCommand(String &controlId, JsonObject &command) {
    if (!isModule(controlId) && controlId != "*")
        return;

    bool getStatus = false;
    if (getValue<bool>("getStatus", command, getStatus) && getStatus) {
        sendStatus();
    }

    if (getValue<bool>("stop", command, getStatus) && getStatus) {
        stop();
    }

    if (getValue<bool>("reset", command, getStatus) && getStatus) {
        debug("reset: moving from %d to 0", stepper->currentPosition());
        stepper->moveTo(0);
        stepper->enableOutputs();
        while (stepper->isRunning()) {
            stepper->run();
        }
        stepper->disableOutputs();
        debug("reset: done");
    }

    if (stepper->isRunning()) {
        String error = "[";
        error += id;
        error += "] command rejected: stepper already moving";
        sendError(is_busy, error);
        return;
    }

    driveStepper(command);

    uint32_t duration;
    if (infoLED != "" && getValue("identify", command, duration)) {
        XRTLdisposableCommand ledCommand(infoLED);

        String color;
        if (getValue("color", command, color)) {
            ledCommand.add("color", color);
        }

        debug("identifying via LED <%s>", infoLED);
        ledCommand.add("pulse", (int)duration);
        sendCommand(ledCommand);
    }
}

void StepperModule::driveStepper(JsonObject &command) {
    int32_t moveValue = 0;
    if (getAndConstrainValue<int32_t>("move", command, moveValue, minimum - maximum, maximum - minimum)) { // full range: maximum - minimum; negative range: minimum - maximum
        stepper->move(moveValue);
        wasRunning = true; // check state in next loop (will issue status when movement done)
    }

    if (getAndConstrainValue<int32_t>("moveTo", command, moveValue, minimum, maximum)) {
        stepper->moveTo(moveValue);
        wasRunning = true; // check state in next loop (will issue status when movement done)
    }

    int32_t target = stepper->targetPosition();

    if ((target > maximum) or (target < minimum)) {
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
    if (getValue<bool>("binaryCtrl", command, binaryCtrl)) {
        if (binaryCtrl) {
            stepper->moveTo(maximum);
        } else {
            stepper->moveTo(minimum);
        }

        wasRunning = true; // check state in next loop (will issue status when movement done)
    }

    if (getValue<bool>("hold", command, holdOn)) {
        debug("hold %sactive", holdOn ? "" : "in");

        if (holdOn) {
            stepper->enableOutputs(); // ensure the motor coils are powered if hold gets called
        }

        wasRunning = true; // check state in next loop (will issue status)
    }

    if (!stepper->isRunning()) // check whether position is reached already
    {
        return;
    }

    // estimate travelTime in ms
    uint32_t distance = abs(stepper->targetPosition() - stepper->currentPosition());
    uint32_t travelTime = 0;
    if (distance > speed * speed / accel) {
        // distance after which the motor reaches max speed (including decelleration)
        // t = s/v_max + v_max/a
        travelTime = 1000 * distance / speed;
        travelTime += 1000 * speed / accel;
    } else {
        // time spend accellerating/decellerating
        // t = 2 * sqrt(s/a)
        travelTime = round(2000 * sqrt(distance / accel));
    }

    if (infoLED != "") { // instruct LED to show pattern
        XRTLdisposableCommand ledCommand(infoLED);

        String color;
        if (getValue("color", command, color)) {
            ledCommand.add("color", color);
        }

        ledCommand.add("hold", true);
        if (stepper->targetPosition() > stepper->currentPosition()) {
            ledCommand.add("cycle", (int)travelTime);
        } else {
            ledCommand.add("cycle", -(int)travelTime);
        }
        sendCommand(ledCommand);
    }

    stepper->enableOutputs();
    debug("moving from %d to %d", stepper->currentPosition(), stepper->targetPosition());
    sendStatus();
    notify(busy);
    return;
}