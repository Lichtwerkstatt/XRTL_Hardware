#include "OutputModule.h"

XRTLoutput::XRTLoutput(bool isPWM){
    pwm = isPWM;
}

void XRTLoutput::attach(uint8_t controlPin) {
    pin = controlPin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void XRTLoutput::attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency) {
    pin = controlPin;
    channel = pwmChannel;
    frequency = pwmFrequency;
    ledcSetup(channel, frequency, 8); // 8 bit resolution -> steps in percentage: 1/255 = 0.39%
    pinMode(pin, OUTPUT);
    ledcAttachPin(pin, channel);
}

void XRTLoutput::toggle(bool targetState) {
    if (targetState == state) return;

    if (targetState) {
        if (pwm) {
            ledcWrite(channel, power);
        }
        else {
            digitalWrite(pin, HIGH);
        }
        state = true;
    }
    else {
        if (pwm) {
            ledcWrite(channel, 0);
        }
        else {
            digitalWrite(pin, LOW);
        }
        state = false;
    }

}

void XRTLoutput::write(uint8_t powerLvl){
    if (!pwm) return;
    power = powerLvl;
}

uint8_t XRTLoutput::read() {
    if (!pwm) {
        if (state) return 255;
        else return 0;
    }
    return power;
}

bool XRTLoutput::getState(){
    return state;
}

OutputModule::OutputModule(String moduleName, XRTL* source) {
    id = moduleName;
    xrtl = source;
}

moduleType OutputModule::getType() {
    return xrtl_output;
}

void OutputModule::pulse(uint16_t milliSeconds) {
    switchTime = esp_timer_get_time() + 1000 * milliSeconds;
    out->toggle(true);
}

void OutputModule::saveSettings(JsonObject& settings) {
    //JsonObject saving = settings.createNestedObject(id);

    settings["pin"] = pin;
    settings["pwm"] = pwm;

    if (!pwm) return;
    settings["channel"] = channel;
    settings["frequency"] = frequency;
}

void OutputModule::loadSettings(JsonObject& settings) {
    //JsonObject loaded = settings[id];

    pin = loadValue<uint8_t>("pin", settings, 27);
    pwm = settings["pwm"].as<bool>();
    pwm = loadValue<bool>("pwm", settings, false);;

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
    Serial.printf(pwm ? "PWM output\n" : "relay output\n");

    if (!pwm) return;

    Serial.printf("PWM channel: %d\n", channel);
    Serial.printf("PWM frequency: %d Hz\n", frequency);
}

void OutputModule::getStatus(JsonObject& payload, JsonObject& status){
    if (out == NULL) return; // avoid errors: status might be called in setup before init occured
    JsonObject outputState = status.createNestedObject(id); 

    outputState["isOn"] = out->getState();
    if (!pwm) return;
    outputState["pwm"] = out->read();
}

void OutputModule::setViaSerial() {
    Serial.println("");
    Serial.println(centerString("",39,'-'));
    Serial.println(centerString(id,39,' '));
    Serial.println(centerString("",39,'-'));
    Serial.println("");

    id = serialInput("controlId: ");
    pwm = (strcmp(serialInput("pwm (y/n): ").c_str(),"y") == 0);
    if (pwm) {
        channel = serialInput("channel: ").toInt();
        frequency = serialInput("frequency: ").toInt();
    }

    if ( strcmp(serialInput("change pin binding (y/n): ").c_str(),"y") != 0) return;

    pin = serialInput("pin: ").toInt();
}

void OutputModule::setup() {
    out = new XRTLoutput(pwm);

    if (!pwm) {
        out->attach(pin);
    }
    else {
        out->attach(pin, channel, frequency);
    }

    out->toggle(false);
}

void OutputModule::loop() {
    if (switchTime == 0) return;
    if (esp_timer_get_time() > switchTime) {
        out->toggle(false);
        sendStatus();
        switchTime = 0;
    }
}

void OutputModule::stop() {
    out->toggle(false);
    debug("module stopped, power off");
}

void OutputModule::handleInternal(internalEvent event){
    if ( out == NULL) return;
    switch(event) {
        case socket_disconnected: {
            if (!out->getState()) return;
            out->toggle(false);
            debug("output powered down for safety reasons");
            return;
        }

        case debug_off: {
            debugging = false;
            return;
        }
        case debug_on: {
            debugging = true;
            return;
        }
    }
}

bool OutputModule::handleCommand(String& controlId, JsonObject& command) {
    if (!isModule(controlId)) return false;

    uint8_t powerLvl;
    if (pwm) {
        if (getValue<uint8_t>("pwm", command, powerLvl)) out->write(powerLvl);
    }

    auto valField = command["val"];

    if (valField.is<bool>()) {
        bool val = valField.as<bool>();

        out->toggle(val);
    }
    else if (valField.is<int>()) {
        uint16_t val = valField.as<uint16_t>();
        pulse(val);
    }
    else {
        String error = "[";
        error += id;
        error += "] command rejected: <val> is neither bool nor int";
        sendError(wrong_type, error);
        return true;
    }
    
    sendStatus();
    return true;
}