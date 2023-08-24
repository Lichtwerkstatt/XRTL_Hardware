#include "ServoModule.h"

ServoModule::ServoModule(String moduleName) {
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");

    parameters.add(frequency, "frequency", "Hz");
    parameters.add(minDuty, "minDuty", "µs");
    parameters.add(maxDuty, "maxDuty", "µs");

    parameters.add(minAngle, "minAngle", "int");
    parameters.add(maxAngle, "maxAngle", "int");
    parameters.add(maxSpeed, "maxSpeed", "float");

    parameters.add(initial, "initial", "int");

    parameters.add(pin, "pin", "int");

    parameters.add(infoLED, "infoLED", "String");
}

moduleType ServoModule::getType() {
    return xrtl_servo;
}

void ServoModule::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void ServoModule::loadSettings(JsonObject &settings) {
    parameters.load(settings);

    if (debugging && *debugging) parameters.print();
}

void ServoModule::setViaSerial() {
    parameters.setViaSerial();
}

bool ServoModule::getStatus(JsonObject &status) {
    status["busy"] = wasRunning;
    status["absolute"] = read();
    status["relative"] = mapFloat(currentDuty, minDuty, maxDuty, 0, 100);

    return true;
}

void ServoModule::setup() {
    servo = new Servo;
    servo->setPeriodHertz(frequency);
    servo->attach(pin, minDuty, maxDuty);

    if (maxSpeed <= 0) {
        timeStep = 0;
    } else {
        timeStep = round(float(maxAngle - minAngle) / float(maxDuty - minDuty) * 1000000 / maxSpeed);
    }

    write(initial);
    targetDuty = currentDuty;
    wasRunning = true;
    nextStep = esp_timer_get_time() + 750000;
    // servo->detach();

    // debug("stepping: %d µs", timeStep);
}

void ServoModule::loop() {
    if (!wasRunning) return;

    int64_t now = esp_timer_get_time();
    if (now < nextStep) return;

    if (currentDuty != targetDuty) {
        if (positiveDirection) {
            servo->writeMicroseconds(currentDuty++);
        } else {
            servo->writeMicroseconds(currentDuty--);
        }
    }

    if (currentDuty == targetDuty) {
        if (!holdOn) // note: when detaching pin do so as close as possible to last write command to avoid unwanted movements
        {
            servo->detach(); // if hold is activated: keep motor powered
        }

        if (infoLED != "") {
            XRTLdisposableCommand ledCommand(infoLED);
            ledCommand.add("hold", false); // TODO: prevent this from canceling patterns during connection phase
            sendCommand(ledCommand);
        }

        wasRunning = false;
        sendStatus();
        debug("done moving");
        notify(ready);
        return;
    }

    nextStep = now + timeStep;
}

void ServoModule::stop() {
    if (!wasRunning) return;
    wasRunning = false;
    targetDuty = currentDuty;
    servo->detach();
    notify(ready);
}

void ServoModule::handleCommand(String &controlId, JsonObject &command) {
    if (!isModule(controlId) && controlId != "*")
        return;

    bool temp = false;
    if (getValue<bool>("getStatus", command, temp) && temp) {
        sendStatus();
    }

    if (getValue<bool>("stop", command, temp) && temp) {
        stop();
        sendStatus();
    }

    if (getValue<bool>("reset", command, temp) && temp) {
        DynamicJsonDocument doc(128);
        JsonObject driveCommand = doc.to<JsonObject>();
        driveCommand["controlId"] = id;
        driveCommand["moveTo"] = initial;
        driveServo(driveCommand);
        while (wasRunning)
            loop();
    }

    if (getValue<bool>("hold", command, holdOn)) {
        debug("hold %sactive", holdOn ? "" : "in");
    }

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

    driveServo(command); // only reached if not busy
}

int16_t ServoModule::read() {
    if (servo == NULL)
        return 0;
    return round(mapFloat(currentDuty, minDuty, maxDuty, minAngle, maxAngle));
}

void ServoModule::write(int16_t target) {
    if (servo == NULL)
        return;
    currentDuty = round(mapFloat(target, minAngle, maxAngle, minDuty, maxDuty));
    servo->writeMicroseconds(currentDuty);
}

void ServoModule::driveServo(JsonObject &command) {

    int16_t target;
    bool binaryCtrl;

    if (getAndConstrainValue<int16_t>("move", command, target, minAngle - maxAngle, maxAngle - minAngle)) { // complete range: maxAngle - minAngle; negative range: minAngle - maxAngle
        target += read();
    } else if (getAndConstrainValue<int16_t>("moveTo", command, target, minAngle, maxAngle)) {
        // nothing to do here
    } else if (getValue<bool>("binaryCtrl", command, binaryCtrl)) {
        if (binaryCtrl) {
            target = maxAngle;
            positiveDirection = true;
        } else {
            target = minAngle;
            positiveDirection = false;
        }
    } else
        return; // command is accepted after this point; check bounds and drive servo

    if (wasRunning) { // ignore move commands if already moving
        String error = "[";
        error += id;
        error += "] already moving";
        sendError(is_busy, error);
        return;
    }

    if ((target < minAngle) or (target > maxAngle)) { // check if bounds are violated after relative movement
        target = constrain(target, minAngle, maxAngle);

        String error = "[";
        error += id;
        error += "] target position was constrained to (";
        error += minAngle;
        error += ",";
        error += maxAngle;
        error += ")";

        sendError(out_of_bounds, error);
    }

    uint32_t travelTime;
    if (timeStep > 0) // speed has been limited
    {
        wasRunning = true;
        servo->attach(pin, minDuty, maxDuty);
        servo->writeMicroseconds(currentDuty);
        targetDuty = round(mapFloat(target, minAngle, maxAngle, minDuty, maxDuty));
        travelTime = abs(targetDuty - currentDuty) * timeStep / 1000; // travelTime in ms

        if (targetDuty > currentDuty) {
            positiveDirection = true;
            nextStep = esp_timer_get_time() + timeStep;
        } else if (targetDuty < currentDuty) {
            positiveDirection = false;
            nextStep = esp_timer_get_time() + timeStep;
        } else if (targetDuty == currentDuty) {
            nextStep = esp_timer_get_time() + 750000; // holding for at least 750 ms
        }
    } else {
        servo->attach(pin, minDuty, maxDuty);
        write(target);
        targetDuty = currentDuty;
        wasRunning = true;
        travelTime = 750;
        nextStep = esp_timer_get_time() + 750000; // holding for at least 750 ms
    }

    if (infoLED != "") {
        XRTLdisposableCommand ledCommand(infoLED);
        String color;
        if (getValue("color", command, color)) {
            ledCommand.add("color", color);
        }

        ledCommand.add("hold", true);
        if (positiveDirection) {
            ledCommand.add("cycle", (int)travelTime);
        } else {
            ledCommand.add("cycle", -(int)travelTime);
        }
        sendCommand(ledCommand);
    }

    debug("moving from %d to %d", currentDuty, targetDuty);
    sendStatus();
    notify(busy);
}