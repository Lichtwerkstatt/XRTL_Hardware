#include "OutputModule.h"

void XRTLoutput::attach(uint8_t controlPin) {
    pin = controlPin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void XRTLoutput::attach(uint8_t controlPin, uint8_t pwmChannel, uint16_t pwmFrequency) {
    pin = controlPin;
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}

void XRTLoutput::toggle(bool targetState) {
    if (targetState == state) return;

    if (targetState) {
        digitalWrite(pin, HIGH);
        state = true;
    }
    else {
        digitalWrite(pin, LOW);
        state = false;
    }

}

void XRTLoutput::set(uint8_t powerLvl){
    
}

bool XRTLoutput::getState(){
    return state;
}

OutputModule::OutputModule(String moduleName, XRTL* source) {
    id = moduleName;
    xrtl = source;
}

void OutputModule::pulse(uint16_t milliSeconds) {
    switchTime = esp_timer_get_time() + 1000 * milliSeconds;
    out->toggle(true);
}

void OutputModule::saveSettings(DynamicJsonDocument& settings) {
    JsonObject saving = settings.createNestedObject(id);

    saving["controlId"] = control;
    saving["pin"] = pin;
}

void OutputModule::loadSettings(DynamicJsonDocument& settings) {
    JsonObject loaded = settings[id];

    control = loaded["controlId"].as<String>();
    pin = loaded["pin"].as<uint8_t>();

    if (strcmp(control.c_str(), "null") == 0){
        control = id;
    }
    if (pin == 0) {
        pin = 27;
    }

    if (!debugging) return;
    Serial.println("");
    Serial.println(centerString("",39,'-'));
    Serial.println(centerString(id,39,' '));
    Serial.println(centerString("",39,'-'));
    Serial.println("");

    Serial.printf("controlId: %s\n",control.c_str());
    Serial.printf("pin: %d\n",pin);
}

void OutputModule::getStatus(JsonObject& payload, JsonObject& status){
    if (out == NULL) return; // avoid errors: status might be called in setup before init

    status[control] = out->getState();
    return;
}

void OutputModule::setViaSerial() {
    Serial.println("");
    Serial.println(centerString("",39,'-'));
    Serial.println(centerString(id,39,' '));
    Serial.println(centerString("",39,'-'));
    Serial.println("");

    control = serialInput("controlId: ");

    if ( strcmp(serialInput("change pin binding (y/n): ").c_str(),"y") != 0) return;

    pin = serialInput("pin: ").toInt();
}

void OutputModule::setup() {
    out->attach(pin);
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
}

void OutputModule::handleInternal(internalEvent event){
    switch(event) {
        case socket_disconnected: {
            out->toggle(false);
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
    if (strcmp(control.c_str(), controlId.c_str() ) != 0) return false;

    auto valField = command["val"];

    if (valField.is<bool>()) {
        bool val = valField.as<bool>();

        out->toggle(val);
        sendStatus();
        return true;
    }
    else if (valField.is<int>()) {
        uint16_t val = valField.as<uint16_t>();
        pulse(val);
        sendStatus();
        return true;
    }
    else {
        String error = "[";
        error += id;
        error += "] command rejected: <val> is neither bool nor int";
        sendError(wrong_type, error);
        return true;
    }
    
    return false;
}

template<typename... Args>
void OutputModule::debug(Args... args) {
  if(!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};