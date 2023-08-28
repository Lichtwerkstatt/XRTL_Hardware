#include "XRTLparameterPack.h"

XRTLpar ParameterPack::nullParameter;

void XRTLpar::load(JsonObject &settings){};
void XRTLpar::save(JsonObject &settings){};
void XRTLpar::print(){};

/**
 * @brief fetch the name of a parameter with regard to its intended visibility
 * @returns pointer to the name or NULL
 * @note returns NULL if the parameter is not supposed to be visible
*/
String *XRTLpar::getName(){return &name;}

/**
 * @brief check if parameter is empty
 * @returns true if parameter is uninitialized
 * @note base class only used for default parameter, specialized (= initialized) classes return false
 */
bool XRTLpar::isNull() {
    return true;
}

void XRTLpar::setViaSerial() {}

/**
 * @brief check if parameter name matches query
 * @param queryName string to be checked for match
 * @returns true if queryName matches internally used name
 */
bool XRTLpar::is(String &queryName) {
    return (queryName == name);
}

ParameterPack::~ParameterPack() {
    for (int i = 0; i < parameterCount; i++) {
        delete parameters[i];
    }
}

/**
 * @brief set the key of the parameter pack as string reference, used for storing and loading the parameters
 * @param key string to be used
 * @note key must not be destroyed before the ParameterPack, no copy is created!
 */
void ParameterPack::setKey(String &key) {
    linkedName = &key;
}

/**
 * @brief set the key of the parameter pack
 * @param key string to be used
 */
void ParameterPack::setKey(const char *key) {
    reserveName = key;
}

/**
 * @brief get the key of the parameter pack
 * @return pointer to the name
 */
String &ParameterPack::name() {
    if (linkedName) return *linkedName;
    else return reserveName;
}

/**
 * @brief save all parameters to the JsonObject provided
 * @param settings store parameters in here
 * @note if a key for the parameter object exists, a nested JsonObject with that name will be created and used
 */
void ParameterPack::save(JsonObject &settings) {
    JsonObject subSettings;
    save(settings, subSettings);
}

/**
 * @brief save all parameters to the JsonObject provided
 * @param settings store parameters in here
 * @param subSettings the address pointing to the used (possibly nested) JsonObject will be stored here
 * @note if a key for the parameter object exists, a nested JsonObject with that name will be created and used
 */
void ParameterPack::save(JsonObject &settings, JsonObject &subSettings) {
    if (name() != "") {
        subSettings = settings.createNestedObject(name());
    } else {
        subSettings = settings;
    }

    for (int i = 0; i < parameterCount; i++) {
        parameters[i]->save(subSettings);
    }
}

/**
 * @brief load all parameters from a JsonObject
 * @param settings JsonObject that the settings will be loaded from
 * @note if further access to the JsonObject the parameters were loaded form is needed, provide a second parameter to store the reference
 */
void ParameterPack::load(JsonObject &settings) {
    JsonObject subSettings;
    load(settings, subSettings);
}

/**
 * @brief load all the parameters from a JsonObject
 * @param settings JsonObject that the settings will be loaded from
 * @param subSettings JsonObject that will receive a pointer to the settings used
 */
void ParameterPack::load(JsonObject &settings, JsonObject &subSettings) {
    if (name() != "") {
        auto candidate = settings[name()];

        if (candidate.isNull() || !candidate.is<JsonObject>()) {
            subSettings = settings;
            return;
        }

        subSettings = candidate.as<JsonObject>();
    } else {
        subSettings = settings;
    }

    for (int i = 0; i < parameterCount; i++) {
        parameters[i]->load(subSettings);
    }
}

// TODO: is this needed?
void ParameterPack::setParent(ParameterPack *newParent) {
    parent = newParent;
}

/**
 * @brief print all listed parameters on the serial interface
 */
void ParameterPack::print() {
    if (name() != "") {
        highlightString(name().c_str(), '-');
    }

    for (int i = 0; i < parameterCount; i++) {
        parameters[i]->print();
    }

    Serial.println("");
}

/**
 * @brief search for a specific parameter by its identifying string
 * @param queryParam ID string to be searched for
 * @return pointer to the parameter if found, special null parameter if not
 * @note check whether the search was successful by using isNull()
 */
XRTLpar &ParameterPack::operator[](String queryParam) {
    for (int i = 0; i < parameterCount; i++) {
        if (parameters[i]->is(queryParam))
            return *parameters[i];
    }

    return nullParameter;
}

// TODO: add methode to access additional settings
/**
 * @brief dialog used by the parameter pack to manage settings via the serial interface
 * @return false if the dialog has ended, true if it should be repeated
 */
bool ParameterPack::dialog() {
    Serial.println("");
    highlightString(name().c_str(), '-');
    Serial.println("");

    for (int i = 0; i < parameterCount; i++) {
        if (parameters[i]->notListed) continue;

        Serial.printf("%d: ", i);
        parameters[i]->print();
    }

    Serial.println("");
    Serial.println("a: set all");
    if (linkedName)
        Serial.println("c: change controlId");
    Serial.println("r: return");

    Serial.println("");
    String rawInput = serialInput("send single letter or number to edit parameter: ");
    uint8_t inputNumber = rawInput.toInt();

    if (rawInput == "r") {
        return false;
    } else if (rawInput == "c" && linkedName) {
        *linkedName = serialInput("controlId (String) = ");
    } else if (rawInput == "a") {
        for (int i = 0; i < parameterCount; i++) {
            if (parameters[i]->notListed) continue;

            parameters[i]->setViaSerial();
        }
    } else if (inputNumber < parameterCount && !parameters[inputNumber]->notListed) {
        parameters[inputNumber]->setViaSerial();
    }

    return true;
}

void ParameterPack::setViaSerial() {
    while (dialog()) {
    }
}