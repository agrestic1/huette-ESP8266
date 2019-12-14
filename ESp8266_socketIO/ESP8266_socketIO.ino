#include "typedefs.h"
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

SocketIoClient Socket;
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
#define DEFLED_BUILTIN // Route PWM to LED_BUILTIN instead PWM_PIN
// #define SETUP // Writes content of json[] into EEPROM (Flashing does not wipe EEPROM)


#define PWM_RANGE 100 // range for analogwrite
#define PWM_PIN 13 // Pin to Output, 13 is LED_BUILTIN

// Globals
DataPacket_t states; // Initialize empty states struct

// JSON
//char[], as shown here, enables the "zero-copy" mode. This mode uses
char json[] = "{\"name\":\"Warmweiss LED Innen\",\"type\":\"Light\",\"on_state\":true,\"brightness\":10}"; // Excemple

StaticJsonDocument<1024> doc; // [bytes] Allocate the capacity of the memory pool of th JSON document in the heap
// DynamicJsonDocument doc(1024); // allocates memory on the stack, it can replace by StaticJsonDocument

// Set method changes states of this device
void set(const char * payload, size_t lenght) {
  DEBUG_PRINTLN(payload);
  int option = 1; // Choose set option
  DataPacket_t data;
  deserialize(payload, &data, option); // Option 1 for "set"

  states = data; // Sore it to global "states"
  LAMP_update(); // Update output
}

void get(const char * payload, size_t lenght) {
  return;
}

// ---------------- JSON -----------------------
void deserialize(const char * json_obj, DataPacket_t* destination, int option){
  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, json_obj);
  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    return;
  }
  switch(option){
    case 1: // Set option was chosen
      // if (*_id != NULL) *id = *_id;
      if (doc.containsKey("name")) strcpy(destination->name, doc["name"]);
      if (doc.containsKey("type"))  strcpy(destination->type, doc["type"]);
      if (doc.containsKey("on_state"))  destination->on_state = doc["on_state"];
      if (doc.containsKey("brightness"))  destination->brightness = doc["brightness"];
      break;
    case 2: // EEPROM option was chosen
      if (doc.containsKey("name"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["name"]);
      if (doc.containsKey("type"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["type"]);
      if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["on_state"]);
      if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_brightness,doc["brightness"]);
      break;
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
  // Serial
  Serial.begin(115200);
  #ifdef DEBUG
    while (!Serial){ // Wait for serial connection
      digitalWrite(PWM_PIN, HIGH); // Fast blink LED
      delay(100);
      digitalWrite(PWM_PIN, LOW);
      delay(100);
    } 
  #endif
  // EEPROM
  EEPROM.begin(EEPROM_size); // [byte] Set size of EEPROM up to 512
  #ifdef SETUP
    set(json,sizeof(json));
    EEPROM_write_all(&states);
  #endif
  EEPROM_read_all(&states); // update status from EEPROM
  LAMP_update(); // Set lamp to status read from EEPROM
  printState();
  // TEST:
  // deserialize(json,1);
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
  Socket.on("set", set); // set event changes states on this device
  Socket.on("get", get); // set event emits states of this device
  // TODO: catch unknown event
}

void loop(void) {
  Socket.loop();
  // need to check wifi status constantly?
}
