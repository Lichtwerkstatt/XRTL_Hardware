#include "InputModule.h"

InputModule::InputModule(String moduleName)
{
    id = moduleName;

    parameters.setKey(id);
    convParameters.setKey("conversions");

    parameters.add(pin, "pin", "int");
    parameters.add(type, "type");
    parameters.add(averageTime, "averageTime", "ms");
    parameters.add(rangeChecking, "rangeChecking", "");
    parameters.addDependent(isBinary, "isBinary", "y/n", "rangeChecking", true);
    parameters.addDependent(loBound, "loBound", "float", "rangeChecking", true);
    parameters.addDependent(hiBound, "hiBound", "float", "rangeChecking", true);
    parameters.add(deadMicroSeconds, "deadMicroSeconds", "µs");
}

InputModule::~InputModule()
{
    for (int i = 0; i < conversionCount; i++)
    {
        delete conversion[i];
    }
    delete input;
}

void InputModule::setup()
{
    input = new XRTLinput;
    input->attach(pin);
    input->averageTime(averageTime); // in ms
    if (input->readMilliVolts() >= hiBound)
    { // initialize lastState
        lastState = true;
    }
    else
    {
        lastState = false;
    }

    next = esp_timer_get_time();      // start streaming immediately
    nextCheck = esp_timer_get_time(); // check immediately
}

void InputModule::loop()
{
    input->loop();

    int64_t now = esp_timer_get_time();
    value = input->readMilliVolts();
    // apply conversion if defined
    for (int i = 0; i < conversionCount; i++)
    {
        conversion[i]->convert(value);
    }

    if (rangeChecking && now >= nextCheck)
    {
        if (value >= hiBound)
        {
            if (isBinary && lastState) // no trigger if state = lastState
            {
            }
            else
            {
                debug("high input");
                notify(input_trigger_high);
                lastState = true;
                nextCheck = now + deadMicroSeconds;
            }
        }
        if (value < loBound)
        {
            if (isBinary && !lastState) // no trigger if state = lastState
            {
            }
            else
            {
                debug("low input");
                notify(input_trigger_low);
                lastState = false;
                nextCheck = now + deadMicroSeconds;
            }
        }
    }

    if (!isStreaming)
        return;

    if (now < next)
        return;
    // debug("reporting voltage: %f mV", value);
    next = now + intervalMicroSeconds;

    DynamicJsonDocument doc(512);
    JsonArray event = doc.to<JsonArray>();

    event.add("data");
    JsonObject payload = event.createNestedObject();
    // payload["componentId"] = getComponent(); // TODO: check function
    payload["controlId"] = id;
    payload["type"] = "float";
    // payload["dataId"] = id;

    JsonObject data = payload.createNestedObject("data");
    data["type"] = "Buffer"; // for consistency with other data transmissions, should look identical on the server side after parsing

    if (isBinary)
    {
        data["data"] = lastState;
    }
    else
    {
        data["data"] = value;
    }

    sendEvent(event);
}

void InputModule::stop()
{
    stopStreaming();
}

void InputModule::saveSettings(JsonObject &settings)
{
    JsonObject subSettings;
    parameters.save(settings, subSettings);

    // conversion settings
    if (conversionCount == 0)
        return;
    JsonArray conversionSettings = subSettings.createNestedArray("conversions");
    for (int i = 0; i < conversionCount; i++)
    {
        JsonObject saveConversionConfig = conversionSettings.createNestedObject();
        conversion[i]->saveSettings(saveConversionConfig);
    }
}

void InputModule::loadSettings(JsonObject &settings)
{
    JsonObject subSettings;
    parameters.load(settings, subSettings);

    // load conversions
    JsonArray loadedConversion = subSettings["conversions"];
    if (!loadedConversion.isNull())
    {
        for (JsonVariant value : loadedConversion)
        { // iterate over all objects within loadedConversion
            JsonObject conversionSettings = value.as<JsonObject>();
            auto typeField = conversionSettings["type"];
            conversion_t convType = loadValue<conversion_t>("type", conversionSettings, offset);
            addConversion(convType);
            conversion[conversionCount - 1]->loadSettings(conversionSettings, debugging);
            // if (debugging) Serial.println("");
        }
    }

    if (debugging)
        parameters.print();
}

void InputModule::setViaSerial()
{
    while (dialog())
    {
    }
}

