// Enums
/*
 * CommandOptions Enum:
 * Defines some numeric values used to identify the type of command received.
 * 
 */
enum CommandOptions {
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

// Typedefs
typedef struct {
  char *name;
  char *type;
  bool on_state;
  int brightness;
  // uint8_t brightness; // Int has 4 byte uin8_t only 1
} DataPacket_t;