#ifndef PARAMETERPACK_H
#define PARAMETERPACK_H

#include "common/XRTLfunctions.h"

class XRTLpar
{
protected:
    String name;
    bool notListed = false;

public:
    virtual void load(JsonObject &settings);
    virtual void save(JsonObject &settings);
    // virtual void setViaSerial(JsonObject& settings);
    virtual void print();

    // base class only used for default (null) parameter, specialized classes return false
    virtual bool isNull();

    virtual void setViaSerial();

    bool is(String &queryName);

    friend class ParameterPack;
};

/**
 * @brief combine a parameter with a string that is used to identify, save and load the parameter
 * @note no copy of the parameter is created -- operations on the returned reference will alter the parameter
 */
template <typename T>
class XRTLparameter : public XRTLpar
{
protected:
    T *parameter;
    String unit;

public:
    XRTLparameter() {}

    XRTLparameter(T &linkedParameter, String &paramName, String &unitStr)
    {
        parameter = &linkedParameter;
        name = paramName;
        unit = unitStr;
    }

    XRTLparameter(T &linkedParameter, String &paramName)
    {
        parameter = &linkedParameter;
        name = paramName;
        notListed = true;
    }

    /**
     * @brief creates a key value pair with the name and value of the parameter in the supplied JsonObject
     * @param settings JsonObject that will receive the key value pair
     */
    virtual void save(JsonObject &settings)
    {
        settings[name] = *parameter;
    }

    /**
     * @brief check if the parameter is contained within a JsonObject and whether it has the correct type, asign read value if so
     * @param settings JsonObject to be checked for parameter
     * @note if the parameter name is not contained within the Object or the type is incorrect, the parameter will remain unchanged
     */
    virtual void load(JsonObject &settings)
    {
        auto candidate = settings[name];
        if (candidate.isNull())
            return;
        if (!candidate.is<T>())
            return;

        *parameter = candidate.as<T>();
    }

    /**
     * @brief print the parameter name and value on the Serial interface
     * @note will not print anything if the parameter is not listed
     */
    virtual void print()
    {
        if (notListed)
            return;
        Serial.print(name.c_str());
        Serial.print(" (");
        Serial.print(unit.c_str());
        Serial.print("): ");
        Serial.println(*parameter);
    }

    /**
     * @brief set a new value for the parameter over the Serial interface
     * @note does nothing if the parameter is not listed 
     */
    virtual void setViaSerial()
    {
        if (notListed)
            return;

        setViaSerialQuery();
    }

    /**
     * @brief actual query dialog for the Serial interface 
     * @note query contains <name> (<unit>)
     */
    void setViaSerialQuery()
    {
        String inputQuery = name;
        inputQuery += " (";
        inputQuery += unit;
        inputQuery += ") = ";

        StaticJsonDocument<64> doc;
        JsonArray input = doc.to<JsonArray>();

        input.add(serialInput(inputQuery));

        if (input[0].isNull())
            return;

        *parameter = input[0].as<T>();
    }

    /**
     * @brief check if the parameter is initialized, i.e. a specialized class used
     * @return true if base calss is used, false if initialized 
     */
    bool isNull()
    {
        return false;
    }

    /**
     * @brief check if the parameter is equal to a supplied value
     * @param testParam test for equality to this value
     * @return true if equal, false if not
     */
    bool isEqual(T &testParam)
    {
        return (*parameter == testParam);
    }

    /**
     * @brief get the linked parameter
     * @return pointer to the linked parameter
     * @note this is not a copy, operations on the target of this pointer will alter the parameter
     */
    T &get()
    {
        return *parameter;
    }
};

template <>
class XRTLparameter<bool> : public XRTLpar
{
protected:
    bool *parameter;
    String unit = "y/n";

public:
    XRTLparameter() {}

    XRTLparameter(bool &linkedParameter, String &paramName, String &unitStr)
    {
        parameter = &linkedParameter;
        name = paramName;
    }

    XRTLparameter(bool &linkedParameter, String &paramName)
    {
        parameter = &linkedParameter;
        name = paramName;
        notListed = true;
    }

    virtual void save(JsonObject &settings)
    {
        settings[name] = *parameter;
    }

    virtual void load(JsonObject &settings)
    {
        auto candidate = settings[name];
        if (candidate.isNull())
            return;
        if (!candidate.is<bool>())
            return;

        *parameter = candidate.as<bool>();
    }

    virtual void setViaSerial()
    {
        if (notListed)
            return;

        setViaSerialQuery();
    }

    void setViaSerialQuery()
    {
        String inputQuery = name;
        inputQuery += " (";
        inputQuery += unit;
        inputQuery += ") = ";

        String input = serialInput(inputQuery);

        if (input == "true" || input == "y" || input == "1" || input == "TRUE" || input == "Y")
        {
            *parameter = true;
        }
        else if (input == "false" || input == "n" || input == "0" || input == "FALSE" || input == "N")
        {
            *parameter = false;
        }
    }

    virtual void print()
    {
        Serial.printf("%s (%s): %s\n", name.c_str(), unit.c_str(), *parameter ? "true" : "false");
    }

