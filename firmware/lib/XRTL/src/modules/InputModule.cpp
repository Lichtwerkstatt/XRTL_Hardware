#include "InputModule.h"

void XRTLinput::attach(uint8_t inputPin) {
    pin = inputPin;
    pinMode(pin, INPUT);

    voltage = analogReadMilliVolts(pin); // make sure voltage is initialized with real value (avoiding trigger thresholds)

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
    input->averageTime(averageTime);//in ms

    next = esp_timer_get_time(); // start streaming immediately
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
                nextCheck = now + deadMicroSeconds;
            }

            if (value <= loBound) {
                notify(input_trigger_low);
                nextCheck = now + deadMicroSeconds;
            }

        }
    }

    if (!isStreaming) return;

    if (now < next) return;
    //debug("[%s] reporting voltage: %f mV", id.c_str(), value);
    next = now + intervalMicroSeconds;

    DynamicJsonDocument doc(512);
    JsonArray event = doc.to<JsonArray>();

    event.add("data");
    JsonObject payload = event.createNestedObject();
    payload["componentId"] = "test"; // TODO: get componentId from socket module
    payload["type"] = "double";
    payload["dataId"] = id;

    JsonObject data = payload.createNestedObject("data");
    data["type"] = "Buffer";
    data["data"] = value;

    sendEvent(event); 
}

void InputModule::stop() {
    stopStreaming();
}

void InputModule::saveSettings(DynamicJsonDocument& settings){
    JsonObject saving = settings.createNestedObject(id);

    saving["control"] = control;
    saving["pin"] = pin;
    saving["averageTime"] = averageTime;

    saving["rangeChecking"] = rangeChecking;

    if (!rangeChecking) return;
    saving["loBound"] = loBound;
    saving["hiBound"] = hiBound;
    saving["deadMicroSeconds"] = deadMicroSeconds;
}

void InputModule::loadSettings(DynamicJsonDocument& settings) {
    JsonObject loaded = settings[id];

    control = loadValue<String>("control", loaded, id);
    pin = loadValue<uint8_t>("pin", loaded, 35);
    averageTime = loadValue<uint16_t>("averageTime", loaded, 0);

    rangeChecking = loadValue<bool>("rangeChecking", loaded, false);
    if (rangeChecking) {
        loBound = loadValue<double>("loBound", loaded, 0);
        hiBound = loadValue<double>("hiBound", loaded, 3300);
        deadMicroSeconds = loadValue<uint32_t>("deadMicroSeconds", loaded, 0);
    }

    if (!debugging) return;
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    Serial.printf("controlId: %s\n", control.c_str());
    Serial.printf("pin: %d\n", pin);
    Serial.printf("averaging time: %d\n", averageTime);

    Serial.printf(rangeChecking ? "triggers active\n" : "triggers inactive\n");
    Serial.printf("low bound: %f\n", loBound);
    Serial.printf("high bound: %f\n", hiBound);
    Serial.printf("dead time: %d\n", deadMicroSeconds);
}

void InputModule::setViaSerial() {
    Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    control = serialInput("controlId: ");
    averageTime = serialInput("averaging time: ").toInt();

    rangeChecking = (strcmp(serialInput("check range (y/n): ").c_str(), "y") == 0);
    if (rangeChecking) {
        loBound = serialInput("low bound: ").toDouble();
        hiBound = serialInput("high bound: ").toDouble();
        deadMicroSeconds = serialInput("dead time: ").toInt() * 1000; // milli seconds are sufficient
    }

    if (strcmp(serialInput("change pin binding (y/n): ").c_str(), "y") == 0) {
        pin = serialInput("pin: ").toInt();
    }
}

void InputModule::startStreaming() {
    next = esp_timer_get_time(); // immediately deliver first value
    isStreaming = true;
    debug("[%s] starting to stream value", id.c_str());
}

void InputModule::stopStreaming() {
    isStreaming = false;
    debug("[%s] stopped streaming values", id.c_str());
}

bool InputModule::handleCommand(String& command) {
    return false;
}

bool InputModule::handleCommand(String& controlId, JsonObject& command) {
    if (strcmp(controlId.c_str(),control.c_str()) != 0) return false;

    auto streamField = command["stream"];
    if ( streamField.isNull() ) {
        // key unused
    }
    else if ( !streamField.is<bool>() ) {
        String errormsg = "[";
        errormsg += control;
        errormsg += "] command rejected: <stream> is no boolean";
        sendError(wrong_type, errormsg);
    }
    else {
        if ( streamField.as<bool>() ) {
            startStreaming();
        }
        else {
            stopStreaming();
        }
    }

    auto intervalField = command["interval"];
    if ( intervalField.isNull() ) {
        // key unused
    }
    else if ( !intervalField.is<int>() ) {
        String errormsg = "[";
        errormsg += control;
        errormsg += "] command rejected: <interval> is no integer";
        sendError(wrong_type, errormsg);
    }
    else {
        intervalMicroSeconds = 1000 * intervalField.as<uint32_t>();// milli second resolution is sufficient, micro seconds only needed for esp_timer
    }

    auto lowerBoundField = command["lowerBound"];
    if ( lowerBoundField.isNull() ) {
        // key unused
    }
    else if ( !( lowerBoundField.is<float>() or lowerBoundField.is<int>() ) ) {
        String errormsg = "[";
        errormsg += control;
        errormsg += "] command rejected: <lowerBound> is no float or int";
        sendError(wrong_type, errormsg);
    }
    else {
        loBound = lowerBoundField.as<double>();
    }

    auto upperBoundField = command["upperBound"];
    if ( upperBoundField.isNull() ) {
        // key unused
    }
    else if ( !(upperBoundField.is<float>() or upperBoundField.is<int>() ) ) {
        String errormsg = "[";
        errormsg += control;
        errormsg += "] command rejected: <lowerBound> is no float or int";
        sendError(wrong_type, errormsg);
    }
    else {
        hiBound = upperBoundField.as<double>();
    }

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