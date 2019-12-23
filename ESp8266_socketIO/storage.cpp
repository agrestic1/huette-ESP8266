#include "EEPROM.h"
#include "storage.hpp"

int StorageClass::read(const int EE_ADR, void* value, int length)
{
    EEPROM.begin(USED_EEPROM_SIZE);
    int i;
    uint8_t *val_ptr = (uint8_t*)value;
    for(i = 0; i < length; i++) {
        *(val_ptr++) = EEPROM.read(EE_ADR + i);
    }
    EEPROM.end();

    return i;
}

int StorageClass::write(const int EE_ADR, const void* value, int length)
{
    EEPROM.begin(USED_EEPROM_SIZE);
    int i;
    uint8_t *val_ptr = (uint8_t*)value;
    for(i = 0; i < length; i++) {
        EEPROM.write(EE_ADR + i, *(val_ptr++));
    }
    EEPROM.end();

    return i;
}

StorageClass Storage;