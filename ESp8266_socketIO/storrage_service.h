#include <EEPROM.h>
#include <Arduino.h>  // for type definitions

#define DEBUG // DEBUG Option prints all that happens to serial Monitor
#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
#endif 

#define EEPROM_size 66 // [byte] Set size of EEPROM (up to 512)
#define EEPROM_adr_on_state 0 // 0x01 throws error EEPROM Adress of state
#define EEPROM_adr_brightness 1 // 0x01 throws error EEPROM Adress of brightness
#define EEPROM_adr_name 2 // 0x01 throws error EEPROM Adress of name
#define EEPROM_adr_type 34 // 0x01 throws error EEPROM Adress of type

// ------------------------------- EEPROM -----------------------------
template <class T> int EEPROM_writeAnything(int ee, const T& value)
{
    const byte* p = (const byte*)(const void*)&value;
    unsigned int i;
    for (i = 0; i < sizeof(value); i++)
          EEPROM.write(ee++, *p++);
    if (EEPROM.commit()) {
    DEBUG_PRINTLN("EEPROM successfully committed");
  } else {
    DEBUG_PRINTLN("ERROR! EEPROM commit failed");
  }
    return i;
}

template <class T> int EEPROM_readAnything(int ee, T& value)
{
    byte* p = (byte*)(void*)&value;
    unsigned int i;
    DEBUG_PRINT("Reading EEPROM from Adress: ");
    DEBUG_PRINT(ee);
    DEBUG_PRINT(" to ");
    DEBUG_PRINTLN(ee+sizeof(value)-1);
    for (i = 0; i < sizeof(value); i++)
          *p++ = EEPROM.read(ee++);
    return i;
}

void EEPROM_wipe(){
  EEPROM.end();
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.end();
  EEPROM.begin(EEPROM_size);
}

void EEPROM_write_all(DataPacket_t* states){
  EEPROM_writeAnything(EEPROM_adr_name,states->name);
  EEPROM_writeAnything(EEPROM_adr_type,states->type);
  EEPROM_writeAnything(EEPROM_adr_on_state,states->on_state);
  EEPROM_writeAnything(EEPROM_adr_brightness,states->brightness);
}

void EEPROM_read_all(DataPacket_t* states){
  DEBUG_PRINT("\nName: ");
  EEPROM_readAnything(EEPROM_adr_name,states->name);
  DEBUG_PRINT("Type: ");
  EEPROM_readAnything(EEPROM_adr_type,states->type);
  DEBUG_PRINT("on_state: ");
  EEPROM_readAnything(EEPROM_adr_on_state,states->on_state);
  DEBUG_PRINT("brightness: ");
  EEPROM_readAnything(EEPROM_adr_brightness,states->brightness);
}