    bool isNull()
    {
        return false;
    }

    bool isEqual(bool &testParam)
    {
        return (*parameter == testParam);
    }

    bool &get()
    {
        return *parameter;
    }
};

template <typename T, typename U>
class XRTLdependentPar : public XRTLparameter<T>
{
private:
    XRTLparameter<U> *dependency;
    U condition;

public:
    XRTLdependentPar(T &linkedParameter, String &paramName, String &unitStr, XRTLparameter<U> *dependencyPar, U &dependencyCond)
    {
        this->parameter = &linkedParameter;
        this->name = paramName;
        this->unit = unitStr;
        dependency = dependencyPar;
        condition = dependencyCond;
    }

    XRTLdependentPar(T &linkedParameter, String &paramName, XRTLparameter<U> *dependencyPar, U &dependencyCond)
    {
        this->parameter = &linkedParameter;
        this->name = paramName;
        this->notListed = true;
        dependency = dependencyPar;
        condition = dependencyCond;
    }

    virtual void save(JsonObject &settings)
    {
        if (!dependency->isEqual(condition))
            return;
        settings[this->name] = *this->parameter;
    }

    virtual void load(JsonObject &settings)
    {
        if (!dependency->isEqual(condition))
            return;
        auto candidate = settings[this->name];
        if (candidate.isNull())
            return;
        if (!candidate.template is<T>())
            return;

        *this->parameter = candidate.template as<T>();
    }

    virtual void print()
    {
        if (!dependency->isEqual(condition))
            return;
        if (this->notListed)
            return;
        Serial.print(this->name.c_str());
        Serial.print(" (");
        Serial.print(this->unit.c_str());
        Serial.print("): ");
        Serial.println(*this->parameter);
    }

    void setViaSerial()
    {
        if (!dependency->isEqual(condition))
            return;
        if (this->notListed)
            return;

        this->setViaSerialQuery();
    }
};

/**
 * @brief Manage multiple parameters simultaneously. Provides methodes for saving, loading and setting parameters.
 */
class ParameterPack
{
protected:
    uint8_t parameterCount = 0;
    XRTLpar *parameters[16];
    ParameterPack *parent = NULL;
    String *linkedName = NULL;
    String reserveName;

    static XRTLpar nullParameter;

public:
    ParameterPack() {}
    ~ParameterPack()
    {
        for (int i = 0; i < parameterCount; i++)
        {
            delete parameters[i];
        }
    }

    /**
     * @brief set the key of the parameter pack as string reference, used for storing and loading the parameters
     * @param key string to be used
     * @note key must not be destroyed before the ParameterPack, no copy is created!
     */
    void setKey(String &key)
    {
        linkedName = &key;
    }

    /**
     * @brief set the key of the parameter pack
     * @param key string to be used
     */
    void setKey(const char *key)
    {
        reserveName = key;
    }

    /**
     * @brief get the key of the parameter pack
     * @return pointer to the name
     */
    String &name()
    {
        if (linkedName)
            return *linkedName;
        else
            return reserveName;
    }

    /**
     * @brief save all parameters to the JsonObject provided
     * @param settings store parameters in here
     * @note if a key for the parameter object exists, a nested JsonObject with that name will be created and used
    */
    void save(JsonObject &settings)
    {
        JsonObject subSettings;
        save(settings, subSettings);
    }

    /**
     * @brief save all parameters to the JsonObject provided
     * @param settings store parameters in here
     * @param subSettings the address pointing to the used (possibly nested) JsonObject will be stored here
     * @note if a key for the parameter object exists, a nested JsonObject with that name will be created and used
    */
    void save(JsonObject &settings, JsonObject &subSettings)
    {
        if (name() != "")
        {
            subSettings = settings.createNestedObject(name());
        }
        else
        {
            subSettings = settings;
        }

        for (int i = 0; i < parameterCount; i++)
        {
            parameters[i]->save(subSettings);
        }
    }

    /**
     * @brief load all parameters from a JsonObject
     * @param settings JsonObject that the settings will be loaded from
     * @note if further access to the JsonObject the parameters were loaded form is needed, provide a second parameter to store the reference 
     */
    void load(JsonObject &settings)
    {
        JsonObject subSettings;
        load(settings, subSettings);
    }

    /**
     * @brief load all the parameters from a JsonObject
     * @param settings JsonObject that the settings will be loaded from
     * @param subSettings JsonObject that will receive a pointer to the settings used 
     */
    void load(JsonObject &settings, JsonObject &subSettings)
    {
        if (name() != "")
        {
            auto candidate = settings[name()];

            if (candidate.isNull() || !candidate.is<JsonObject>())
            {
                subSettings = settings;
                return;
            }

            subSettings = candidate.as<JsonObject>();
        }
        else
        {
            subSettings = settings;
        }

        for (int i = 0; i < parameterCount; i++)
        {
            parameters[i]->load(subSettings);
        }
    }

