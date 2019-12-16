#include "typedefs.h" // Has to be loaded before storrage_service.h, it's used there
#include "config.h" // Configuration file, must be adjusted
#include "privates.h" // conrains privat info like WiFi SSID and PW, must be adjusted
#include "storrage_service.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <SocketIoClient.h> // requires SocketIoCleint package by Vincent Wyszynski and also WebSockets Package by Markus Sattler
//#include <EEPROM.h> // necessary to store and read values from Flash (virtual EEPORM)
#include <ArduinoJson.h> // requires ArduinoJson package by Benoit Blanchon
#include <string.h>
// content of privates.h:
// const char* ssid = "yourSSID";
// const char* password = "yourPW";


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

// #define SETUP // Writes content of json[] into EEPROM (Flashing does not wipe EEPROM)

// Globals
DataPacket_t states; // Initialize empty states struct
SocketIoClient Socket;

// JSON
//char[], as shown here, enables the "zero-copy" mode. This mode uses
char json[] = "{\"name\":\"Warmweiss LED Innen2\",\"type\":\"Light\",\"on_state\":true,\"brightness\":10}"; // Excemple
char test_get_json[] = "{\"name\",\"type\",\"on_state\",\"brightness\"}"; // Excemple

StaticJsonDocument<255> doc; // [bytes] Allocate the capacity of the memory pool of th JSON document in the heap
// DynamicJsonDocument doc(1023); // allocates memory on the stack, it can replace by StaticJsonDocument

// Prototypes
// int deserialize(const char * json_obj, DataPacket_t* destination, int option, char* json_obj_out = "{}"); // Protoype to make 

// Set method changes states of this device
void set(const char * payload, size_t lenght) {
  // Since our received payload may be overridden by a subsequent command, we have to store it in a fresh buffer here!
  // First dynamically allocate 'length' bytes of memory on the stack, we have to free this memory later!
  char* commandBuffer = (char*)malloc(length*sizeof(char));
  if(commandBuffer == NULL) {
    // Error, we were unable to get the requested amount of memory :-(
    Socket.emit("Error in set. Memory allocation failed."); // Send error
    return;
  }
  // Copy the payload to our buffer (using strncpy ensures we won't have a buffer overflow)
  strncpy(commandBuffer, payload, lenght);
  // Though we have to ensure that our string is null terminated, so we simply set the last element to '\0'
  // (requirement of 'deserializeJson()')
  commandBuffer[lenght-1] = '\0';

  // From here on we use our 'commandBuffer' instead of 'payload', since payload is just a pointer
  // that we are not in control of. Additionally - and for this reason - we can disregard the original 'const' qualifier
  DEBUG_PRINTLN(commandBuffer);
  // No need for variable here, we just want to have something human readable.. using enum
  // uint8_t option = 2; // Choose set option
  // DataPacket_t data; // Initialize empty data packet which will be filled
  // We intend to deserialize our JSON String to our global 'states' struct.. no need for local data packet
  int success = deserialize(commandBuffer/*, &states*/, COMMAND_SET, nullptr); // Option 2 for "set"

  if (success < 0) {
    Socket.emit("Error in set. Error parsing content"); // Send error
  } else {
    // Todo: we need a serialize function that is able to generate a 'emittable' JSON String from our struct 'data'
    // The following will probably not work, would have to move every single property...
    // states = data; // Sore it to global "states"
  }

  // Free the allocated memory, we don't need it anymore (have our 'data' struct now)
  free(commandBuffer);

  LAMP_update(); // Update output
}

void get(const char * payload, size_t lenght) {
  // Since our received payload may be overridden by a subsequent command, we have to store it in a fresh buffer here!
  // First dynamically allocate 'length' bytes of memory on the stack, we have to free this memory later!
  char* commandBuffer = (char*)malloc(length*sizeof(char));
  if(commandBuffer == NULL) {
    // Error, we were unable to get the requested amount of memory :-(
    Socket.emit("Error in set. Memory allocation failed."); // Send error
    return;
  }
  // Copy the payload to our buffer (using strncpy ensures we won't have a buffer overflow)
  strncpy(commandBuffer, payload, lenght);
  // Though we have to ensure that our string is null terminated, so we simply set the last element to '\0'
  // (requirement of 'deserializeJson()')
  commandBuffer[lenght-1] = '\0';

  // From here on we use our 'commandBuffer' instead of 'payload', since payload is just a pointer
  // that we are not in control of. Additionally - and for this reason - we can disregard the original 'const' qualifier
  DEBUG_PRINTLN(commandBuffer);
  // No need for variable here, we just want to have something human readable.. using enum
  // uint8_t option = 1; // Choose get option
  char json_obj_out[255] = "{}"; // Empty json string, to be filled by deserialize function, will be emitted later
  // DataPacket_t data; // Initialize empty data packet which will be filled
  int success = deserialize(payload/*, &data*/, COMMAND_GET, json_obj_out); // Option 1 for "get"

  if (success < 0) {
    Socket.emit("Error in get. Error parsing content"); // Send error
  } else {
    // Todo: we need a serialize function that is able to generate a 'emittable' JSON String from our struct 'data'
    Socket.emit("get", json_obj_out); // Send the state out
  }

  // Free the allocated memory, we don't need it anymore (have our 'data' struct now)
  free(commandBuffer);
}

