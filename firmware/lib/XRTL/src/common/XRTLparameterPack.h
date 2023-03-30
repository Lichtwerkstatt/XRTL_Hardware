#ifndef PARAMETERPACK_H
#define PARAMETERPACK_H

#include "common/XRTLfunctions.h"

class XRTLpar {
    protected:
    String name;
    bool notListed = false;

    public:
    virtual void load(JsonObject& settings){};
    virtual void save(JsonObject& settings){};
    //virtual void setViaSerial(JsonObject& settings);
    virtual void print(){};

    // base class only used for default (null) parameter, specialized classes return false
    virtual bool isNull() {
        return true;
    }

    virtual void setViaSerial(){}

    bool is(String& queryName) {
        return (queryName == name);
    }

    friend class ParameterPack;
};

template <typename T>
class XRTLparameter : public XRTLpar {
    protected:
    T* parameter;
    String unit;

    public:
    XRTLparameter(){}
    
    XRTLparameter(T& linkedParameter, String& paramName, String& unitStr) {
        parameter = &linkedParameter;
        name = paramName;
        unit = unitStr;
    }

    XRTLparameter(T& linkedParameter, String& paramName) {
        parameter = &linkedParameter;
        name = paramName;
        notListed = true;
    }

    virtual void save(JsonObject& settings) {
        settings[name] = *parameter;
    }

    virtual void load(JsonObject& settings) {
        auto candidate = settings[name];
        if (candidate.isNull()) return;
        if (!candidate.is<T>()) return;

        *parameter = candidate.as<T>();
    }
    
    virtual void print() {
        if (notListed) return;
        Serial.print(name.c_str());
        Serial.print(" (");
        Serial.print(unit.c_str());
        Serial.print("): ");
        Serial.println(*parameter);
    }

    virtual void setViaSerial() {
        if (notListed) return;

        setViaSerialQuery();
    }

    void setViaSerialQuery() {
        String inputQuery = name;
        inputQuery += " (";
        inputQuery += unit;
        inputQuery += ") = ";

        StaticJsonDocument<64> doc;
        JsonArray input = doc.to<JsonArray>();     

        input.add(serialInput(inputQuery));

        if (input[0].isNull()) return;
        
        *parameter = input[0].as<T>();
    }

    bool isNull() {
        return false;
    }

    bool isEqual(T& testParam) {
        return (*parameter == testParam);
    }

    T& get() {
        return *parameter;
    }
};

template <>
class XRTLparameter<bool> : public XRTLpar {
    protected:
    bool* parameter;
    String unit = "y/n";

    public:
    XRTLparameter(){}

    XRTLparameter(bool& linkedParameter, String& paramName, String& unitStr) {
        parameter = &linkedParameter;
        name = paramName;
    }

    XRTLparameter(bool& linkedParameter, String& paramName) {
        parameter = &linkedParameter;
        name = paramName;
        notListed = true;
    }

    virtual void save(JsonObject& settings) {
        settings[name] = *parameter;
    }

    virtual void load(JsonObject& settings) {
        auto candidate = settings[name];
        if (candidate.isNull()) return;
        if (!candidate.is<bool>()) return;

        *parameter = candidate.as<bool>();
    }

    virtual void setViaSerial() {
        if (notListed) return;

        setViaSerialQuery();
    }

    void setViaSerialQuery() {
        String inputQuery = name;
        inputQuery += " (";
        inputQuery += unit;
        inputQuery += ") = ";

        String input = serialInput(inputQuery);

        if (input == "true" || input == "y" || input == "1" || input == "TRUE" || input == "Y") {
            *parameter = true;
        }
        else if (input == "false" || input == "n" || input == "0" || input == "FALSE" || input == "N") {
            *parameter = false;
        }
    }
    
    virtual void print() {
        Serial.printf("%s (%s): %s\n", name.c_str(), unit.c_str(), *parameter ? "true" : "false" );
    }

