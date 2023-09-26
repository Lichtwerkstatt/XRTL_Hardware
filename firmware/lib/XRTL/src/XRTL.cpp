#include "XRTL.h"

XRTL::~XRTL() {
    for (int i = 0; i < moduleCount; i++) {
        delete module[i];
    }

    for (int i = 0; i < internalCount; i++) {
        delete customInternal[i];
    }
}

void XRTL::setup() {
    Serial.begin(115200);

    // debug("starting setup");
    loadSettings();

    // execute setup of all modules
    for (int i = 0; i < moduleCount; i++) {
        module[i]->setup();
    }

    debug("setup complete");
}

void XRTL::loop() {
    // execute loop of each module
    for (int i = 0; i < moduleCount; i++) {
        // debug("loop <%s>", module[i]->getID().c_str());
        module[i]->loop();
        // debug("executed successfully");
    }

    if (!Serial.available()) return;

    // allow to switch into debug mode
    String input = sanitizeBackSpace(Serial.readStringUntil('\n'));
    if (input == "debug") {
        debugging = !debugging;
        Serial.printf("[%s] debug mode %sabled\n", id.c_str(), debugging ? "en" : "dis" );
    }

    // only allow inputs if debugging
    if (!debugging) return;

    if (input == "setup") {
        setViaSerial();
    } else if (input == "debug") { // do not interprete debug switch as event
    } else { // parse input as event
        DynamicJsonDocument serialEvent(1024);
        DeserializationError error = deserializeJson(serialEvent, input);
        if (error) {
            Serial.printf("[%s] deserializeJson() failed on serial input: %s\n", id.c_str(), error.c_str());
            Serial.printf("[%s] input: %s\n", id.c_str(), input.c_str());
        } else if (socketIO != NULL) { // make sure the socket is initialized
            socketIO->handleEvent(serialEvent);
        }
    }
}

