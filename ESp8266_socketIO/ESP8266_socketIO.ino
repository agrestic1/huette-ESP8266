// #include <ESP8266WiFi>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <SocketIoClient.h> // requires SocketIoCleint package by Vincent Wyszynski and also WebSockets Package by Markus Sattler
#include "typedefs.h" // Has to be loaded before storrage_service.h, it's used there
#include "privates.h" // conrains privat info like Socket Server Address, WiFi SSID and PW, must be adjusted
#include "deviceData.hpp"

SocketIoClient Socket;

void get(const char * payload, size_t length) {
  handleCommand(payload, length, COMMAND_GET);
}

void set(const char * payload, size_t length) {
  handleCommand(payload, length, COMMAND_SET);
}

void publish(const char* payload, size_t length) {
  // Todo: build JSON string from current setup and emit over socket
  // Socket.emit("publish", "{\"name\": \"LED innen\", \"type\": \"Light\"}");

  // To publish all our properties, we do the same as for a 'get' command for ALL properties:

  size_t size = 3;
  // Prepare our jsonString to hold the first 2 characters including '\0' null terminator
  char *jsonString = (char*)malloc(size);
  strcpy(jsonString, "{\"\0");

  // Now we start adding our properties
  // Lets do this dynamically
  for(int i = 0; i < PROPERTY_COUNT; i++) {
    // We don't have to add +1 for the null terminator as this is taken care of already
    size += strlen(properties[i]);
    // Reallocate memory for 'jsonString' adding space for next property plus 5 additional characters (":0,")
    jsonString = (char*)realloc(jsonString, size + 5);
    // copy next property to end of 'jsonString'
    strcat(jsonString, properties[i]);
    // Add 5 filling characters plus '\0' null terminator
    strcat(jsonString, "\":0,\"\0");
    // Of course we have to add this to our size as well
    size += 5;
  }
  // Once finished we have to override the last two characters
  char* c_ptr = (jsonString + (strlen(jsonString) - 2));
  // Add last 2 characters plus '\0' null terminator
  strcpy(c_ptr, "}\"\0");

  DEBUG_PRINTLN(jsonString);

  // Now call the handler requesting 'publish' response
  handleCommand((const char*)jsonString, size, COMMAND_PUBLISH);
}

void write_eeprom(const char * payload, size_t length) {
  handleCommand(payload, length, COMMAND_WRITE_EEPROM);
}

void disconnect(const char * payload, size_t length) {
  DEBUG_PRINTLN("Connection lost, reconnecting..");
  Socket.begin(HOST, PORT);
}

// handleCommand takes care of set, get, and eeprom_write commands now
void handleCommand(const char * payload, size_t length, CommandOptions option) {
  // Since our received payload may be overridden by a subsequent command, we have to store it in a fresh buffer here!
  // First dynamically allocate 'length' bytes of memory on the stack, we have to free this memory later!
  char* commandBuffer = (char*)malloc(length*sizeof(char));
  // Todo: Maybe we can use variable length to allocate our Document?!
  StaticJsonDocument<MAX_JSON_SIZE> doc; // [bytes] Allocate the capacity of the memory pool of th JSON document in the heap  
  if(commandBuffer == NULL) {
    // Error, we were unable to get the requested amount of memory :-(
    Socket.emit("error", "{Error in set. Memory allocation failed.}"); // Send error
    return;
  }
  // Copy the payload to our buffer (using strncpy ensures we won't have a buffer overflow)
  strncpy(commandBuffer, payload, length);
  // Though we have to ensure that our string is null terminated, so we simply set the last element to '\0'
  // (requirement of 'deserializeJson()')
  commandBuffer[length-1] = '\0';

  // From here on we use our 'commandBuffer' instead of 'payload', since payload is just a pointer
  // that we are not in control of. Additionally - and for this reason - we can disregard the original 'const' qualifier
  DEBUG_PRINTLN(commandBuffer);
  // Attempt to deserialize received JSON string
  int success = Device.Deserialize(doc, (const char*)commandBuffer); // Option 2 for "set"
  if (success < 0) {
    Socket.emit("error", "{Error deserializing JSON string.}"); // Send error
  } else {
    // If deserialization worked, let's head to parsing our JSON document now..
    DEBUG_PRINTLN("Deserialization succeeded");
    // Parse the resulting JSON document
    success = Device.Parse(doc, option);
    if (success < 0) {
      Socket.emit("error", "{Error parsing data.}"); // Send error
    } else {
      // At this point we should have a beautiful JSON Document already containing our response
      DEBUG_PRINTLN("Parsing succeeded");
      // Serialize our response (produces JSON String)
      success = Device.Serialize(doc, commandBuffer);
      if (success < 0) {
        Socket.emit("error", "{Error serializing JSON data.}"); // Send error
      } else {
        DEBUG_PRINTLN("Serialization succeeded");
        switch(option) {
          case COMMAND_GET:
            Socket.emit("get", commandBuffer);
            break;
          case COMMAND_PUBLISH:
            Socket.emit("publish", commandBuffer);
            break;
          case COMMAND_SET:
            Socket.emit("set", commandBuffer);
            break;
          case COMMAND_WRITE_EEPROM:
            Socket.emit("write_eeprom", commandBuffer);
          default:
            Socket.emit("error", "{Error: Unknown event.}");
            break;
        }
      }
    }
  }

  // Free the allocated memory, we don't need it anymore
  free(commandBuffer);
}

// ----------------- DEBUG -------------------
void printState(){
  Device.debugPrint();
}

void setupSerial() {
// Serial
#ifdef DEBUG
  Serial.begin(115200);
    while (!Serial){ // Wait for serial connection
      digitalWrite(PWM_PIN, HIGH); // Fast blink LED
      delay(100);
      digitalWrite(PWM_PIN, LOW);
      delay(100);
    } 
    DEBUG_PRINTLN("Default Values:");
    printState();
#endif
}

// Outsourced setup routines

void setupPeripherals() {
  // GPIO
  pinMode(PWM_PIN, OUTPUT);
  analogWriteRange(PWM_RANGE); // use 8 bit range instead of 10 bit so it's easier to handle
  digitalWrite(PWM_PIN, LOW);
}

void setupWifi() {
  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  DEBUG_PRINTLN("Starting WiFi init");
  while (WiFi.status() != WL_CONNECTED) {  // Wait for connection
    delay(500);
    DEBUG_PRINT("Waiting for connection.. ");
  }
  DEBUG_PRINTLN("Connected to ");
  DEBUG_PRINTLN(ssid);
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}

void setupSocket(){
  // Socket
  Socket.begin(HOST, PORT); // Connected to Device Socket using config from config.h
  // Subscriptions:
  Socket.on("get", get); // get selected properties from this device
  Socket.on("set", set); // set selected properties on this device
  Socket.on("publish", publish); // publish all properties 
  Socket.on("write_eeprom", write_eeprom); // writes selected properties to EEPROM as default settings
  Socket.on("disconnect", disconnect);
}

// --------------- SETUP -----------------
void setup(void) {
  states = new DeviceData(); 

  setupSerial();
  setupPeripherals();
  setupWifi();
  setupSocket();
}

void loop(void) {
  Socket.loop();
  // Todo: need to check wifi status constantly?
}
