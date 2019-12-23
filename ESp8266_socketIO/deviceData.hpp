#ifndef DEVICEDATA_H /* START GUARD */
#define DEVICEDATA_H

#include <Arduino.h>
#include <ArduinoJson.h> // requires ArduinoJson package by Benoit Blanchon
#include <string.h>
#include "typedefs.h" // Has to be loaded before storrage_service.h, it's used there
#include "storage.hpp"

#define MAX_JSON_SIZE 512
#define PROPERTY_COUNT sizeof(properties) / sizeof(properties[0]) // Automatically decide on the count of properties actually available (It's 4, but might change in the future) \
                                                                  // sizeof(properties[0]) should be the count of bytes of the largest member of propertie

extern const char *properties[4];

class DeviceData
{
private:
    DeviceProperties_t data;

public:
    /**
 * Initialize Singleton instance of DeviceData
 */
    DeviceData();

    /**
 * Deserialize JSON String to JSON Document
 */
    int Deserialize(JsonDocument &doc, const char *jsonString);

    /**
 * Parse JSON Document and store to session variables
 */
    int Parse(JsonDocument &doc, CommandOptions command);

    /**
 * Serialize session variables to JSON String
 */
    int Serialize(const JsonDocument &doc, char *jsonString);

    /**
 * Prints all properties to the serial if DEBUG is enabled
 */
    void debugPrint(void);

private:
    /**
 * Handle device operation
 */
    void updateOutput(void);

    /**
 * Set session variables
 */
    bool setName(JsonVariant value);
    bool setType(JsonVariant value);
    bool setOnState(JsonVariant value);
    bool setBrightness(JsonVariant value);

    /**
 * Storing to EEPROM
 */
    void storeName(void);
    void storeType(void);
    void storeOnState(void);
    void storeBrightness(void);

    /**
 * Fetching from EEPROM and overriding session variables
 */
    void resetName(void);
    void resetType(void);
    void resetOnState(void);
    void resetBrightness(void);
};

extern DeviceData Device;

#endif