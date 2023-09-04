#include "ServoModule.h"

// TODO: investigate fade option of hardware PWM -- drawback: can't stop fade once activated
ServoModule::ServoModule(String moduleName) {
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");

    parameters.add(channel, "channel", "int");
    parameters.add(frequency, "frequency", "Hz");
    parameters.add(minDuty, "minDuty", "µs");
    parameters.add(maxDuty, "maxDuty", "µs");

    parameters.add(minAngle, "minAngle", "float");
    parameters.add(maxAngle, "maxAngle", "float");
    parameters.add(maxSpeed, "maxSpeed", "float");

    parameters.add(initial, "initial", "float");

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
    status["relative"] = mapFloat(currentTicks, minTicks, maxTicks, 0, 100);

    return true;
}

void ServoModule::setup() {
    timeStep = round((double) 1000000.0 / (double) frequency); // duration of one full PWM cycle in µs, steps cannot be shorter than this
    minTicks = round((double) minDuty * (double) frequency * (double) 0.065535);
    maxTicks = round((double) maxDuty * (double) frequency * (double) 0.065535);
    if (maxSpeed > 0) {
        stepSize = round((double) maxSpeed * ((double) maxDuty - (double) minDuty) / ((double) maxAngle - (double) minAngle) * (double) 0.065535);
        debug("stepsize: %d", stepSize);
    }

    ledcSetup(channel, frequency, 16);
    ledcAttachPin(pin, channel);

    if (maxSpeed <= 0) {
        timeStep = 0; // re-use as switch for direct setting of servo
    }

    write(initial);
    targetTicks = currentTicks;
    wasRunning = true;
    nextStep = esp_timer_get_time() + 750000;

    // debug("stepping: %d µs", timeStep);
}

void ServoModule::loop() {
    if (!wasRunning) return;

    if (esp_timer_get_time() < nextStep) return;

    if (currentTicks == targetTicks || (positiveDirection && currentTicks > targetTicks) || (!positiveDirection && currentTicks < targetTicks)) {
        currentTicks = targetTicks;         // rewrite currentTicks to target if it has been exceeded
        if (!holdOn) ledcWrite(channel, 0); // if hold is deactivated: power down motor

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
    else {
        if (positiveDirection) {
            currentTicks += stepSize;
        }
        else {
            currentTicks -= stepSize;
        }

        ledcWrite(channel, currentTicks);
    }

    nextStep = esp_timer_get_time() + timeStep; // make sure one complete PWM cycle passed before the next step (controller only updates after cycle)
}

void ServoModule::stop() {
    if (!wasRunning) return;
    wasRunning = false;
    targetTicks = currentTicks;
    ledcWrite(channel, 0);
    notify(ready);
}

void ServoModule::handleCommand(String &controlId, JsonObject &command) {
    if (!isModule(controlId) && controlId != "*")
        return;

    bool tempBool = false;
    if (getValue<bool>("getStatus", command, tempBool) && tempBool) {
        sendStatus();
    }

    if (getValue<bool>("stop", command, tempBool) && tempBool) {
        stop();
        sendStatus();
    }

    if (getValue<bool>("reset", command, tempBool) && tempBool) {
        DynamicJsonDocument doc(128);
        JsonObject driveCommand = doc.to<JsonObject>();
        driveCommand["controlId"] = id;
        driveCommand["moveTo"] = initial;
        driveServo(driveCommand);
        while (wasRunning) loop();
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

float ServoModule::read() {
    return mapFloat(currentTicks, minTicks, maxTicks, minAngle, maxAngle);
}

/**
 * @brief immediately sets the servo to the new target
 * @param target value that the servo motor should move to (value range)
*/
void ServoModule::write(float target) {
    currentTicks = round(mapFloat(target, minAngle, maxAngle, minTicks, maxTicks));
    ledcWrite(channel, currentTicks);
}

void ServoModule::driveServo(JsonObject &command) {
    float target;
    bool binaryCtrl;
    if (getAndConstrainValue<float>("move", command, target, minAngle - maxAngle, maxAngle - minAngle)) { // complete range: maxAngle - minAngle; negative range: minAngle - maxAngle
        target += read();
    }
    else if (getAndConstrainValue<float>("moveTo", command, target, minAngle, maxAngle)) {
        // nothing to do here
    }
    else if (getValue<bool>("binaryCtrl", command, binaryCtrl)) {
        if (binaryCtrl) {
            target = maxAngle;
        } else {
            target = minAngle;
        }
    }
    else return; // command is accepted after this point; check bounds and drive servo

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

    if (target >= read()) {
        positiveDirection = true;
    }
    else {
        positiveDirection = false;
    }

    uint32_t travelTime;
    targetTicks = round(mapFloat(target, minAngle, maxAngle, minTicks, maxTicks));
    if (timeStep > 0) // speed has been limited
    {
        wasRunning = true;
        ledcWrite(channel, currentTicks);

        if (targetTicks == currentTicks) {
            nextStep = esp_timer_get_time() + 750000; // holding for at least 750 ms
        }
        else {
            travelTime = timeStep * stepSize;
            nextStep = esp_timer_get_time() + timeStep;
        }
    } else {
        ledcWrite(channel, targetTicks);
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

    debug("moving from %d to %d", currentTicks, targetTicks);
    sendStatus();
    notify(busy);
}