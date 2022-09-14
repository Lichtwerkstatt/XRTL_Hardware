#include "InputModule.h"

void XRTLinput::attach(uint8_t inputPin) {
    pin = inputPin;
    pinMode(pin, INPUT);
    next = esp_timer_get_time() + averageMicroSeconds;
}

void XRTLinput::averageTime(int64_t measurementTime) {
    averageMicroSeconds = 1000 * measurementTime;
}

void XRTLinput::loop() {
    /*if (esp_timer_get_time() > sampleTime) {
        if (sampleCount == maxSamples) {

        }
    }*/
    if (next == 0) {
        voltage = analogReadMilliVolts(pin);
        return;
    }

    now = esp_timer_get_time();
    if (now < next) {// average time not reached, keep going
        buffer += analogReadMilliVolts(pin);
        sampleCount++;
        return;
    }
    else {// average time over, deliver value
        next = now + averageMicroSeconds;
        if (sampleCount == 0) {// no measurment taken
            buffer += analogReadMilliVolts(pin);
            sampleCount++;
        }
        voltage = ((double) buffer) / ((double)sampleCount);
        //Serial.printf("[input] %dx oversampling, voltage: %f\n", sampleCount, voltage);
        buffer = 0;
        sampleCount = 0;
    }
}

double XRTLinput::readMilliVolts() {
    return voltage;
}

InputModule::InputModule(String moduleName, XRTL* source) {
    id = moduleName;
    xrtl = source;
}

void InputModule::setup() {
    input = new XRTLinput;
    input->attach(pin);
    input->averageTime(time);//in ms
}

void InputModule::loop() {
    input->loop();

    // check for violation of input bounds
    if (rangeChecking) {
        double value = input->readMilliVolts();
        if (value > hiBound) notify(input_trigger_high);
        if (value < loBound) notify(input_trigger_low);
    }

    if (!isStreaming) return;
    //send voltage
}

void InputModule::saveSettings(DynamicJsonDocument& settings){
    JsonObject saving = settings.createNestedObject(id);
    
    saving["pin"] = pin;
    saving["time"] = time;
}

void InputModule::loadSettings(DynamicJsonDocument& settings) {
    JsonObject loaded = settings[id];

    pin = loaded["pin"].as<uint8_t>();
    time = loaded["time"].as<uint16_t>();

    if (!debugging) return;
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    Serial.printf("pin: %d\n", pin);
    Serial.printf("time: %d\n", time);
}

void InputModule::setViaSerial() {
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    pin = serialInput("pin: ").toInt();
    time = serialInput("time: ").toInt();
}

void InputModule::setStreamTimeCap(uint32_t milliSeconds) {
    intervalMicroSeconds = 1000 * milliSeconds;
}

void InputModule::startStreaming() {
    next = esp_timer_get_time() + intervalMicroSeconds;
}

bool InputModule::handleCommand(String& command) {
    return false;
}

bool InputModule::handleCommand(String& controlId, JsonObject& command) {
    return false;
}

void InputModule::handleInternal(internalEvent event) {
    switch(event) {
        case socket_disconnected: {
            //stop streaming
            debug("[%s] stream stopped due to failed connection",id.c_str());
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

template<typename... Args>
void InputModule::debug(Args... args) {
  if(!debugging) return;
  Serial.printf(args...);
  Serial.print('\n');
};