#ifndef TYPEDEFS_H /* START GUARD */
#define TYPEDEFS_H

#include <Arduino.h> // for type definitions

/**
 * If DEBUG is defined, strings may be printed through the serial interface
 * using macros 'DEBUG_PRINT' or 'DEBUG_PRINTLN'
 */
#ifndef DEBUG
  #define DEBUG // DEBUG Option prints all that happens to serial Monitor
    #ifdef DEBUG
      #define DEBUG_PRINT(x)     Serial.print (x)
      #define DEBUG_PRINTLN(x)  Serial.println (x)
    #else
      #define DEBUG_PRINT(x)
      #define DEBUG_PRINTLN(x)
    #endif 
#endif

/**
 * Settings for Device
 */
#define DEFAULT_DEVICE_TYPE "Light"
#define DEFAULT_DEVICE_NAME "Dimmable LED"
#define DEFAULT_DEVICE_ON_STATE false
#define DEFAULT_DEVICE_BRIGHTNESS 50

/**
 * Settings for PWM Output
 */
#define PWM_PIN // Route PWM to LED_BUILTIN instead PWM_PIN
#define PWM_RANGE 100  // range for analogwrite
#define PWM_PIN 14     // Pin to Output, ATTENTION: not used ifdef DEFLED_BUILTIN

/**
 * Maximum size of any string within device properties
 */
#define MAX_STRING_SIZE 32


/**
 * CommandOptions Enum:
 * Defines some numeric values used to identify the type of command received.
 * 
 */
enum CommandOptions
{
  // Received invalid command
  COMMAND_INVALID = 0,
  // Received 'get' command
  COMMAND_GET = 1,
  // Received 'publish' command
  COMMAND_PUBLISH = 2,
  // Received 'set' command
  COMMAND_SET = 3,
  // Received 'setEeprom' command
  COMMAND_WRITE_EEPROM = 4
};

/**
 * DeviceProperties Struct:
 * These are the properties of our Device
 */
typedef struct
{
  char name[MAX_STRING_SIZE];
  char type[MAX_STRING_SIZE];
  bool on_state;
  int brightness;
} DeviceProperties_t;

#endif