    bool isNull() {
        return false;
    }

    bool isEqual(bool& testParam) {
        return (*parameter == testParam);
    }

    bool& get() {
        return *parameter;
    }
};

template <typename T, typename U>
class XRTLdependentPar : public XRTLparameter<T> {
    private:
    XRTLparameter<U>* dependency;
    U condition;

    public:
    XRTLdependentPar(T& linkedParameter, String& paramName, String& unitStr, XRTLparameter<U>* dependencyPar, U& dependencyCond) {
        this->parameter = &linkedParameter;
        this->name = paramName;
        this->unit = unitStr;
        dependency = dependencyPar;
        condition = dependencyCond;
    }

    XRTLdependentPar(T& linkedParameter, String& paramName, XRTLparameter<U>* dependencyPar, U& dependencyCond) {
        this->parameter = &linkedParameter;
        this->name = paramName;
        this->notListed = true;
        dependency = dependencyPar;
        condition = dependencyCond;
    }

    virtual void save(JsonObject& settings) {
        if (!dependency->isEqual(condition)) return;
        settings[this->name] = *this->parameter;
    }

    virtual void load(JsonObject& settings) {
        if (!dependency->isEqual(condition)) return;
        auto candidate = settings[this->name];
        if (candidate.isNull()) return;
        if (!candidate.template is<T>()) return;

        *this->parameter = candidate.template as<T>();
    }
    
    virtual void print() {
        if (!dependency->isEqual(condition)) return;
        if (this->notListed) return;
        Serial.print(this->name.c_str());
        Serial.print(" (");
        Serial.print(this->unit.c_str());
        Serial.print("): ");
        Serial.println(*this->parameter);
    }

    void setViaSerial() {
        if (!dependency->isEqual(condition)) return;
        if (this->notListed) return;

        this->setViaSerialQuery();
    }
};

class ParameterPack {
    protected:
    uint8_t parameterCount = 0;
    XRTLpar* parameters[16];
    ParameterPack* parent = NULL;
    String* linkedName = NULL;
    String reserveName;

    static XRTLpar nullParameter;

    public:
    ParameterPack () {}

    void setKey(String& key) {
        linkedName = &key;
    }

    void setKey(const char* key){
        reserveName = key;
    }

    String& name() {
        if (linkedName != NULL) return *linkedName;
        else return reserveName;
    }

    // @brief save all parameters to the JsonObject provided
    // @param settings store parameters in here
    // @note if a key for the parameter object exists, a nested JsonObject with that name will be created and used
    void save(JsonObject& settings) {
        JsonObject subSettings;
        save(settings, subSettings);
    }

    // @brief save all parameters to the JsonObject provided
    // @param settings store parameters in here
    // @param subSettings the address pointing to the used (possibly nested) JsonObject will be stored here
    // @note if a key for the parameter object exists, a nested JsonObject with that name will be created and used
    void save(JsonObject& settings, JsonObject& subSettings) {
        if (name() != "") {
            subSettings = settings.createNestedObject(name());
        }
        else {
            subSettings = settings;
        }

        for (int i = 0; i < parameterCount; i++) {
            parameters[i]->save(subSettings);
        }
    }

    void load(JsonObject& settings) {
        JsonObject subSettings;
        load(settings, subSettings);
    }

    void load(JsonObject& settings, JsonObject& subSettings) {
        if (name() != "") {
            auto candidate = settings[name()];

            if (candidate.isNull() || !candidate.is<JsonObject>()) {
                subSettings = settings;
                return;
            }

            subSettings = candidate.as<JsonObject>();
        }
        else {
            subSettings = settings;
        }

        for (int i = 0; i < parameterCount; i++) {
            parameters[i]->load(subSettings);
        }
    }


