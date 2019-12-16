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

#define MAX_JSON_SIZE 1024

// #define SETUP // Writes content of json[] into EEPROM (Flashing does not wipe EEPROM)

// Globals
// DataPacket_t states; // Initialize empty states struct

// Our new Device class
class DeviceData {
  private:
    char* properties = ["name", "type", "on_state", "brightness"];
    DataPacket_t data;

    updateOutput(void) {
#ifdef DEFLED_BUILTIN
      if (this->data.on_state){
        analogWrite(LED_BUILTIN,PWM_RANGE-this->data.brightness); 
      } else {
        analogWrite(LED_BUILTIN,PWM_RANGE); 
      }
#else
      if (states.on_state){
        analogWrite(PWM_PIN,this->data.brightness); 
      } else {
        analogWrite(PWM_PIN,0); 
      }
#endif
      DEBUG_PRINT("Brightness changed to ");
      DEBUG_PRINTLN(states.brightness);
    };

    setName(JsonVariant value) {
      // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
      if(!value.isNull()) {
        const char* name = value.as<const char*>();
        // The C library function size_t strlen(const char *str) computes the length of the string str up to, but not including the terminating null character.
        // But we also want null termination so: + 1
        size_t length = strlen(name) + 1;
        // Reallocate memory for private member 'name'
        this->data.name = (char*)realloc(this->data.name, length * sizeof(char));
        // Copy received 'name' to private member 'name'
        strcpy(this->data.name, name);
      }
    };

    setType(JsonVariant value) {
      // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
      if(!value.isNull()) {
        const char* type = value.as<const char*>();
        // The C library function size_t strlen(const char *str) computes the length of the string str up to, but not including the terminating null character.
        // But we also want null termination so: + 1
        size_t length = strlen(type) + 1;
        // Reallocate memory for private member 'type'
        this->data.type = (char*)realloc(this->data.type, length * sizeof(char));
        // Copy received 'name' to private member 'type'
        strcpy(this->data.type, type);
      }
    };

    setOnState(JsonVariant value) {
      // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
      if(!value.isNull()) {
        bool state = value.as<bool>();
        this->data.on_state = state;
        updateOutput();
      }
    };

    setBrightness(JsonVariant value) {
      // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
      if(!value.isNull()) {
        int level = value.as<int>();
        this->data.brightness = level;
        updateOutput();
      }
    };

    storeName(void) {
      EEPROM_writeAnything(EEPROM_adr_name, this->data.name);
    }

    storeType(void) {
      EEPROM_writeAnything(EEPROM_adr_type, this->data.type);
    }

    storeOnState(void) {
      EEPROM_writeAnything(EEPROM_adr_on_state, this->data.on_state);
    }

    storeBrightness(void) {
      EEPROM_writeAnything(EEPROM_adr_brightness, this->data.brightness);
    }

  public:
    DeviceData() {
      // Allocate 1 character for strings and initialize to zero ('\0')
      // // No name
      // this->data.name = (char*)calloc(sizeof(char));
      // // No type
      // this->data.type = (char*)calloc(sizeof(char));
      // // Default OFF
      // this->data.on_state = false;
      // // 50% Brightness
      // this->data.brightness = 50;
      EEPROM_read_all(&this->data);
      updateOutput();
    };

    int Deserialize(StaticJsonDocument **doc, const char **jsonString) {
      // Deserialize the JSON string (remember doc is only MAX_JSON_SIZE bytes wide, so pass 'inputSize' also)
      DeserializationError error = deserializeJson(*doc, *jsonString, MAX_JSON_SIZE);
      // Test if parsing succeeds.
      if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.c_str());

