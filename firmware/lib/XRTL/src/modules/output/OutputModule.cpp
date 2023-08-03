#include "OutputModule.h"

OutputModule::OutputModule(String moduleName) {
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");

    parameters.add(pin, "pin", "int");

    parameters.add(pwm, "pwm", "");
    parameters.addDependent(channel, "channel", "int", "pwm", true);
    parameters.addDependent(frequency, "frequency", "Hz", "pwm", true);

    parameters.add(infoLED, "infoLED", "String");
}

OutputModule::~OutputModule() {
    if (out != NULL) delete out;
}

moduleType OutputModule::getType() {
    return xrtl_output;
}

void OutputModule::pulse(uint16_t milliSeconds) {
    switchTime = esp_timer_get_time() + 1000 * milliSeconds;
    out->toggle(true);
}

void OutputModule::saveSettings(JsonObject &settings) {
    parameters.save(settings);
}

void OutputModule::loadSettings(JsonObject &settings) {
    parameters.load(settings);
    if (debugging && *debugging) parameters.print();
}

bool OutputModule::getStatus(JsonObject &status) {
    if (!out) {
        String errmsg = "[";
        errmsg += id;
        errmsg += "] output not initialized";
        sendError(hardware_failure, errmsg);
        return false;
    }

    status["busy"] = switchTime == 0 ? false : true;
    status["isOn"] = out->getState();
    if (!pwm) return true;
    
    status["pwm"] = out->read();
    return true;
}

void OutputModule::setViaSerial() {
    parameters.setViaSerial();
}

void OutputModule::setup() {
    if (!GPIO_IS_VALID_GPIO(pin)) debug("invalid pin");
    else if (!pwm) {
        out = new XRTLoutput(pwm);
        out->attach(pin);
    } else if (channel >= SOC_LEDC_CHANNEL_NUM) {
        debug("channel not available");
    } else {
        out = new XRTLoutput(pwm);
        if (!out->attach(pin, channel, frequency)) {
            debug("unable to set frequency");
            delete out;
            out = NULL;
        }
    }

    if (!out) {
        debug("output deactivated");
        return;
    }

    out->toggle(false);
}

void OutputModule::loop() {
    if (switchTime == 0 || !out) return;
    if (esp_timer_get_time() > switchTime) { // time is up -> switch off
        out->toggle(false);
        sendStatus();
        switchTime = 0; // return time trigger to off state
        notify(ready);
    }
}

void OutputModule::stop() {
    if (!out) return;
    out->toggle(false);
    debug("module stopped, power off");
}

void OutputModule::handleInternal(internalEvent eventId, String &sourceId) {
    switch (eventId) {
    case socket_disconnected: {
        if (!out || !out->getState()) return;
        out->toggle(false);
        debug("output powered down for safety reasons");
        return;
    }
    }
}

void OutputModule::handleCommand(String &controlId, JsonObject &command) {
    if (!isModule(controlId) && controlId != "*") return;

    bool getStatus = false;
    if (getValue<bool>("getStatus", command, getStatus) && getStatus) {
        sendStatus();
    }

    if (!out) return; // only available if initialized successfully

    if (pwm) // prevent pwm if a relay is used
    {
        uint8_t powerLvl;
        if (getValue<uint8_t>("pwm", command, powerLvl)) {
            if (infoLED != "") {
                XRTLdisposableCommand ledCommand(infoLED);

                String color;
                if (getValue("color", command, color)) {
                    ledCommand.add("color", color);
                }

                ledCommand.add("pulse", 2000);
                sendCommand(ledCommand);
            }

            out->write(powerLvl);
            sendStatus();
        }
    }

    bool targetState;
    if (getValue<bool>("switch", command, targetState)) {
        if (infoLED != "") {
            XRTLdisposableCommand ledCommand(infoLED);

            String color;
            if (getValue("color", command, color)) {
                ledCommand.add("color", color);
            }

            ledCommand.add("pulse", 2000);
            sendCommand(ledCommand);
        }

        out->toggle(targetState);
        sendStatus();
    }

    uint32_t toggleTime = 0;
    if (getValue<uint32_t>("pulse", command, toggleTime)) {
        if (infoLED != "") {
            XRTLdisposableCommand ledCommand(infoLED);

            String color;
            if (getValue("color", command, color)) {
                ledCommand.add("color", color);
            }

            ledCommand.add("cycle", (int)toggleTime);
            sendCommand(ledCommand);
        }

        pulse(toggleTime);
        sendStatus();
        notify(busy);
    }

    if (infoLED != "" && getValue("identify", command, toggleTime)) {
        XRTLdisposableCommand ledCommand(infoLED);

        String color;
        if (getValue("color", command, color)) {
            ledCommand.add("color", color);
        }

        debug("identifying via LED <%s>", infoLED);
        ledCommand.add("pulse", (int)toggleTime);
        sendCommand(ledCommand);
    }
}