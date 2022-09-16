#include "InputModule.h"

void XRTLinput::attach(uint8_t inputPin) {
    pin = inputPin;
    pinMode(pin, INPUT);

    voltage = analogReadMilliVolts(pin); // make sure an appropriate value is initialized (avoiding trigger thresholds)

    next = esp_timer_get_time() + averageMicroSeconds;
}

void XRTLinput::averageTime(int64_t measurementTime) {
    averageMicroSeconds = 1000 * measurementTime;
}

void XRTLinput::loop() {
    now = esp_timer_get_time();
    if (now < next) {// average time not reached, keep going
        buffer += analogReadMilliVolts(pin);
        sampleCount++;
        return;
    }
    else {// average time over, store averaged value
        next = now + averageMicroSeconds;
        if (sampleCount == 0) {// no measurment taken, take single value
            buffer += analogReadMilliVolts(pin);
            sampleCount++;
        }
        voltage = ((double) buffer) / ((double)sampleCount);
        //Serial.printf("[input] new voltage aquired: %f mV (%d x oversampling)\n", voltage, sampleCount);
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

    next = esp_timer_get_time() + intervalMicroSeconds;
    nextCheck = esp_timer_get_time();
}

void InputModule::loop() {
    input->loop();
    
    int64_t now = esp_timer_get_time();
    double value = input->readMilliVolts();

    // check for violation of input bounds
    if (rangeChecking) {
        if (now > nextCheck) {

            if (value >= hiBound) {
                notify(input_trigger_high);
                nextCheck = now + deadTimeMicroSeconds;
            }

            if (value <= loBound) {
                notify(input_trigger_low);
                nextCheck = now + deadTimeMicroSeconds;
            }

        }
    }

    if (!isStreaming) return;

    if (now < next) return;
    debug("[%s] reporting voltage: %f mV\n", id.c_str(), value);
    // TODO: send voltage
}

void InputModule::stop() {
    stopStreaming();
}

void InputModule::saveSettings(DynamicJsonDocument& settings){
    JsonObject saving = settings.createNestedObject(id);
    
    saving["pin"] = pin;
    saving["time"] = time;

    saving["rangeChecking"] = rangeChecking;

    if (!rangeChecking) return;
    saving["loBound"] = loBound;
    saving["hiBound"] = hiBound;
    saving["deadTimeMicroSeconds"] = deadTimeMicroSeconds;
}

void InputModule::loadSettings(DynamicJsonDocument& settings) {
    JsonObject loaded = settings[id];

    pin = loaded["pin"].as<uint8_t>();
    time = loaded["time"].as<uint16_t>();

    rangeChecking = loaded["rangeChecking"].as<bool>();
    if (rangeChecking) {
        loBound = loaded["loBound"].as<double>();
        hiBound = loaded["hiBound"].as<double>();
        deadTimeMicroSeconds = loaded["deadTimeMicroSeconds"].as<uint32_t>();

        if (loBound < 0) {// 
            loBound = 0.0;
        }
        if (hiBound > 3300) {
            hiBound = 3300.0;
        }
    }

    if (!debugging) return;
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    Serial.printf("pin: %d\n", pin);
    Serial.printf("time: %d\n", time);

    Serial.printf(rangeChecking ? "triggers active\n" : "triggers inactive\n");
    Serial.printf("low bound: %f\n", loBound);
    Serial.printf("high bound: %f\n", hiBound);
    Serial.printf("dead time: %d\n", deadTimeMicroSeconds);
}

void InputModule::setViaSerial() {
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    time = serialInput("averaging time: ").toInt();

    rangeChecking = (strcmp(serialInput("check range (y/n): ").c_str(), "y") == 0);
    if (rangeChecking) {
        loBound = serialInput("low bound: ").toDouble();
        hiBound = serialInput("high bound: ").toDouble();
        deadTimeMicroSeconds = serialInput("dead time: ").toInt();
    }

    if (strcmp(serialInput("change pin binding (y/n): ").c_str(), "y") == 0) {
        pin = serialInput("pin: ").toInt();
    }
}

void InputModule::setStreamTimeCap(uint32_t milliSeconds) {
    intervalMicroSeconds = 1000 * milliSeconds;
}

void InputModule::startStreaming() {
    next = esp_timer_get_time() + intervalMicroSeconds;
    isStreaming = true;
    debug("[%s] starting to stream value", id.c_str());
    // TODO: setting up message for sending binary
}

void InputModule::stopStreaming() {
    isStreaming = false;
    debug("[%s] stopped streaming values", id.c_str());
}

bool InputModule::handleCommand(String& command) {
    return false;
}

bool InputModule::handleCommand(String& controlId, JsonObject& command) {
    if (strcmp(controlId.c_str(),id.c_str()) != 0) return false;

    if (isStreaming) stopStreaming();
    else startStreaming();

    return true;
}

void InputModule::handleInternal(internalEvent event) {
    switch(event) {
        case socket_disconnected: {
            //stop streaming
            if (!isStreaming) return;
            isStreaming = false;
            debug("[%s] stream stopped due to disconnect event",id.c_str());
            return;
        }
        
        case input_trigger_high: {
            debug("[%s] high level trigger tripped", id.c_str());
            return;
        }
        case input_trigger_low: {
            debug("[%s] low level trigger tripped", id.c_str());
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