        // Todo: Send error message over to backend using Socket.emit
        // Better: Define other error levels (-2, -3 ..) To indicate type of error (using enum)
        return -1;
      } else {
        // Success
        return 0;
      }
    };

    int Parse(StaticJsonDocument **doc, CommandOptions command) {
      // Automatically decide on the count of properties actually available (It's 4, but might change in the future)
      // sizeof(properties[0]) should be the count of bytes of the largest member of properties
      bool changed[sizeof(properties)/sizeof(properties[0])];
      int changedIt = 0;

      if(command > COMMAND_GET) {
        // Always implies set of internal properties if > COMMAND_GET
        // keep track if a property was actually given or not
        // This is simply to avoid parsing the properties all over again for the next step...
        // Using postincrement means: first read value, then increment
        changed[changedIt++] = this->setName(*doc["name"].as<JsonVariant>());
        changed[changedIt++] = this->setType(*doc["type"].as<JsonVariant>());
        changed[changedIt++] = this->setOnState(*doc["on_state"].as<JsonVariant>());
        changed[changedIt] = this->setBrightness(*doc["brightness"].as<JsonVariant>());
      }

      if(command == COMMAND_EEPROM) {
        // is higher than COMMAND_GET so they are already locally available
        changedIt = 0;
        // Do that incrementing stuff again
        if(changed[changedIt++]) this->storeName();
        if(changed[changedIt++]) this->storeType();
        if(changed[changedIt++]) this->storeOnState();
        if(changed[changedIt]) this->storeBrightness();
      }

      // Todo: Check validity

      // COMMAND_GET is always implied, we send back the actual state of our device
      // Note the reverse order of 'changedIt'
      if(changed[changedIt--]) *doc["brightness"].set(this->data.brightness);
      if(changed[changedIt--]) *doc["on_state"].set(this->data.on_state);
      if(changed[changedIt--]) *doc["type"].set(this->data.type);
      if(changed[changedIt]) *doc["name"].set(this->data.name);

      return 0;
    };

    int Serialize(StaticJsonDocument **doc, const char **jsonString) {
      // Computes the length of the minified JSON document that serializeJson() produces, excluding the null-terminator.
      // But we also want null termination so: + 1
      size_t len = measureJson(*doc) + 1;
      // Reallocate memory for our JSON string buffer
      jsonString = (char*)realloc(jsonString, len * sizeof(char));
      // Serialize the JSON document
      int bytes = serializeJson(*doc, *jsonString, len);
      // Test if parsing succeeds.
      if (bytes == 0) {
        Serial.print(F("serializeJson() failed: "));
        Serial.println("No bytes written");

        // Todo: Send error message over to backend using Socket.emit
        // Better: Define other error levels (-2, -3 ..) To indicate type of error (using enum)
        return -1;
      } else {
        // Success
        return 0;
      }
    }

    void print(void) {
      DEBUG_PRINTLN(this->data.name);
      DEBUG_PRINTLN(this->data.type);
      DEBUG_PRINTLN(this->data.on_state?"True":"False");
      DEBUG_PRINTLN(this->data.brightness);
    }
};

// global singleton instance of our Device
DeviceData states;
SocketIoClient Socket;

// JSON
//char[], as shown here, enables the "zero-copy" mode. This mode uses
char json[] = "{\"name\":\"Warmweiss LED Innen2\",\"type\":\"Light\",\"on_state\":true,\"brightness\":10}"; // Excemple
char test_get_json[] = "{\"name\",\"type\",\"on_state\",\"brightness\"}"; // Excemple

// StaticJsonDocument<255> doc; // [bytes] Allocate the capacity of the memory pool of th JSON document in the heap
// DynamicJsonDocument doc(1023); // allocates memory on the stack, it can replace by StaticJsonDocument

// Prototypes
// int deserialize(const char * json_obj, DataPacket_t* destination, int option, char* json_obj_out = "{}"); // Protoype to make 

void get(const char * payload, size_t length) {
  handleCommand(payload, length, COMMAND_GET);
}

void set(const char * payload, size_t length) {
  handleCommand(payload, length, COMMAND_SET);
}

void write_eeprom(const char * payload, size_t length) {
  handleCommand(payload, length, COMMAND_WRITE_EEPROM);
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
    Socket.emit("Error in set. Memory allocation failed."); // Send error
    return;
  }
  // Copy the payload to our buffer (using strncpy ensures we won't have a buffer overflow)
  strncpy(commandBuffer, payload, length);
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
  int success = states.Deserialize(&doc, &commandBuffer); // Option 2 for "set"
  if (success < 0) {
    Socket.emit("Error deserializing JSON string"); // Send error
  } else {
    // Todo: we need a serialize function that is able to generate a 'emittable' JSON String from our struct 'data'
    // The following will probably not work, would have to move every single property...
    // states = data; // Sore it to global "states"

    // If deserialization worked, let's head to parsing our JSON document now..
    success = states.Parse(&doc, option);
    if (success < 0) {
      Socket.emit("Error parsing data"); // Send error
    } else {
      // At this point we should have a beautiful JSON Document already containing our response
      // Serialize our response (produces JSON String)
      success = states.Serialize(&doc, &commandBuffer);
      if (success < 0) {
        Socket.emit("Error serializing JSON data"); // Send error
      } else {
        Socket.emit(commandBuffer);
      }
    }
  }

  // Free the allocated memory, we don't need it anymore
  free(commandBuffer);

  // Our global singleton 'states' instance takes care of itself..
  // LAMP_update(); // Update output
}

// void get(const char * payload, size_t lenght) {
//   // Since our received payload may be overridden by a subsequent command, we have to store it in a fresh buffer here!
//   // First dynamically allocate 'length' bytes of memory on the stack, we have to free this memory later!
//   char* commandBuffer = (char*)malloc(length*sizeof(char));
//   if(commandBuffer == NULL) {
//     // Error, we were unable to get the requested amount of memory :-(
//     Socket.emit("Error in set. Memory allocation failed."); // Send error
//     return;
//   }
//   // Copy the payload to our buffer (using strncpy ensures we won't have a buffer overflow)
//   strncpy(commandBuffer, payload, lenght);
//   // Though we have to ensure that our string is null terminated, so we simply set the last element to '\0'
//   // (requirement of 'deserializeJson()')
//   commandBuffer[lenght-1] = '\0';