bool InputModule::conversionDialog()
{
    Serial.println("");
    Serial.println(centerString("current conversions", 39, ' '));
    Serial.println("");

    for (int i = 0; i < conversionCount; i++)
    {
        Serial.printf("%d: %s\n", i, conversionName[conversion[i]->getType()]);
    }

    Serial.println("");
    Serial.println("a: add conversion");
    Serial.println("d: delete conversion");
    Serial.println("s: swap conversions");
    Serial.println("r: return");

    Serial.println("");
    String choice = serialInput("send single letter or number to edit conversion: ");
    uint8_t choiceNum = choice.toInt();

    if (choice == "r")
    {
        return false;
    }
    else if (choice == "a")
    {
        Serial.println(centerString("conversions available", 39, ' ').c_str());
        for (int i = 0; i < 6; i++)
        {
            Serial.printf("%d: %s\n", i, conversionName[i]);
        }
        Serial.println("");

        conversion_t type = (conversion_t)serialInput("send number to specify type: ").toInt();
        addConversion(type);
        Serial.printf("conversionCount: %d\n", conversionCount);
        conversion[conversionCount - 1]->setViaSerial();
    }
    else if (choice == "d")
    {
        Serial.println("");
        uint8_t deleteChoice = serialInput("send number to delete conversion: ").toInt();

        if (deleteChoice >= conversionCount - 1)
            return true;

        delete conversion[deleteChoice];

        for (int i = deleteChoice; i < conversionCount; i++)
        {
            conversion[i] = conversion[i + 1];
        }
        conversion[conversionCount--] = NULL;
    }
    else if (choice == "s")
    {
        Serial.println("");
        Serial.println("send the numbers of two conversions to swap");
        String choiceOne = serialInput("first conversion: ");
        if (choiceOne == "r")
            return true;
        String choiceTwo = serialInput("second conversion: ");
        if (choiceTwo == "r")
            return true;

        uint8_t choiceOneNum = choiceOne.toInt();
        uint8_t choiceTwoNum = choiceTwo.toInt();

        if (choiceOneNum < conversionCount || choiceTwoNum < conversionCount || choiceOneNum != choiceTwoNum)
        {
            InputConverter *tmp = conversion[choiceOneNum];
            conversion[choiceOneNum] = conversion[choiceTwoNum];
            conversion[choiceTwoNum] = tmp;
        }
    }
    else if (choiceNum < conversionCount)
    {
        conversion[choiceNum]->setViaSerial();
    }

    return true;
}

bool InputModule::dialog()
{
    Serial.println("");
    Serial.println(centerString("", 39, '-'));
    Serial.println(centerString(id.c_str(), 39, ' '));
    Serial.println(centerString("", 39, '-'));
    Serial.println("");

    Serial.println("available settings:");
    Serial.println("");
    Serial.println("b: basic settings");
    Serial.println("m: manage conversions");
    Serial.println("r: return");
    Serial.println("");

    String choice = serialInput("send single letter to edit settings: ");
    if (choice == "r")
        return false;
    else if (choice == "b")
        parameters.setViaSerial();
    else if (choice == "m")
    {
        while (conversionDialog())
        {
        }
    }

    return true;
}

bool InputModule::getStatus(JsonObject &status)
{
    if (input == NULL)
        return true; // avoid errors: status might be called in setup before init occured

    status["averageTime"] = averageTime;
    status["updateTime"] = intervalMicroSeconds / 1000;
    status["stream"] = isStreaming;

    if (isBinary)
    {
        status["input"] = lastState;
    }
    else
    {
        status["input"] = value;
    }

    return true;
    // TODO: what about this?
    // moduleState["value"] = value; <-- make this variable accessible outside loop?
    // moduleState["triggerState"] = lastState;
}

void InputModule::startStreaming()
{
    if (isStreaming)
    {
        debug("stream already active");
        return;
    }
    next = esp_timer_get_time(); // immediately deliver first value
    isStreaming = true;
    sendStatus();
    debug("starting to stream value");
}

void InputModule::stopStreaming()
{
    if (!isStreaming)
    {
        debug("stream already inactive");
        return;
    }
    isStreaming = false;
    sendStatus();
    debug("stopped streaming values");
}

bool InputModule::handleCommand(String &command)
{
    return false;
}

void InputModule::handleCommand(String &controlId, JsonObject &command)
{
    if (!isModule(controlId)  && controlId != "*")
        return;

    bool temp = false;
    if (getValue<bool>("getStatus", command, temp) && temp)
    {
        sendStatus();
    }

    if (getValue<bool>("stream", command, temp))
    {
        if (temp)
        {
            startStreaming();
        }
        else
        {
            stopStreaming();
        }
    }

    if (getValue<uint16_t>("averageTime", command, averageTime))
    {
        input->averageTime(averageTime);
        sendStatus();
    }

    uint32_t interval;
    if (getValue<uint32_t>("updateTime", command, interval))
    {
        intervalMicroSeconds = 1000 * interval;
        sendStatus();
    }

    if (!rangeChecking)
        return;

    getValue<double>("upperBound", command, hiBound);
    getValue<double>("lowerBound", command, loBound);

    // return true;
}

void InputModule::handleInternal(internalEvent eventId, String &sourceId)
{
    switch (eventId)
    {
    case socket_disconnected:
    {
        // stop streaming
        if (!isStreaming)
            return;
        isStreaming = false;
        debug("stream stopped due to disconnect event");
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
    }
}

void InputModule::addConversion(conversion_t type)
{
    if (conversionCount == 16)
    {
        Serial.println("WARNING: maximum number of conversions reached");
        return;
    }

    switch (type)
    {
    case thermistor:
    {
        conversion[conversionCount++] = new Thermistor();
        debug("thermistor conversion added");
        return;
    }

    case resistance_voltage_divider:
    {
        conversion[conversionCount++] = new ResistanceDivider();
        debug("resistance conversion added");
        return;
    }

    case voltage_voltage_divider:
    {
        conversion[conversionCount++] = new VoltageDivider();
        debug("voltage divider conversion added");
        return;
    }

    case map_value:
    {
        conversion[conversionCount++] = new MapValue();
        debug("mapping conversion added");
        return;
    }

    case offset:
    {
        conversion[conversionCount++] = new Offset();
        debug("offset conversion added");
        return;
    }

    case multiplication:
    {
        conversion[conversionCount++] = new Multiplication();
        debug("multiplication conversion added");
        return;
    }
    }
}