/**
 * @brief add a software module to the manager
 * @param moduleName controlId of the new module
 * @param moduleType integer, codes for the type of module
 * @note the controlId should be unique, otherwise individual control is not possible anymore. The following IDs are restricted and should not be used: "*", "none", "core", "internal"
*/
bool XRTL::addModule(String moduleName, moduleType category) {
    if (moduleCount == 16) {
        debug("unable to add module: maximum number of modules reached");
        return false;
    }

    if (moduleName == 0 || moduleName == "") {
        debug("unable to add module: ID must not be empty");
        return false;
    }

    if (this->operator[](moduleName) != NULL) {
        debug("unable to add module <%s>: ID already in use", moduleName.c_str());
        return false;
    }

    if (moduleName == "*" || moduleName == "none" || moduleName == "core" || moduleName == "internal") {
        debug("unable to add module <%s>: ID restricted to internal use", moduleName);
        return false;
    }

    bool addSuccessful = false;
    switch (category) {
    case xrtl_socket: {
        socketIO = new SocketModule(moduleName);
        module[moduleCount] = socketIO;
        addSuccessful = true;
        break;
    }
    case xrtl_wifi: {
        module[moduleCount] = new WifiModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_infoLED: {
        module[moduleCount] = new InfoLEDModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_stepper: {
        module[moduleCount] = new StepperModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_servo: {
        module[moduleCount] = new ServoModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_camera: {
        module[moduleCount] = new CameraModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_output: {
        module[moduleCount] = new OutputModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_input: {
        module[moduleCount] = new InputModule(moduleName);
        addSuccessful = true;
        break;
    }
    case xrtl_macro: {
        module[moduleCount] = new MacroModule(moduleName);
        addSuccessful = true;
        break;
    }
    }

    if (addSuccessful) {
        debug("<%s> module added: <%s>", moduleNames[category], moduleName.c_str());
        module[moduleCount++]->setLinks(this, &debugging);
    }

    return addSuccessful;
}

/**
 * @brief print the controlIds of all available modules to the serial monitor
*/
void XRTL::listModules() {
    for (int i = 0; i < moduleCount; i++) {
        Serial.printf("%d: %s\n", i, module[i]->getID().c_str());
    }
}

/**
 * @brief dialog for the serial interfac allowing to delete modules
 * @param number index of the module to delete in the module array
 * @note since changes to the settings are stored in the modules after loading, those are lost after deletion. Only settings written to flash survive a restart.
*/
void XRTL::delModule(uint8_t number) {
    if (number < 0 || number >= moduleCount)
        return;
    // any number not in range defaults as exit methode

    String deletedName = module[number]->getID();
    Serial.printf("[%s] deleting <%s> ... ", id.c_str(), deletedName.c_str());
    delete module[number];
    for (int i = number; i < moduleCount - 1; i++) {
        module[i] = module[i + 1];
    }
    moduleCount--;
    Serial.println("done");
}

/**
 * @brief dialog for the serial interface allowing to swap to modules
 * @param numberX index of the first module in the module array
 * @param numberY index of the second module in the module array
*/
void XRTL::swapModules(uint8_t numberX, uint8_t numberY) {
    if (numberX == numberY || numberX < 0 || numberX >= moduleCount || numberY < 0 || numberY >= moduleCount)
        return;

    XRTLmodule *temp = module[numberX];
    module[numberX] = module[numberY];
    module[numberY] = temp;

    debug("swapped <%s> and <%s>", module[numberX]->getID().c_str(), module[numberY]->getID().c_str());
}

/**
 * @brief search all available modules for a match in their controlId
 * @param moduleName search for a match in the `controlId`s of all modules
 * @returns pointer to the module that matches the controlId or NULL if no match was found
*/
XRTLmodule *XRTL::operator[](String moduleName) {
    for (int i = 0; i < moduleCount; i++) {
        if (module[i]->isModule(moduleName)) {
            return module[i];
        }
    }
    return NULL; // probably not a good idea as default, return a special null module instead?
}

/**
 * @brief write the settings of all modules to the flash
*/
void XRTL::saveSettings() {
    DynamicJsonDocument doc(4096);
    JsonObject settings = doc.to<JsonObject>();

    settings["debug"] = debugging;

    for (int i = 0; i < moduleCount; i++) {
        module[i]->saveSettings(settings);
    }

    if (internalCount > 0) {
        JsonArray internalEventSettings = settings.createNestedArray("internal");
        for (int i = 0; i < internalCount; i++) {
            JsonObject currentSettings = internalEventSettings.createNestedObject();
            customInternal[i]->save(currentSettings);
        }
    }

    // serializeJsonPretty(doc, Serial);
    Serial.println("");

    if (!LittleFS.begin(false)) {
        debug("failed to mount LittleFS");
        debug("trying to format LittleFS");
        if (!LittleFS.begin(true)) {
            debug("failed to mount LittleFS again");
            debug("unable to format LittleFS");
            debug("unable to save settings");
            return;
        }
    }

    File file = LittleFS.open("/settings.txt", "w");
    if (serializeJsonPretty(doc, file) == 0) {
        debug("failed to write file");
        debug("could not save settings");
    } else {
        debug("settings successfully saved");
    }
    file.close();
    LittleFS.end();
}

/**
 * @brief load the settings from flash and add modules as specified in there
*/
void XRTL::loadSettings() {

    if (!LittleFS.begin(false)) {
        debug("failed to mount LittleFS");
        debug("trying to format LittleFS");
        if (!LittleFS.begin(true)) {
            debug("failed to mount LittleFS again");
            debug("unable to format LittleFS");
            debug("could not load settings");
            return;
        } else {
            debug("successfully formated file system");
            debug("creating new settings file");

            saveSettings();
        }
    }

    File file = LittleFS.open("/settings.txt", "r");
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, file);
    if (error) {
        debug("deserializeJson() failed while loading settings: <%s>", error.c_str());
    }
    file.close();
    LittleFS.end();

    // serializeJsonPretty(doc, Serial);
    Serial.println("");

    JsonObject settings = doc.as<JsonObject>();

    debugging = loadValue("debug", settings, true);
    
    if (debugging) {
        highlightString("loading settings", '=');
    }
    
    JsonArray internalSettings = settings["internal"];
    if (!internalSettings.isNull()) {
        if (debugging)
            highlightString("custom internal events", '-');

        for (JsonObject eventSettings : internalSettings) {
            customInternal[internalCount] = new InternalHook;
            customInternal[internalCount++]->load(eventSettings, debugging);
        }
    }

    for (JsonPair kv : settings) {
        moduleType type;
        if ((!kv.value().isNull()) and (kv.value().is<JsonObject>())) {
            // JsonObject moduleSettings = kv.value().as<JsonObject>();
            auto typeField = kv.value()["type"];
            // auto typeField = moduleSettings["type"];
            if ((!typeField.isNull()) && (typeField.is<int>())) {
                type = typeField.as<moduleType>();
                addModule(kv.key().c_str(), type);
                module[moduleCount - 1]->loadSettings(settings);
            }
        }
    }

    if (moduleCount == 0) { //
        debug("WARNING: no modules found, adding socket and wifi module");
        addModule("socket", xrtl_socket);
        addModule("wifi", xrtl_wifi);
        saveSettings();
        ESP.restart();
    }


    if (debugging)
        highlightString("loading successfull", '=');
}

/**
 * @brief dialog for the serial interface allowing to manage all settings
*/
bool XRTL::settingsDialog() {
    highlightString("modules:", '-');
    listModules();
    Serial.println("");
    Serial.println("a: add module");
    Serial.println("d: delete module");
    Serial.println("s: swap modules");
    Serial.println("i: internal events");
    Serial.println("b: disable debug mode");
    Serial.println("r: save and restart");
    Serial.println("");
    String choice = serialInput("choose single letter or number from above: ");
    uint8_t choiceInt = choice.toInt();

    if (choice == "a") {
        highlightString("add module", '-');
        Serial.println("module type is determined by number, available types:");
        Serial.println("");
        for (int i = 0; i < 9; i++) {
            Serial.printf("%d: %s\n", i, moduleNames[i]);
        }
        Serial.println("");
        Serial.println("r: return");
        Serial.println("");

        choice.clear();
        choice = serialInput("send number: ");
        choiceInt = choice.toInt();

        if (choice != "r" && choiceInt < 9) {
            moduleType newModuleType = (moduleType)choiceInt;
            String newModuleName = serialInput("new module name: ");
            if (addModule(newModuleName, newModuleType)) {
                JsonObject emptySettings;
                module[moduleCount - 1]->loadSettings(emptySettings); // initialize with default parameters
                module[moduleCount - 1]->setup();                     // run setup if necessary
            }
        }
    } else if (choice == "d") {
        highlightString("remove module", '-');
        Serial.println("choose module to delete: ");
        Serial.println("");
        listModules();
        Serial.println("");
        Serial.println("r: return");
        Serial.println("");

        choice.clear();
        choice = serialInput("choose number to delete: ");

        if (choice != "r") {
            uint8_t deleteChoice = choice.toInt();
            delModule(deleteChoice);
        }
    } else if (choice == "s") {
        highlightString("swap modules", '-');
        Serial.println("choose two modules to swap:");
        Serial.println("");
        listModules();
        Serial.println("");
        Serial.println("r: exit");
        Serial.println("");

        String firstModule = serialInput("first module: ");

        if (firstModule != "r") {
            String secondModule = serialInput("second module: ");
            if (secondModule != "r") {
                swapModules((uint8_t)firstModule.toInt(), (uint8_t)secondModule.toInt());
            }
        }
    } else if (choice == "i") {
        // add internal event hooks
        manageInternal();
    } else if (choice == "b") {
        debugging = false;
        Serial.printf("\ndebugging %sabled\n\n", debugging ? "en" : "dis" );
    } else if (choice == "r") {
        return false;
    } else if (choiceInt < moduleCount) {
        module[choiceInt]->setViaSerial();
    } else {
        Serial.printf("setup routine <%s> unknown\n", choice);
    }

    return true;
}

/**
 * @brief call the settings dialog, save the settings and restart after all changes have been made
*/
void XRTL::setViaSerial() {
    stop();

    highlightString("serial setup", '=');

    // repeat dialog until it is intentionally exited
    while (settingsDialog()) {
    }

    saveSettings();

    highlightString("setup complete", '=');

    Serial.println("restarting now");
    ESP.restart();
}

/**
 * @brief call the stop methode of all available modules
*/
void XRTL::stop() {
    debug("stopping all modules");

    for (int i = 0; i < moduleCount; i++) {
        module[i]->stop();
    }
}

/**
 *
 * @brief instruct core to send the status of this module
 * @note content of the status is filled by getStatus(), if getStatus() returns false
 */
void XRTLmodule::sendStatus() {
    DynamicJsonDocument doc(1024);
    JsonArray event = doc.to<JsonArray>();
    event.add("status");

    JsonObject payload = event.createNestedObject();
    payload["controlId"] = id;

    JsonObject status = payload.createNestedObject("status");
    if (!getStatus(status))
        return;
    xrtl->sendEvent(event);
}

/**
 *
 * @brief send the status of all modules that provide status information to the server
 * @note SocketModule::sendAllStatus() is the only methode of a XRTLmodule that can trigger this
 */
void XRTL::sendStatus() {
    for (int i = 0; i < moduleCount; i++) {
        module[i]->sendStatus();
    }
}

/**
 *
 * @brief send the status of all modules that provide status information to the server
 */
void SocketModule::sendAllStatus() {
    xrtl->sendStatus();
}

/**
 *
 * @brief send event via socket module
 * @param event reference to the event to send (JsonArray)
 * @note event Format: [<event name>,{<payload>}]
 */
void XRTL::sendEvent(JsonArray &event) {
    if (socketIO == NULL) {
        debug("unable to send event: no endpoint module");
        return;
    }
    socketIO->sendEvent(event);
}

/**
 *
 * @brief instruct core to send an event
 * @param event reference to the event to send (JsonArray)
 * @note event Format: [<event name>,{<payload>}]
 */
void XRTLmodule::sendEvent(JsonArray &event) {
    xrtl->sendEvent(event);
}

/**
 *
 * @brief send binary data via the endpoint
 * @param binaryLeadFrame complete websocket frame to be sent prior to the data frame. Must include placeholder for data, see note
 * @param payload pointer to the data to be included
 * @param length size of the data object to be sent
 * @note placeholder: 451-[<event name>,{<additional payload>,{"_placeholder":true,"num":0}}]
 */
void XRTL::sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length) {
    if (socketIO == NULL) {
        debug("unable to send event: no endpoint module");
    }
    socketIO->sendBinary(binaryLeadFrame, payload, length);
}

/**
 *
 * @brief send binary data via the endpoint
 * @param binaryLeadFrame complete websocket frame to be sent prior to the data frame. Must include placeholder for data, see note
 * @param payload pointer to the data to be included
 * @param length size of the data object to be sent
 * @note placeholder: 451-[<event name>,{<additional payload>,{"_placeholder":true,"num":0}}]
 */
void XRTLmodule::sendBinary(String &binaryLeadFrame, uint8_t *payload, size_t length) {
    xrtl->sendBinary(binaryLeadFrame, payload, length);
}

/**
 *
 * @brief send command to a module
 * @param command reference to the command to send
 * @param userId reference to the String used as userId in the command message
 * @note if the requested controlId can not be found on the hardware, the command will be send to the socket server instead
 */
void XRTL::sendCommand(XRTLcommand &command, const String &userId = "") {
    String &controlId = command.getId();
    XRTLmodule *targetModule = operator[](controlId);
    DynamicJsonDocument doc(512);

    if (targetModule != NULL) { // module located on this hardware
        JsonObject commandObj = doc.to<JsonObject>();

        command.fillCommand(commandObj);
        targetModule->handleCommand(controlId, commandObj);
    }
    else {
        JsonArray event = doc.to<JsonArray>();
        event.add("command");
        JsonObject commandObj = event.createNestedObject();

        if (userId == "") {
            commandObj["userId"] = getComponent(); // only use default if no socket module is present
        }
        else {
            commandObj["userId"] = userId;
        }

        command.fillCommand(commandObj);
        sendEvent(event);
    }
}

/**
 *
 * @brief send command to a module
 * @param command reference to the command to send
 * @note if the requested controlId can not be found on the hardware, the command will be send to the socket server instead
 */
void XRTLmodule::sendCommand(XRTLcommand &command) {
    xrtl->sendCommand(command, id);
}

/**
 *
 * @brief send internal event to all modules
 * @param eventId type of the occuring event, see internalEvent
 * @param sourceId controlId of the module issuing the event
 * @note events can be reacted on by using handleInternal() or internal hook of the core
 */
void XRTL::notify(internalEvent eventId, String &sourceId) {
    // notify modules
    for (int i = 0; i < moduleCount; i++) {
        module[i]->handleInternal(eventId, sourceId);
    }

    // core event handler
    switch (eventId) {
    }

    for (int i = 0; i < internalCount; i++) {
        InternalHook *hook = customInternal[i];
        if (hook->isTriggered(eventId, sourceId)) {
            sendCommand(hook->getCommand());
        }
    }
}

/**
 *
 * @brief send internal event to all modules
 * @param eventId type of the occuring event, see internalEvent
 * @note events can be reacted on by using handleInternal() or internal hook of the core
 */
void XRTLmodule::notify(internalEvent state) {
    xrtl->notify(state, id);
}

/**
 * @brief fetch the component name from the socket module
 * @returns name stored in the socketmodule (not the controlId) or "core" if no socket module is present
*/
String &XRTL::getComponent() {
    if (socketIO == NULL) {
        debug("WARNING: no socket module present");
        return id;
    }
    return socketIO->getComponent();
}

/**
 * @brief 
*/
String &XRTLmodule::getComponent() {
    return xrtl->getComponent();
}

/**
 * @brief offer the seperated controlId and the entire command to all modules available
 * @param controlId String holding the ID of the addressed module
 * @param command JsonObject holding the entire command
 * @note the command is offered to every module, but the modules themself decide whether it is relevant for them or not
*/
void XRTL::pushCommand(String &controlId, JsonObject &command) {
    for (int i = 0; i < moduleCount; i++) {
        module[i]->handleCommand(controlId, command);
    }
}

/**
 * @brief offer the seperated controlId and the entire status to all modules available
 * @param controlId String holding the ID of the sending module
 * @param status JsonObject holding the entire status
 * @note the status is offered to every module, but the modules themself decide whether it is relevant for them or not
*/
void XRTL::pushStatus(String &controlId, JsonObject &status) {
    for (int i = 0; i < moduleCount; i++) {
        module[i]->handleStatus(controlId, status);
    }
}


/**
 * @brief forwards the entire command as well as the already extracted controlId to the module manager
 * @param controlId String holding the ID of the addressed module
 * @param command JsonObject holding the entire command (including controlId)
 * @note the controlId should be extracted before pushing to avoid multiple modules searching for it on their own. The macro module can also push Commands but uses a different methode to avoid a feedback loop.
*/
void SocketModule::pushCommand(String &controlId, JsonObject &command) {
    xrtl->pushCommand(controlId, command);
}

/**
 * @brief forwards the entire status as well as the already extracted controlId to the module manager
 * @param controlId String holding the ID of the sending module
 * @param status JsonObject holding the entire status (including controlId)
 * @note the controlId should be extracted before pushing to avoid multiple modules searching for it on their own
*/
void SocketModule::pushStatus(String &controlId, JsonObject &status) {
    xrtl->pushStatus(controlId, status);
}

/**
 *
 * @brief send an error via the endpoint
 * @param ernr refers to the errortype, see componentError for details
 * @param message errormessage as single string
 */
void XRTL::sendError(componentError ernr, String msg) {
    if (socketIO == NULL) {
        debug("unable to send event: no endpoint module");
        return;
    }
    socketIO->sendError(ernr, msg);
}

/**
 *
 * @brief send an error via the endpoint
 * @param ernr refers to the errortype, see componentError for details
 * @param message errormessage as single string
 */
void XRTLmodule::sendError(componentError ernr, String msg) {
    xrtl->sendError(ernr, msg);
}

/**
 * @brief dialog for the serial interface, allows to edit the internal listener settings
*/
void XRTL::manageInternal() {
    highlightString("internal events", '-');
    Serial.println("");
    Serial.println("current internal event listeners:");
    Serial.println("");

    for (int i = 0; i < internalCount; i++) {
        Serial.printf("%d: %s, %s -> %s \n",
                      i,
                      internalEventNames[customInternal[i]->getType()],
                      customInternal[i]->getId().c_str(),
                      customInternal[i]->getCommand().getId().c_str());
    }

    Serial.println("");
    Serial.println("a: add listener");
    Serial.println("s: swap listeners");
    Serial.println("d: delete listener");
    Serial.println("r: return");
    Serial.println("");

    String choice = serialInput("send single letter or number to edit listeners: ");
    uint8_t choiceNum = choice.toInt();

    if (choice == "r")
        return;
    else if (choice == "a")
        addInternal();
    else if (choice == "s")
        swpInternal();
    else if (choice == "d")
        delInternal();
    else if (choiceNum < internalCount) {
        customInternal[choiceNum]->setViaSerial();
    }
}

/**
 * @brief dialog for the serial interface, allows to add an internal listener
*/
void XRTL::addInternal() {
    if (internalCount >= 16) {
        Serial.println("maximum event count reached");
        return;
    }

    Serial.println("");
    Serial.println("available event types:");
    Serial.println("");
    for (int i = 0; i < 10; i++) {
        Serial.printf("%d: %s\n", i, internalEventNames[i]);
    }

    Serial.println("");
    uint8_t choiceNum = serialInput("send number to choose event type for listener: ").toInt();
    if (choiceNum > 11) return;
    internalEvent eventType = (internalEvent)choiceNum;

    String listeningId = serialInput("send string to specify controlId to listen for: ");
    if (listeningId == "core") return;

    customInternal[internalCount] = new InternalHook();
    customInternal[internalCount]->set(eventType, listeningId);
    customInternal[internalCount++]->getCommand().setViaSerial();
}

/**
 * @brief dialog for the serial interface, allows to swap two internal listeners
*/
void XRTL::swpInternal() {
    if (internalCount < 2) {
        debug("not enough listeners for swapping");
        return;
    }

    Serial.println("send the number of two listeners to swap");
    uint8_t choiceNum1 = serialInput("first listener: ").toInt();
    uint8_t choiceNum2 = serialInput("second listener:").toInt();

    if (choiceNum1 > internalCount || choiceNum2 > internalCount || choiceNum1 == choiceNum2)
        return;

    InternalHook *tmp = customInternal[choiceNum1];
    customInternal[choiceNum1] = customInternal[choiceNum2];
    customInternal[choiceNum2] = tmp;
}

/**
 * @brief dialog for the serial interface, allows to delete an internal listener
*/
void XRTL::delInternal() {
    Serial.println("current event listeners:");
    Serial.println("");
    for (int i = 0; i < internalCount; i++) {
        Serial.printf("%d: %s, %s -> %s \n",
                      i,
                      internalEventNames[customInternal[i]->getType()],
                      customInternal[i]->getId().c_str(),
                      customInternal[i]->getCommand().getId().c_str());
    }
    Serial.println("");

    uint8_t choiceNum = serialInput("send number to delete: ").toInt();

    if (choiceNum >= internalCount)
        return;

    delete customInternal[choiceNum];

    for (int i = choiceNum; i < internalCount; i++) {
        customInternal[i] = customInternal[i + 1];
    }
    internalCount--;
}