//   // From here on we use our 'commandBuffer' instead of 'payload', since payload is just a pointer
//   // that we are not in control of. Additionally - and for this reason - we can disregard the original 'const' qualifier
//   DEBUG_PRINTLN(commandBuffer);
//   // No need for variable here, we just want to have something human readable.. using enum
//   // uint8_t option = 1; // Choose get option
//   char json_obj_out[255] = "{}"; // Empty json string, to be filled by deserialize function, will be emitted later
//   // DataPacket_t data; // Initialize empty data packet which will be filled
//   int success = deserialize(commandBuffer/*, &data*/, COMMAND_GET, json_obj_out); // Option 1 for "get"

//   if (success < 0) {
//     Socket.emit("Error in get. Error parsing content"); // Send error
//   } else {
//     // Todo: we need a serialize function that is able to generate a 'emittable' JSON String from our struct 'data'
//     Socket.emit("get", json_obj_out); // Send the state out
//   }

//   // Free the allocated memory, we don't need it anymore (have our 'data' struct now)
//   free(commandBuffer);
// }

// ---------------- JSON -----------------------
// int deserialize(const char * json_obj/*, DataPacket_t* destination*/, uint8_t option, char* json_obj_out){
//   // Deserialize the JSON document
//   DeserializationError error = deserializeJson(doc, json_obj);
//   // Test if parsing succeeds.
//   if (error) {
//     Serial.print(F("deserializeJson() failed: "));
//     Serial.println(error.c_str());

//     // Todo: Send error message over to backend using Socket.emit
//     return -1;
//   }
  
//   // Removed indirect access to, anyways, global 'states' DataPacket.. no need for that
//    switch(option){
//     case COMMAND_GET: // Get option was chosen
//       // simply fill the existing doc with requested data
//       if (doc.containsKey("name"))  doc["name"].set(states.getName()); // Tested
//       if (doc.containsKey("type"))  doc["type"].set(states.getType()); 
//       if (doc.containsKey("on_state"))  doc["on_state"].set(states.getOnState()); 
//       if (doc.containsKey("brightness"))  doc["brightness"].set(states.getBrightness()); 
//       // serializeJson(doc, json_obj_out); // Tried directly, but does not work
//       char json_obj_tmp[255]; // shitty, i want to put the infos directly into json_obj_out
//       serializeJson(doc, json_obj_tmp);
//       strcpy(json_obj_out, json_obj_tmp);
//       break;
//     case COMMAND_SET: // Set option was chosen: Stores values of JSON into destination (i.e. states)
//       // if (*_id != NULL) *id = *_id;
//       if (doc.containsKey("name")) states.setName(doc["name"]);
//       if (doc.containsKey("type"))  states.setType(doc["type"]);
//       if (doc.containsKey("on_state"))  states.setOnState(doc["on_state"];
//       if (doc.containsKey("brightness"))  states.setBrightness(doc["brightness"];
//       break;
//     case COMMAND_WRITE_EEPROM: // EEPROM option was chosen: Stores values of JSON into EEPROM
//       if (doc.containsKey("name"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["name"]);
//       if (doc.containsKey("type"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["type"]);
//       if (doc.containsKey("on_state"))  EEPROM_writeAnything(EEPROM_adr_on_state,doc["on_state"]);
//       if (doc.containsKey("brightness"))  EEPROM_writeAnything(EEPROM_adr_brightness,doc["brightness"]);
//       break;
//     default:
//       // This should never happen!
//       // Means we received an invalid/unknown command
//       break;
//   }
//   return 0;
// }

// ----------------- DEBUG -------------------
void printState(){
  states.print();
}

// --------------- SETUP -----------------
void setup(void) {
  // GPIO
  pinMode(PWM_PIN, OUTPUT);
  analogWriteRange(PWM_RANGE); // use 8 bit range instead of 10 bit so it's easier to handle
  digitalWrite(PWM_PIN, LOW);
    // EEPROM
  EEPROM.begin(EEPROM_size); // [byte] Set size of EEPROM up to 512
  // Have to use different approach
  // #ifdef SETUP
  //   set(json,sizeof(json));
  //   EEPROM_write_all(&states);
  // #endif
  // See constructor of 'states'
 //EEPROM_read_all(&states); // update status from EEPROM
  // LAMP_update(); // Set lamp to status read from EEPROM
  states = new DeviceData();

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
  Socket.on("setEEPROM", write_eeprom); // writes new default states into EEPROM
  // TODO: catch unknown event
}

void loop(void) {
  Socket.loop();
  // need to check wifi status constantly?
}
