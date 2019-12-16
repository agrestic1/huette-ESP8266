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
    const char *properties[4] = { "name", "type", "on_state", "brightness" };
    DataPacket_t data;

    void updateOutput(void) {
#ifdef DEFLED_BUILTIN
      if (this->data.on_state){
        analogWrite(LED_BUILTIN,PWM_RANGE-this->data.brightness); 
      } else {
        analogWrite(LED_BUILTIN,PWM_RANGE); 
      }
#else
      if (states->on_state){
        analogWrite(PWM_PIN,this->data.brightness); 
      } else {
        analogWrite(PWM_PIN,0); 
      }
#endif
      DEBUG_PRINT("Brightness changed to ");
      DEBUG_PRINTLN(this->data.brightness);
    };

    bool setName(JsonVariant value) {
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
        return true;
      }

      return false;
    };

    bool setType(JsonVariant value) {
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
        return true;
      }

      return false;
    };

    bool setOnState(JsonVariant value) {
      // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
      if(!value.isNull()) {
        bool state = value.as<bool>();
        this->data.on_state = state;
        updateOutput();
        return true;
      }

      return false;
    };

    bool setBrightness(JsonVariant value) {
      // See API note regarding '.containsKey()': https://arduinojson.org/v6/api/jsonobject/containskey/
      if(!value.isNull()) {
        int level = value.as<int>();
        this->data.brightness = level;
        updateOutput();
        return true;
      }

      return false;
    };

    void storeName(void) {
      EEPROM_writeAnything(EEPROM_adr_name, this->data.name);
    }

    void storeType(void) {
      EEPROM_writeAnything(EEPROM_adr_type, this->data.type);
    }

    void storeOnState(void) {
      EEPROM_writeAnything(EEPROM_adr_on_state, this->data.on_state);
    }

    void storeBrightness(void) {
      EEPROM_writeAnything(EEPROM_adr_brightness, this->data.brightness);
    }

  public:
    DeviceData() {
      // Allocate 1 character for strings and initialize to zero ('\0')
      // No name
      this->data.name = (char*)calloc(1, sizeof(char));
      // No type
      this->data.type = (char*)calloc(1, sizeof(char));
      // Default OFF
      this->data.on_state = false;
      // 50% Brightness
      this->data.brightness = 50;

      // Todo: Manage EEPROM

      // EEPROM
      EEPROM.begin(EEPROM_size); // [byte] Set size of EEPROM up to 512
      // EEPROM_read_all(&this->data);

      updateOutput();
    };

    int Deserialize(JsonDocument& doc, const char *jsonString) {
      // Deserialize the JSON string (remember doc is only MAX_JSON_SIZE bytes wide, so pass 'inputSize' also)
      DeserializationError error = deserializeJson(doc, jsonString, (size_t)MAX_JSON_SIZE);
      // Test if parsing succeeds.
      if (error) {
        // Serial.print(F("deserializeJson() failed: "));
        // Serial.println(error.c_str());
        DEBUG_PRINT("deserializeJson() failed: ");
        DEBUG_PRINTLN(error.c_str());

        // Todo: Send error message over to backend using Socket.emit
        // Better: Define other error levels (-2, -3 ..) To indicate type of error (using enum)
        return -1;
      } else {
        // Success
        return 0;
      }
    };

    int Parse(JsonDocument& doc, CommandOptions command) {
      // Automatically decide on the count of properties actually available (It's 4, but might change in the future)
      // sizeof(properties[0]) should be the count of bytes of the largest member of properties
      bool changed[sizeof(properties)/sizeof(properties[0])];
      int changedIt = 0;

      if(command > COMMAND_GET) {
        // Always implies set of internal properties if > COMMAND_GET
        // keep track if a property was actually given or not
        // This is simply to avoid parsing the properties all over again for the next step...
        // Using postincrement means: first read value, then increment
        changed[changedIt++] = this->setName(doc["name"].as<JsonVariant>());
        changed[changedIt++] = this->setType(doc["type"].as<JsonVariant>());
        changed[changedIt++] = this->setOnState(doc["on_state"].as<JsonVariant>());
        changed[changedIt] = this->setBrightness(doc["brightness"].as<JsonVariant>());
      }

      if(command == COMMAND_WRITE_EEPROM) {
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
      if(changed[changedIt--]) doc["brightness"].set(this->data.brightness);
      if(changed[changedIt--]) doc["on_state"].set(this->data.on_state);
      if(changed[changedIt--]) doc["type"].set(this->data.type);
      if(changed[changedIt]) doc["name"].set(this->data.name);

      return 0;
    };

    int Serialize(const JsonDocument& doc, char *jsonString) {
      // Computes the length of the minified JSON document that serializeJson() produces, excluding the null-terminator.
      // But we also want null termination so: + 1
      size_t len = measureJson(doc) + 1;
      // Reallocate memory for our JSON string buffer
      jsonString = (char*)realloc(jsonString, len * sizeof(char));
      // Serialize the JSON document
      size_t count = serializeJson(doc, jsonString, len);
      // Test if parsing succeeds.
      if (count == 0) {
        // Serial.print(F("serializeJson() failed: "));
        // Serial.println("No bytes written");
        DEBUG_PRINT("serializeJson() failed: "));
        DEBUG_PRINTLN("No bytes written");

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
DeviceData *states;
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

void publish(const char* payload, size_t length) {
  // Todo: build JSON string from current setup and emit over socket
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
  int success = states->Deserialize(doc, (const char*)commandBuffer); // Option 2 for "set"
  if (success < 0) {
    Socket.emit("error", "{Error deserializing JSON string.}"); // Send error
  } else {
    // If deserialization worked, let's head to parsing our JSON document now..
    DEBUG_PRINTLN("Deserialization succeeded");
    // Parse the resulting JSON document
    success = states->Parse(doc, option);
    if (success < 0) {
      Socket.emit("error", "{Error parsing data.}"); // Send error
    } else {
      // At this point we should have a beautiful JSON Document already containing our response
      DEBUG_PRINTLN("Parsing succeeded");
      // Serialize our response (produces JSON String)
      success = states->Serialize(doc, commandBuffer);
      if (success < 0) {
        Socket.emit("error", "{Error serializing JSON data.}"); // Send error
      } else {
        DEBUG_PRINTLN("Serialization succeeded");
        switch(option) {
          case COMMAND_GET:
            Socket.emit("get", commandBuffer);
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
  states->print();
}

void setupSerial() {
// Serial
#ifdef DEBUG
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
    DEBUG_PRINT("Waiting for connection..");
  }
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(ssid);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());
}


// --------------- SETUP -----------------
void setup(void) {
  setupSerial();
  setupPeripherals();
  setupWifi();

  states = new DeviceData();

  // Socket
  Socket.begin(HOST, PORT); // Connected to Device Socket using config from config.h
  // Subscriptions:
  Socket.on("get", get); // get selected properties from this device
  Socket.on("set", set); // set selected properties on this device
  Socket.on("publish", publish); // publish all properties 
  Socket.on("write_eeprom", write_eeprom); // writes selected properties to EEPROM as default settings

}

void loop(void) {
  Socket.loop();
  // Todo: need to check wifi status constantly?
}
