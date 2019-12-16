// Typedefs
typedef struct {
  char name[32];
  char type[32];
  bool on_state;
  uint8_t brightness; // Int has 4 byte uin8_t only 1
} DataPacket_t;