    // @brief add a parameter that automatically gets loaded from and saved to flash; setable via serial interface
    // @param linkedParameter parameter that gets linked, must exist in all contexts of the parameter object and can be changed by it
    // @param paramName identifier string for the parameter
    // @param unit string representing the physical meaning of the parameter (visible in query procedure)
    template <typename T>
    void add(T& linkedParameter, String paramName, String unit) {
        if (parameterCount == 16) return;

        parameters[parameterCount++] = new XRTLparameter<T>(linkedParameter, paramName, unit);
    }

    // @brief add a parameter that automatically gets loaded from and saved to flash
    // @param linkedParameter parameter that gets linked, must exist in all contexts of the parameter object and can be changed by it
    // @param paramName identifier string for the parameter
    // @param unit string representing the physical meaning of the parameter (visible in query procedure)
    // @note parameter does not get listed and is inaccessible via serial interface, good for internal parameters
    template <typename T>
    void add(T& linkedParameter, String paramName) {
        if (parameterCount == 16) return;
        
        parameters[parameterCount++] = new XRTLparameter<T>(linkedParameter, paramName);
    }

    template <typename T, typename U>
    void addDependent(T& linkedParameter, String paramName, String unit, String dependee, U condition) {
        if (parameterCount == 16) return;

        XRTLpar* dependency = &operator[](dependee);
        if (dependency->isNull()) return; // check if parameter exists
        XRTLparameter<U>* targetPtr = static_cast<XRTLparameter<U> *>(dependency); // convert pointer

        parameters[parameterCount++] = new XRTLdependentPar<T,U>(linkedParameter, paramName, unit, targetPtr, condition);
    }

    template <typename T, typename U>
    void addDependent(T& linkedParameter, String paramName, String dependee, U condition) {
        if (parameterCount == 16) return;

        XRTLpar* dependency = &operator[](dependee);
        if (dependency->isNull()) return;
        XRTLparameter<U>* targetPtr = static_cast<XRTLparameter<U> *>(dependency);

        parameters[parameterCount++] = new XRTLdependentPar<T,U>(linkedParameter, paramName, targetPtr, condition);
    }

    void setParent(ParameterPack* newParent) {
        parent = newParent;
    }

    void print() {
        if (name() != "") {
            Serial.println(centerString("", 39, '-'));
            Serial.println(centerString(name().c_str(), 39, ' '));
            Serial.println(centerString("", 39, '-'));
        }

        for (int i = 0; i < parameterCount; i++) {
            parameters[i]->print();
        }

        Serial.println("");
    }

    XRTLpar& operator[](String queryParam) {
        for (int i = 0; i < parameterCount; i++) {
            if (parameters[i]->is(queryParam)) return *parameters[i];
        }

        return nullParameter;
    }

    void setViaSerial() {
        Serial.println("");
        Serial.println(centerString("", 39, '-'));
        Serial.println(centerString(name(), 39, ' '));
        Serial.println(centerString("", 39, '-'));
        Serial.println("");

        for (int i = 0; i < parameterCount; i++) {
            if (!parameters[i]->notListed) {
                Serial.printf("%d: %s\n", i, parameters[i]->name.c_str());
            }
        }

        Serial.println("");
        Serial.println("a: set all");
        if (linkedName != NULL) Serial.println("c: change controlId");
        Serial.println("r: return");

        Serial.println("");
        String rawInput = serialInput("choice: ");
        uint8_t inputNumber = rawInput.toInt();

        if (rawInput == "c") {
            *linkedName = serialInput("controlId (String) = ");
            return;
        }
        else if (rawInput == "a") {
            if (linkedName != NULL) {
                *linkedName = serialInput("controlId (String) = ");
            }

            for (int i = 0; i < parameterCount; i++) {
                if (!parameters[i]->notListed) parameters[i]->setViaSerial();
            }
        }
        else if (rawInput == "r") {
            return;
        }
        else if (inputNumber < parameterCount && !parameters[inputNumber]->notListed) {
            parameters[inputNumber]->setViaSerial();
        }
    }
};

#endif