#include "OutputModule.h"

OutputModule::OutputModule(String moduleName)
{
    id = moduleName;

    parameters.setKey(id);
    parameters.add(type, "type");
    parameters.add(pin, "pin", "int");
    parameters.add(pwm, "pwm", "");
    parameters.addDependent(channel, "channel", "int", "pwm", true);
    parameters.addDependent(frequency, "frequency", "Hz", "pwm", true);
    parameters.add(infoLED, "infoLED", "String");
}

OutputModule::~OutputModule()
{
    if (out != NULL)
        delete out;
}

moduleType OutputModule::getType()
{
    return xrtl_output;
}

void OutputModule::pulse(uint16_t milliSeconds)
{
    switchTime = esp_timer_get_time() + 1000 * milliSeconds;
    out->toggle(true);
}

void OutputModule::saveSettings(JsonObject &settings)
{
    /*settings["pin"] = pin;
    settings["pwm"] = pwm;
    settings["guardedModule"] = guardedModule;

    if (!pwm) return;
    settings["channel"] = channel;
    settings["frequency"] = frequency;*/
    parameters.save(settings);
}

void OutputModule::loadSettings(JsonObject &settings)
{
    /*pin = loadValue<uint8_t>("pin", settings, 27);
    pwm = settings["pwm"].as<bool>();
    pwm = loadValue<bool>("pwm", settings, false);
    guardedModule = loadValue<String>("guardedModule", settings, "none");

    if (pwm) {
        channel = loadValue<uint8_t>("channel", settings, 5);
        frequency = loadValue<uint16_t>("frequency", settings, 1000);

        if ( (frequency != 1000) and (frequency != 5000) and (frequency != 8000) and (frequency != 10000)) { // only 1, 5, 8 and 10 kHz allowed
            frequency = 1000;
        }
    }


    if (!debugging) return;

    Serial.printf("controlId: %s\n",id.c_str());
    Serial.printf("pin: %d\n",pin);
    Serial.printf("triggered by: %s\n", guardedModule);
    Serial.printf("%s output\n", pwm ? "PWM" : "relay");

    if (!pwm) return;

    Serial.printf("PWM channel: %d\n", channel);
    Serial.printf("PWM frequency: %d Hz\n", frequency);*/
    parameters.load(settings);
    if (debugging)
        parameters.print();
}

bool OutputModule::getStatus(JsonObject &status)
{
    if (out == NULL)
        return true; // avoid errors: status might be called in setup before init occured

    status["isOn"] = out->getState();
    if (!pwm)
        return true;
    status["pwm"] = out->read();
    return true;
}

void OutputModule::setViaSerial()
{
    /*Serial.println("");
    Serial.println(centerString("",39,'-'));
    Serial.println(centerString(id,39,' '));
    Serial.println(centerString("",39,'-'));
    Serial.println("");

    id = serialInput("controlId: ");

    guardedModule = serialInput("supervise module: ");

    pwm = ( serialInput("pwm (y/n): ") == "y" );
    if (pwm) {
        channel = serialInput("channel: ").toInt();
        frequency = serialInput("frequency: ").toInt();
    }

    if ( serialInput("change pin binding (y/n): ") != "y" ) return;

    pin = serialInput("pin: ").toInt();*/
    parameters.setViaSerial();
}

void OutputModule::setup()
{
    out = new XRTLoutput(pwm);

    if (!pwm)
    {
        out->attach(pin);
    }
    else
    {
        out->attach(pin, channel, frequency);
    }

    out->toggle(false);
}

void OutputModule::loop()
{
    if (switchTime == 0)
        return;
    if (esp_timer_get_time() > switchTime)
    { // time is up -> switch off
        out->toggle(false);
        sendStatus();
        switchTime = 0; // return time trigger to off state
    }
}

void OutputModule::stop()
{
    out->toggle(false);
    debug("module stopped, power off");
}

void OutputModule::handleInternal(internalEvent eventId, String &sourceId)
{
    if (out == NULL)
        return;
    switch (eventId)
    {
    case socket_disconnected:
    {
        if (!out->getState())
            return;
        out->toggle(false);
        debug("output powered down for safety reasons");
        return;
    }

    case debug_off:
    {
        debugging = false;
        return;
    }
    case debug_on:
    {
        debugging = true;
        return;
    }

    /* case input_trigger_high:
    {
        if (guardedModule == "")
            return;
        if (sourceId != guardedModule)
            return;

        out->toggle(false);

        String errormsg = "[";
        errormsg += id;
        errormsg += "] output turned off: value in <";
        errormsg += guardedModule;
        errormsg += "> above limit";
        sendError(out_of_bounds, errormsg);

        sendStatus();
        return;
    } */
    /* case input_trigger_low:
    {
        if (guardedModule == "")
            return;
        if (sourceId != guardedModule)
            return;

        out->toggle(false);

        String errormsg = "[";
        errormsg += id;
        errormsg += "] output turned off: value in <";
        errormsg += guardedModule;
        errormsg += "> below limit";
        sendError(out_of_bounds, errormsg);

        sendStatus();
        return;
    } */
    }
}

void OutputModule::handleCommand(String &controlId, JsonObject &command)
{
    if (!isModule(controlId))
        return;

    bool getStatus = false;
    if (getValue<bool>("getStatus", command, getStatus) && getStatus)
    {
        sendStatus();
    }

    if (pwm) // prevent pwm if a relay is used
    {
        uint8_t powerLvl;
        if (getValue<uint8_t>("pwm", command, powerLvl))
        {
            if (infoLED != "")
            {
                XRTLdisposableCommand ledCommand(infoLED);

                String color;
                if (getValue("color", command, color))
                {
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
    if (getValue<bool>("switch", command, targetState))
    {
        if (infoLED != "")
        {
            XRTLdisposableCommand ledCommand(infoLED);

            String color;
            if (getValue("color", command, color))
            {
                ledCommand.add("color", color);
            }

            ledCommand.add("pulse", 2000);
            sendCommand(ledCommand);
        }

        out->toggle(targetState);
        sendStatus();
    }

    uint32_t toggleTime = 0;
    if (getValue<uint32_t>("pulse", command, toggleTime))
    {
        if (infoLED != "")
        {
            XRTLdisposableCommand ledCommand(infoLED);

            String color;
            if (getValue("color", command, color))
            {
                ledCommand.add("color", color);
            }

            ledCommand.add("cycle", (int) toggleTime);
            sendCommand(ledCommand);
        }

        pulse(toggleTime);
        sendStatus();
    }
}