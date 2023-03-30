#include "InputModule.h"

InputModule::InputModule(String moduleName) {
    id = moduleName;

    parameters.setKey(id);
    convParameters.setKey("conversions");

    parameters.add(pin, "pin", "int");
    parameters.add(type, "type");
    parameters.add(averageTime, "averageTime", "ms");
    parameters.add(rangeChecking, "rangeChecking", "");
    parameters.addDependent(relayViolations, "relayViolations", "", "rangeChecking", true);
    parameters.addDependent(loBound, "loBound", "float", "rangeChecking", true);
    parameters.addDependent(hiBound, "hiBound", "float",  "rangeChecking", true);
    parameters.add(deadMicroSeconds, "deadMicroSeconds", "µs");
}

moduleType InputModule::getType() {
    return xrtl_input;
}

void InputModule::setup() {
    input = new XRTLinput;
    input->attach(pin);
    input->averageTime(averageTime);// in ms
    if ( input->readMilliVolts() >= hiBound ) { // initialize lastState
        lastState = true;
    }
    else {
        lastState = false;
    }

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
    // TODO: report to server?
    if (rangeChecking && now > nextCheck) {
        if (value >= hiBound) {
            if (!lastState) {
                notify(input_trigger_high);
                debug("high level guard triggered");
                lastState = true;
                if (relayViolations) sendStatus();
            }

            nextCheck = now + deadMicroSeconds;
        }

        if (value <= loBound) {
            if (lastState){
                notify(input_trigger_low);
                debug("low level guard triggered");
                lastState = false;
                if (relayViolations) sendStatus();
            }

            nextCheck = now + deadMicroSeconds;
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
    //payload["componentId"] = getComponent(); // TODO: check function
    payload["controlId"] = id;
    payload["type"] = "float";
    //payload["dataId"] = id;

    JsonObject data = payload.createNestedObject("data");
    data["type"] = "Buffer";// for consistency with other data transmissions, should look identical on the server side after parsing
    data["data"] = value;

    sendEvent(event); 
}

void InputModule::stop() {
    stopStreaming();
}

void InputModule::saveSettings(JsonObject& settings){
    /*settings["pin"] = pin;
    settings["averageTime"] = averageTime;
    settings["rangeChecking"] = rangeChecking;
    settings["relayViolations"] = relayViolations;

    settings["loBound"] = loBound;
    settings["hiBound"] = hiBound;
    settings["deadMicroSeconds"] = deadMicroSeconds;*/
    JsonObject subSettings;
    parameters.save(settings, subSettings);

    // conversion settings
    if (conversionCount == 0) return;
    JsonArray conversionSettings = subSettings.createNestedArray("conversions");
    for (int i = 0; i < conversionCount; i++) {
        JsonObject saveConversionConfig = conversionSettings.createNestedObject();
        conversion[i]->saveSettings(saveConversionConfig);
    }
}

void InputModule::loadSettings(JsonObject& settings) {
    //JsonObject loaded = settings[id];
    //parameters.load(settings);

    /*pin = loadValue<uint8_t>("pin", settings, 35);
    averageTime = loadValue<uint16_t>("averageTime", settings, 0);
    rangeChecking = loadValue<bool>("rangeChecking", settings, false);
    relayViolations = loadValue<bool>("relayViolations", settings, false);
    
    loBound = loadValue<double>("loBound", settings, 0);// default is minimum ADC voltage in mV -- no conversion
    hiBound = loadValue<double>("hiBound", settings, 3300);// default is maximum ADC voltage in mV -- no conversion
    deadMicroSeconds = loadValue<uint32_t>("deadMicroSeconds", settings, 0);*/
    JsonObject subSettings;
    parameters.load(settings, subSettings);

    // load conversions
    JsonArray loadedConversion = subSettings["conversions"];
    if (!loadedConversion.isNull()) {
        for (JsonVariant value : loadedConversion) { // iterate over all objects within loadedConversion
            JsonObject conversionSettings = value.as<JsonObject>();
            auto typeField = conversionSettings["type"];
            conversion_t convType = loadValue<conversion_t>("type", conversionSettings, offset);
            addConversion(convType);
            conversion[conversionCount - 1]->loadSettings(conversionSettings, debugging);
            //if (debugging) Serial.println("");
        }
    }

    if (debugging) parameters.print();

    /*if (!debugging) return;

    Serial.printf("controlId: %s\n", id.c_str());
    Serial.printf("pin: %d\n", pin);
    Serial.printf("averaging time: %d\n", averageTime);
    Serial.println("");

    Serial.printf("triggers %sactive\n", rangeChecking ? "" : "in");
    Serial.printf("low bound: %f\n", loBound);
    Serial.printf("high bound: %f\n", hiBound);
    Serial.printf("dead time: %d\n", deadMicroSeconds);*/
}

void InputModule::setViaSerial() {
    /*Serial.println("");
    Serial.println(centerString("",39,'-').c_str());
    Serial.println(centerString(id,39,' ').c_str());
    Serial.println(centerString("",39,'-').c_str());
    Serial.println("");

    id = serialInput("controlId: ");
    averageTime = serialInput("averaging time: ").toInt();

    if ( serialInput("change pin binding (y/n): ") == "y" ) {
        pin = serialInput("pin: ").toInt();
    }
 
    if ( serialInput("check range (y/n): ") == "y" ) {
        rangeChecking = true;
        relayViolations = (serialInput("relay violations to server (y/n): ") == "y");
        loBound = serialInput("low bound: ").toDouble();
        hiBound = serialInput("high bound: ").toDouble();
        deadMicroSeconds = serialInput("dead time: ").toInt() * 1000; // milli seconds are sufficient
    }
    else {
        rangeChecking = false;
    }*/
    parameters.setViaSerial();

    if ( serialInput("change conversion (y/n): ") != "y" ) return;
    for (int i = 0; i < conversionCount; i++) {
        delete conversion[i];
        conversion[i] = NULL;
    }
    conversionCount = 0;

    while ( serialInput("add conversion (y/n): ") == "y" ) {
        Serial.println("");
        Serial.println(centerString("conversions available",39,' ').c_str());
        for (int i = 0; i < 5; i++) {
            Serial.printf("%d: %s\n", i, conversionName[i]);
        }
        Serial.println("");

        conversion_t type = (conversion_t) serialInput("conversion: ").toInt();
        addConversion(type);
        Serial.printf("conversionCount: %d\n", conversionCount);
        conversion[conversionCount - 1]->setViaSerial();
    }
}

bool InputModule::getStatus(JsonObject& status) {
    if (input == NULL) return true; // avoid errors: status might be called in setup before init occured

    status["averageTime"] = averageTime;
    status["updateTime"] = intervalMicroSeconds / 1000;
    status["stream"] = isStreaming;
    status["inputState"] = lastState;

    return true;
    // TODO: what about this?
    //moduleState["value"] = value; <-- make this variable accessible outside loop?
    //moduleState["triggerState"] = lastState;
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

void InputModule::handleCommand(String& controlId, JsonObject& command) {
    if (!isModule(controlId)) return;

    bool temp = false;
    if (getValue<bool>("getStatus", command, temp) && temp) {
        sendStatus();
    }

    if ( getValue<bool>("stream", command, temp) ) { 
        if (temp) {
            startStreaming();
        }
        else {
            stopStreaming();
        }
    }

    if (getValue<uint16_t>("averageTime", command, averageTime)) {
        input->averageTime(averageTime);
        sendStatus();
    }

    uint32_t interval;
    if ( getValue<uint32_t>("updateTime", command, interval) ) {
        intervalMicroSeconds = 1000 * interval;
        sendStatus();
    }

    if (!rangeChecking) return;

    getValue<double>("upperBound", command, hiBound);
    getValue<double>("lowerBound", command, loBound);

    //return true;
}

void InputModule::handleInternal(internalEvent eventId, String& sourceId) {
    switch(eventId) {
        case socket_disconnected: {
            //stop streaming
            if (!isStreaming) return;
            isStreaming = false;
            debug("stream stopped due to disconnect event");
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
    if (conversionCount == 16) {
        Serial.println("WARNING: maximum number of conversions reached");
        return;
    }

    switch (type) {
        case thermistor: {          
            conversion[conversionCount] = new Thermistor();
            conversionCount++;
            debug("thermistor conversion added");
            return;
        }
        
        case resistance_voltage_divider: {
            conversion[conversionCount] = new ResistanceDivider();
            conversionCount++;
            debug("resistance conversion added");
            return;
        }

        case map_value: {
            conversion[conversionCount] = new MapValue();
            conversionCount++;
            debug("mapping conversion added");
            return;
        }
        
        case offset: {
            conversion[conversionCount] = new Offset();
            conversionCount++;
            debug("offset conversion added");
            return;
        }

        case multiplication: {
            conversion[conversionCount] = new Multiplication();
            conversionCount++;
            debug("multiplication conversion added");
            return;
        }
    }
}