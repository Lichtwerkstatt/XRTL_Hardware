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

moduleType InputModule::getType() {
    return xrtl_input;
}

void InputModule::setup() {
    input = new XRTLinput;
    input->attach(pin);
    input->averageTime(averageTime);//in ms

    next = esp_timer_get_time(); // start streaming immediately
    nextCheck = esp_timer_get_time(); // check immediately
}

void InputModule::loop() {
    input->loop();
    
    int64_t now = esp_timer_get_time();
    double value = input->readMilliVolts();
    // apply conversion if defined
    for (int i = 0; i < conversionCount; i++) {
        conversion[i]->convert(value);
    }

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
    //debug("reporting voltage: %f mV", value);
    next = now + intervalMicroSeconds;

    DynamicJsonDocument doc(512);
    JsonArray event = doc.to<JsonArray>();

    event.add("data");
    JsonObject payload = event.createNestedObject();
    payload["componentId"] = getComponent(); // TODO: check function
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

void InputModule::saveSettings(JsonObject& settings){
    //JsonObject saving = settings.createNestedObject(id);

    settings["pin"] = pin;
    settings["averageTime"] = averageTime;

    settings["rangeChecking"] = rangeChecking;

    if (rangeChecking) {
        settings["loBound"] = loBound;
        settings["hiBound"] = hiBound;
        settings["deadMicroSeconds"] = deadMicroSeconds;
    }

    // conversion settings
    if (conversionCount == 0) return;
    JsonArray savingConversion = settings.createNestedArray("conversions");
    for (int i = 0; i < conversionCount; i++) {
        conversion[i]->saveSettings(savingConversion);
    }
}

void InputModule::loadSettings(JsonObject& settings) {
    //JsonObject loaded = settings[id];

    pin = loadValue<uint8_t>("pin", settings, 35);
    averageTime = loadValue<uint16_t>("averageTime", settings, 0);

    rangeChecking = loadValue<bool>("rangeChecking", settings, false);
    if (rangeChecking) {
        loBound = loadValue<double>("loBound", settings, 0);
        hiBound = loadValue<double>("hiBound", settings, 3300);
        deadMicroSeconds = loadValue<uint32_t>("deadMicroSeconds", settings, 0);
    }

    // load conversions
    JsonArray loadedConversion = settings["conversions"];
    if (!loadedConversion.isNull()) {
        for (JsonVariant var : loadedConversion) {
            JsonObject initializer = var.as<JsonObject>();
            conversion_t type = loadValue<conversion_t>("type", initializer, thermistor);
            addConversion(type);
            conversion[conversionCount - 1]->loadSettings(initializer, debugging);
            if (debugging) Serial.println("");
        }
    }

    if (!debugging) return;

    Serial.printf("controlId: %s\n", id.c_str());
    Serial.printf("pin: %d\n", pin);
    Serial.printf("averaging time: %d\n", averageTime);
    Serial.println("");

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

    id = serialInput("controlId: ");
    averageTime = serialInput("averaging time: ").toInt();

    if (strcmp(serialInput("change pin binding (y/n): ").c_str(), "y") == 0) {
        pin = serialInput("pin: ").toInt();
    }

    if (strcmp(serialInput("change conversion (y/n): ").c_str(), "y") != 0) return;
    for (int i = 0; i < conversionCount; i++) {
        delete conversion[i];
        conversion[i] = NULL;
    }
    conversionCount = 0;

    while (strcmp(serialInput("add conversion (y/n): ").c_str(), "y") == 0) {
        Serial.println("");
        Serial.println(centerString("conversions available",39,' ').c_str());
        for (int i = 0; i < 5; i++) {
            Serial.printf("%d: %s\n", i, conversionName[i]);
        }
        Serial.println("");

        conversion_t type = (conversion_t) serialInput("conversion: ").toInt();
        Serial.printf("trying to add conversion: %s\n", conversionName[type]);
        addConversion(type);
        Serial.printf("conversionCount: %d\n", conversionCount);
        conversion[conversionCount - 1]->setViaSerial();
    }

    rangeChecking = (strcmp(serialInput("check range (y/n): ").c_str(), "y") == 0);
    if (rangeChecking) {
        loBound = serialInput("low bound: ").toDouble();
        hiBound = serialInput("high bound: ").toDouble();
        deadMicroSeconds = serialInput("dead time: ").toInt() * 1000; // milli seconds are sufficient
    }
}

void InputModule::startStreaming() {
    if (isStreaming) {
        debug("stream already active");
        return;
    }
    next = esp_timer_get_time(); // immediately deliver first value
    isStreaming = true;
    sendStatus();
    debug("starting to stream value");
}

void InputModule::stopStreaming() {
    if (!isStreaming) {
        debug("stream already inactive");
        return;
    }
    isStreaming = false;
    sendStatus();
    debug("stopped streaming values");
}

bool InputModule::handleCommand(String& command) {
    return false;
}

bool InputModule::handleCommand(String& controlId, JsonObject& command) {
    if (!isModule(controlId)) return false;

    bool stream = false;
    if ( getValue<bool>("stream", command, stream) ) { 
        if (stream) {
            startStreaming();
        }
        else {
            stopStreaming();
        }
    }

    if (getValue<uint16_t>("averageTime", command, averageTime)) {
        input->averageTime(averageTime);
    }

    uint32_t interval;
    if ( getValue<uint32_t>("updateTime", command, interval) ) {
        intervalMicroSeconds = 1000 * interval;
    }

    if (!rangeChecking) return true;

    getValue<double>("upperBound", command, hiBound);
    getValue<double>("lowerBound", command, loBound);

    return true;
}

void InputModule::handleInternal(internalEvent event) {
    switch(event) {
        case socket_disconnected: {
            //stop streaming
            if (!isStreaming) return;
            isStreaming = false;
            debug("stream stopped due to disconnect event");
            return;
        }
        
        case input_trigger_high: {
            debug("high level trigger tripped");
            return;
        }
        case input_trigger_low: {
            debug("low level trigger tripped");
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

void InputModule::addConversion(conversion_t type) {
    if (conversionCount == 8) {
        Serial.println("WARNING: maximum number of conversions reached");
    }

    switch (type) {
        case thermistor: {          
            conversion[conversionCount++] = new Thermistor;
            return;
        }
        
        case resistance_voltage_divider: {
            conversion[conversionCount++] = new ResistanceDivider;
            return;
        }

        case map_value: {
            conversion[conversionCount++] = new MapValue;
            return;
        }
        
        case offset: {
            conversion[conversionCount++] = new Offset;
            return;
        }

        case multiplication: {
            conversion[conversionCount++] = new Multiplication;
            return;
        }
    }
}