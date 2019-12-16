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

// Output
#define DEFLED_BUILTIN // Route PWM to LED_BUILTIN instead PWM_PIN
#define PWM_RANGE 100 // range for analogwrite
#define PWM_PIN 13 // Pin to Output, ATTENTION: not used ifdef DEFLED_BUILTIN

// Globals
DataPacket_t states; // Initialize empty states struct
SocketIoClient Socket;

// JSON
//char[], as shown here, enables the "zero-copy" mode. This mode uses
char json[] = "{\"name\":\"Warmweiss LED Innen2\",\"type\":\"Light\",\"on_state\":true,\"brightness\":10}"; // Excemple
char test_get_json[] = "{\"name\",\"type\",\"on_state\",\"brightness\"}"; // Excemple

StaticJsonDocument<1023> doc; // [bytes] Allocate the capacity of the memory pool of th JSON document in the heap
// DynamicJsonDocument doc(1023); // allocates memory on the stack, it can replace by StaticJsonDocument

// Prototypes
// int deserialize(const char * json_obj, DataPacket_t* destination, int option, char* json_obj_out = "{}"); // Protoype to make 

// Set method changes states of this device
void set(const char * payload, size_t lenght) {
  DEBUG_PRINTLN(payload);
  int option = 2; // Choose set option
  DataPacket_t data; // Initialize empty data packe which will be filled
  deserialize(payload, &data, option); // Option 2 for "set"
  // if (success < 0) {
  //     Socket.emit("Error in get. Error parsing content"); // set event changes states on this device
  //   } else {
  //     states = data; // Sore it to global "states"
  //   }
  LAMP_update(); // Update output
}

void get(const char * payload, size_t lenght) {
  DEBUG_PRINTLN(payload);
  // Deserialize the JSON document
  int option = 1; // Choose get option
  // DataPacket_t data = states;
  char* json_obj_out = deserialize(payload, &states, option); // Option 1 for "get"
    if (false) {
      Socket.emit("Error in get. Error parsing content"); // set event changes states on this device
    } else {
      // any of these function causes stack overflow .. TMEP solution: emmit in deserialize function
      // DEBUG_PRINTLN(json_obj_out);
      //Socket.emit("response", json_obj_out); // set event changes states on this device
    }

  return;
}

// ---------------- JSON -----------------------
char* deserialize(const char * json_obj, DataPacket_t* destination, int option){
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json_obj);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return "Error";
  }
  char json_obj_out[1023] ="{}"; 
  switch(option){
    case 1: // Get option was chosen
      // simply fill the doc with requested data
      if (doc.containsKey("name"))  doc["name"].set(destination->name); // Tested
      if (doc.containsKey("type"))  doc["type"].set(destination->type); 
      if (doc.containsKey("on_state"))  doc["on_state"].set(destination->on_state); 
      if (doc.containsKey("brightness"))  doc["brightness"].set(destination->brightness); 
      // json_obj = doc.to<JsonArray>(); // json_obj: {"name"}  -> {"name":"LED"} // does not work
      // serializeJson(doc, json_obj); // Tried directly, cannot work because json_obj is actually payload and thats const 
      DEBUG_PRINTLN("Before:");
      DEBUG_PRINTLN(json_obj);
      serializeJson(doc, json_obj_out);
      DEBUG_PRINTLN("After:");
      Serial.println(json_obj_out);
      Socket.emit("get", json_obj_out); // set event changes states on this device
      break;
    case 2: // Set option was chosen: Stores values of JSON into destination (i.e. states)
      // if (*_id != NULL) *id = *_id;
      if (doc.containsKey("name")) strcpy(destination->name, doc["name"]);
      if (doc.containsKey("type"))  strcpy(destination->type, doc["type"]);
      if (doc.containsKey("on_state"))  destination->on_state = doc["on_state"];
      if (doc.containsKey("brightness"))  destination->brightness = doc["brightness"];
      break;
    case 3: // EEPROM option was chosen: Stores values of JSON into EEPROM
      if (doc.containsKey("name"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["name"]);
      if (doc.containsKey("type"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["type"]);
      if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["on_state"]);
      if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_brightness,doc["brightness"]);
      break;
    return json_obj_out;
  }
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
  // Socket.on("event", event); // "event" from excemple can be deleted
  Socket.on("get", get); // set event emits states of this device
  Socket.on("set", set); // set event changes states on this device
  // Socket.on("setEEPROM", setEEPROM); // set event emits states of this device
  // TODO: catch unknown event


  // TEST:
  // deserialize(json,2);
  get(json,sizeof(json));
}

void loop(void) {
  Socket.loop();
  // need to check wifi status constantly?
}
