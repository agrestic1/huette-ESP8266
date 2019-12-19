#ifndef STORAGE_H /* START GUARD */
#define STORAGE_H

#include <Arduino.h> // for type definitions
#include "typedefs.h"

#define DEBUG_memory // DEBUG Option prints all that happens to serial Monitor
#ifdef DEBUG_memory
#define DEBUG_memory_PRINT(x) Serial.print(x)
#define DEBUG_memory_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_memory_PRINT(x)
#define DEBUG_memory_PRINTLN(x)
#endif

#define member_size(type, member) sizeof(((type *)0)->member)

#define STORAGE_NAME_ADDRESS        0
#define STORAGE_TYPE_ADDRESS        STORAGE_NAME_ADDRESS + member_size(DeviceProperties_t, name)
#define STORAGE_ON_STATE_ADDRESS    STORAGE_TYPE_ADDRESS + member_size(DeviceProperties_t, type)
#define STORAGE_BRIGHTNESS_ADDRESS  STORAGE_ON_STATE_ADDRESS + member_size(DeviceProperties_t, on_state)

#define USED_EEPROM_SIZE            STORAGE_BRIGHTNESS_ADDRESS + member_size(DeviceProperties_t, brightness)

class StorageClass
{
public:
    /**
     * Reads 'length' bytes starting from 'EE_ADR' and stores the result to the given
     * location.
     */
    int read(const int EE_ADR, void *value, int length);
    int write(const int EE_ADR, const void* value, int length);
};

extern StorageClass Storage;

#endif /* END GUARD */