// ---------------- JSON -----------------------
int deserialize(const char * json_obj/*, DataPacket_t* destination*/, uint8_t option, char* json_obj_out){
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json_obj);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());

    // Todo: Send error message over to backend using Socket.emit
    return -1;
  }
  
  // Removed indirect access to, anyways, global 'states' DataPacket.. no need for that
   switch(option){
    case COMMAND_GET: // Get option was chosen
      // simply fill the existing doc with requested data
      if (doc.containsKey("name"))  doc["name"].set(states->name); // Tested
      if (doc.containsKey("type"))  doc["type"].set(states->type); 
      if (doc.containsKey("on_state"))  doc["on_state"].set(states->on_state); 
      if (doc.containsKey("brightness"))  doc["brightness"].set(states->brightness); 
      // serializeJson(doc, json_obj_out); // Tried directly, but does not work
      char json_obj_tmp[255]; // shitty, i want to put the infos directly into json_obj_out
      serializeJson(doc, json_obj_tmp);
      strcpy(json_obj_out, json_obj_tmp);
      break;
    case COMMAND_SET: // Set option was chosen: Stores values of JSON into destination (i.e. states)
      // if (*_id != NULL) *id = *_id;
      if (doc.containsKey("name")) strcpy(states->name, doc["name"]);
      if (doc.containsKey("type"))  strcpy(states->type, doc["type"]);
      if (doc.containsKey("on_state"))  states->on_state = doc["on_state"];
      if (doc.containsKey("brightness"))  states->brightness = doc["brightness"];
      break;
    case COMMAND_WRITE_EEPROM: // EEPROM option was chosen: Stores values of JSON into EEPROM
      if (doc.containsKey("name"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["name"]);
      if (doc.containsKey("type"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["type"]);
      if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["on_state"]);
      if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_brightness,doc["brightness"]);
      break;
    default:
      // This should never happen!
      // Means we received an invalid/unknown command
      break;
  }
  return 0;
}

void LAMP_update(){
    #ifdef DEFLED_BUILTIN
      if (states.on_state){
        analogWrite(LED_BUILTIN,PWM_RANGE-states.brightness); 
        } else {
          analogWrite(LED_BUILTIN,PWM_RANGE); 
        }
    #else
      if (states.on_state){
        analogWrite(PWM_PIN,states.brightness); 
      } else {
        analogWrite(PWM_PIN,0); 
      }
    #endif
  DEBUG_PRINT("Brightness changed to ");
  DEBUG_PRINTLN(states.brightness);
}

// ----------------- DEBUG -------------------
void printState(){
  DEBUG_PRINTLN(states.name);
  DEBUG_PRINTLN(states.type);
  DEBUG_PRINTLN(states.on_state?"True":"False");
  DEBUG_PRINTLN(states.brightness);
}

// --------------- SETUP -----------------
void setup(void) {
  // GPIO
  pinMode(PWM_PIN, OUTPUT);
  analogWriteRange(PWM_RANGE); // use 8 bit range instead of 10 bit so it's easier to handle
  digitalWrite(PWM_PIN, LOW);
    // EEPROM
  EEPROM.begin(EEPROM_size); // [byte] Set size of EEPROM up to 512
  #ifdef SETUP
    set(json,sizeof(json));
    EEPROM_write_all(&states);
  #endif
  EEPROM_read_all(&states); // update status from EEPROM
  LAMP_update(); // Set lamp to status read from EEPROM
  // Serial
  Serial.begin(115200);
  #ifdef DEBUG
    while (!Serial){ // Wait for serial connection
      digitalWrite(PWM_PIN, HIGH); // Fast blink LED
      delay(100);
      digitalWrite(PWM_PIN, LOW);
      delay(100);
    } 
    DEBUG_PRINTLN("Default Values:");
    printState();
  #endif

  // TEST:
  // deserialize(json,2);
  get(json,sizeof(json));

    // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DEBUG_PRINTLN("");
  while (WiFi.status() != WL_CONNECTED) {  // Wait for connection
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(ssid);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());

  // Socket
  Socket.begin(HOST, PORT); // Connected to Device Socket using config from config.h
  // Subscriptions:
  //Socket.on("connect", sendType); // Connected to server
  Socket.on("get", get); // get event emits states of this device
  Socket.on("set", set); // set event changes states on this device
  // Socket.on("setEEPROM", setEEPROM); // writes new default states into EEPROM
  // TODO: catch unknown event
}

void loop(void) {
  Socket.loop();
  // need to check wifi status constantly?
}
