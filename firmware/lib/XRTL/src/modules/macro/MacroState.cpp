#include "MacroState.h"

MacroState::MacroState(String &name, XRTLmodule *parent) {
    stateName = name;
    macro = parent;
}

MacroState::~MacroState() {
    for (int i = 0; i < commandCount; i++) {
        delete commands[i];
    }
}

/**
 * @brief print all currently registered commands on the serial interface
 */
void MacroState::listCommands() {
    for (int i = 0; i < commandCount; i++) {
        printf("%i: %s\n", i, commands[i]->getId().c_str());
    }
}

/**
 * @brief add a new empty command
 * @note command must be filled manually after calling this function by using commands[commandCount - 1]->set()
 */
void MacroState::addCommand() {
    if (commandCount > 7) {
        Serial.println("maximum number of commands reached");
        return;
    }
    commands[commandCount++] = new XRTLsetableCommand();
}

/**
 * @brief add a new command
 * @param id controlId of the target module
 * @param key controlKey to use
 * @param val controlValue to accompany controlKey
 * @note the availability of controlKey and controlValue depend on the type of the target module
 */
void MacroState::addCommand(String &id, String &key, JsonVariant &val) {
    addCommand();

    if (val.is<bool>())
        commands[commandCount - 1]->set(id, key, val.as<bool>());
    else if (val.is<int>())
        commands[commandCount - 1]->set(id, key, val.as<long>());
    else if (val.is<float>())
        commands[commandCount - 1]->set(id, key, val.as<double>());
    if (val.is<String>())
        commands[commandCount - 1]->set(id, key, val.as<String>());
}

/**
 * @brief delete a specific command from the list
 * @param number number of the command in the commands list
 */
void MacroState::delCommand(uint8_t number) {
    if (commands[number] == NULL)
        delete commands[number];
    for (int i = number; i < commandCount - 1; i++) {
        commands[i] = commands[i + 1];
    }
    commands[commandCount--] = NULL;
}

/**
 * @brief swap two specific commands in the list
 * @param firstNumber number of the first command
 * @param secondNumber number of the second command
 */
void MacroState::swpCommand(uint8_t firstNumber, uint8_t secondNumber) {
    XRTLcommand *temp = commands[firstNumber];
    commands[firstNumber] = commands[secondNumber];
    commands[secondNumber] = temp;
}

/**
 * @brief dialog for managing the settings of the state via serial interface, polls data from the user
 * @returns true if the dialog should be called another time; false if the user asked to return to the previous menu
 */
bool MacroState::dialog() {
    Serial.println(centerString(stateName, 39, ' '));
    Serial.println("");
    listCommands();
    Serial.println("");

    Serial.println("a: add command");
    Serial.println("d: delete command");
    Serial.println("s: swap commands");
    Serial.println("c: change state name");
    Serial.printf("r: return");

    Serial.println("");
    String choice = serialInput("send single letter or number to edit command: ");
    uint8_t choiceInt = choice.toInt();

    if (choice == "r")
        return false;
    else if (choice == "a") {
        addCommand();
        commands[commandCount - 1]->setViaSerial();
    } else if (choice == "d") {
        Serial.println("");
        Serial.println("current commands:");
        listCommands();
        Serial.println("");
        choice = serialInput("send number to delete command: ");
        choiceInt = choice.toInt();

        if (choiceInt < commandCount) {
            delCommand(choiceInt);
        }
    } else if (choice == "s") {
        Serial.println("");
        Serial.println("send numbers of two commands to swap: ");
        uint8_t firstNumber = serialInput("first command: ").toInt();
        uint8_t secondNumber = serialInput("second command: ").toInt();
        swpCommand(firstNumber, secondNumber);
    } else if (choice == "c") {
        stateName = serialInput("new state name: ");
    } else if (choiceInt < commandCount) {
        commands[choiceInt]->setViaSerial();
    }
    return true;
}

/**
 * @brief start activating the state
 * @note loop() needs to be called in order to actually execute the commands
 */
void MacroState::activate() {
    macro->debug("activated <%s>", stateName);
    currentCommand = 0;
}

/**
 * @brief checks whether or not the state has been executed completely
 * @returns true if completed
 */
bool MacroState::isCompleted() {
    return (currentCommand >= commandCount);
}

/**
 * @brief execute the next command
 * @note will automatically stop if the end is reached
 */
void MacroState::loop() {
    if (isCompleted()) {
        macro->stop();
        return;
    }

    macro->sendCommand(*commands[currentCommand++]);
}

/**
 * @brief get the stored name of the state
 * @returns name of the state as String
 */
String MacroState::getName() {
    return stateName;
}

/**
 * @brief returns a command relative to the one currently to be executed
 * @param distance number of steps to take relative to the current command; may be negative
 * @returns pointer to the queried command; NULL if distance leads to invalid command number
 */
XRTLcommand *MacroState::relCommand(int8_t distance) {
    int8_t query = currentCommand + distance;
    if (query < 0 || query >= commandCount)
        return NULL;

    return commands[query];
}

/**
 * @brief save all commands in the provided JsonArray
 * @param settings JsonArray to store all necessary information in
 */
void MacroState::saveSettings(JsonArray &settings) {
    for (int i = 0; i < commandCount; i++) {
        JsonObject commandSettings = settings.createNestedObject();
        commands[i]->saveSettings(commandSettings);
    }
}

/**
 * @brief load commands from the provided settings file
 * @param settings JsonArray containing commands to load
 */
void MacroState::loadSettings(JsonArray &settings) {
    for (JsonObject commandSettings : settings) {
        String commandId = loadValue("id", commandSettings, "");
        if (commandId == "")
            continue;

        commandSettings.remove("id");
        String commandKey;
        JsonVariant commandVal;
        for (JsonPair commandKV : commandSettings) {
            commandKey = commandKV.key().c_str();
            commandVal = commandKV.value();
        }
        Serial.printf("%s:{%s:%s}\n", commandId.c_str(), commandKey, commandVal.as<String>().c_str());
        addCommand(commandId, commandKey, commandVal);
    }
}

/**
 * @brief manage the state settings using the serial interface
 */
void MacroState::setViaSerial() {
    while (dialog()) {
    }
}