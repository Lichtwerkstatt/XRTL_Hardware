#include "XRTLparameterPack.h"

XRTLpar ParameterPack::nullParameter;

void XRTLpar::load(JsonObject &settings){};
void XRTLpar::save(JsonObject &settings){};
void XRTLpar::print(){};

/**
 * @brief check if parameter is empty
 * @returns true if parameter is uninitialized
 * @note base class only used for default parameter, specialized (= initialized) classes return false
 */
bool XRTLpar::isNull()
{
    return true;
}

void XRTLpar::setViaSerial() {}

/**
 * @brief check if parameter name matches query
 * @param queryName string to be checked for match
 * @returns true if queryName matches internally used name 
 */
bool XRTLpar::is(String &queryName)
{
    return (queryName == name);
}