    // @brief add a parameter that automatically gets loaded from and saved to flash; setable via serial interface
    // @param linkedParameter parameter to be managed, must exist in all contexts of the parameter pack and may be changed by it
    // @param paramName identifier string for the parameter
    // @param unit string representing the physical meaning of the parameter (visible in query procedure)
    template <typename T>
    void add(T &linkedParameter, String paramName, String unit)
    {
        if (parameterCount >= 16)
            return;

        parameters[parameterCount++] = new XRTLparameter<T>(linkedParameter, paramName, unit);
    }

    // @brief add a parameter that automatically gets loaded from and saved to flash
    // @param linkedParameter parameter to be managed, must exist in all contexts of the parameter pack and may be changed by it
    // @param paramName identifier string for the parameter
    // @param unit string representing the physical meaning of the parameter (visible in query procedure)
    // @note parameter does not get listed and is inaccessible via serial interface, good for internal parameters
    template <typename T>
    void add(T &linkedParameter, String paramName)
    {
        if (parameterCount == 16)
            return;

        parameters[parameterCount++] = new XRTLparameter<T>(linkedParameter, paramName);
    }

    /**
     * @brief add a parameter that only gets handled if another parameter meets certain conditions; setable via serial interface
     * @param linkedParameter parameter to be managed, must exist in all contexts of the parameter pack and may be changed by it
     * @param paramName identifier string for the parameter
     * @param unit string representing the physical meaning of the parameter (visible in query procedure)
     * @param dependee identifier of the parameter that this parameter depends on; must exist within this parameter pack
     * @param condition if the dependee parameter is not equal to this value, handle the linked parameter as not listed
     */
    template <typename T, typename U>
    void addDependent(T &linkedParameter, String paramName, String unit, String dependee, U condition)
    {
        if (parameterCount == 16)
            return;

        XRTLpar *dependency = &operator[](dependee);
        if (dependency->isNull())
            return;                                                                // check if parameter exists
        XRTLparameter<U> *targetPtr = static_cast<XRTLparameter<U> *>(dependency); // convert pointer

        parameters[parameterCount++] = new XRTLdependentPar<T, U>(linkedParameter, paramName, unit, targetPtr, condition);
    }

    /**
     * @brief add a parameter that only gets handled if another parameter meets certain conditions
     * @param linkedParameter parameter to be managed, must exist in all contexts of the parameter pack and may be changed by it
     * @param paramName identifier string for the parameter
     * @param dependee identifier of the parameter that this parameter depends on; must exist within this parameter pack
     * @param condition if the dependee parameter is not equal to this value, handle the linked parameter as not listed
     * @note this methode is probably not needed:\n any unlisted parameter does not need to be checked for dependencies
     */
    template <typename T, typename U>
    void addDependent(T &linkedParameter, String paramName, String dependee, U condition)
    {
        if (parameterCount == 16)
            return;

        XRTLpar *dependency = &operator[](dependee);
        if (dependency->isNull())
            return;
        XRTLparameter<U> *targetPtr = static_cast<XRTLparameter<U> *>(dependency);

        parameters[parameterCount++] = new XRTLdependentPar<T, U>(linkedParameter, paramName, targetPtr, condition);
    }

    // TODO: is this needed?
    void setParent(ParameterPack *newParent)
    {
        parent = newParent;
    }

    /**
     * @brief print all listed parameters on the serial interface 
     */
    void print()
    {
        if (name() != "")
        {
            Serial.println(centerString("", 39, '-'));
            Serial.println(centerString(name().c_str(), 39, ' '));
            Serial.println(centerString("", 39, '-'));
        }

        for (int i = 0; i < parameterCount; i++)
        {
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
    XRTLpar &operator[](String queryParam)
    {
        for (int i = 0; i < parameterCount; i++)
        {
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
    bool dialog()
    {
        Serial.println("");
        Serial.println(centerString("", 39, '-'));
        Serial.println(centerString(name(), 39, ' '));
        Serial.println(centerString("", 39, '-'));
        Serial.println("");

        for (int i = 0; i < parameterCount; i++)
        {
            if (!parameters[i]->notListed)
            {
                Serial.printf("%d: %s\n", i, parameters[i]->name.c_str());
            }
        }

        Serial.println("");
        Serial.println("a: set all");
        if (linkedName)
            Serial.println("c: change controlId");
        Serial.println("r: return");

        Serial.println("");
        String rawInput = serialInput("choice: ");
        uint8_t inputNumber = rawInput.toInt();

        if (rawInput == "r")
        {
            return false;
        }
        else if (rawInput == "c")
        {
            *linkedName = serialInput("controlId (String) = ");
        }
        else if (rawInput == "a")
        {
            if (linkedName != NULL)
            {
                *linkedName = serialInput("controlId (String) = ");
            }

            for (int i = 0; i < parameterCount; i++)
            {
                if (!parameters[i]->notListed)
                    parameters[i]->setViaSerial();
            }
        }
        else if (inputNumber < parameterCount && !parameters[inputNumber]->notListed)
        {
            parameters[inputNumber]->setViaSerial();
        }

        return true;
    }

    void setViaSerial()
    {
        while (dialog())
        {
        }
    }
};

#endif