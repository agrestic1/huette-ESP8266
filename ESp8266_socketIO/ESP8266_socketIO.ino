#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <SocketIoClient.h> // requires SocketIoCleint package by Vincent Wyszynski and also WebSockets Package by Markus Sattler
#include <EEPROM.h> // necessary to store and read values from Flash (virtual EEPORM)
#include "privates.h" // conrains your WiFi SSID and PW, must be adjusted
// content of privates.h:
// const char* ssid = "yourSSID";
// const char* password = "yourPW";

SocketIoClient Socket;
#define DEBUG // DEBUG Option prints all that happens to serial Monitor

#ifdef DEBUG
 #define DEBUG_PRINT(x)     Serial.print (x)
 #define DEBUG_PRINTLN(x)  Serial.println (x)
#else
 #define DEBUG_PRINT(x)
 #define DEBUG_PRINTLN(x)
#endif 

#define PWM_PIN 13 // Pin to Output, 13 is LED_BUILTIN
#define EEPROM_size 1
// Globals
const int EEPROM_adr_bright = 255; // EEPROM Adress of brightness
int bright = 255; // Brightness, initialize to off (=255)

IPAddress staticIP(192, 168, 178, 22); // ESP8266's static IP adress
IPAddress gateway(192, 168, 178, 1); // Your Gateway
IPAddress subnet(255, 255, 255, 0); // Your Subnet mask

// TODO: change to JSON-based communication
void event(const char * payload, size_t lenght) {
  String text = String((char *) &payload[0]);
  if (text == "LED1") {
    LAMP_update();
    DEBUG_PRINTLN("led just lit");
  }
  if (text == "LED0") {
    LAMP_update();
    DEBUG_PRINTLN("led just lit");
    }
  if(text.startsWith("D")){
      String xVal=(text.substring(text.indexOf("D")+1,text.length())); // remove D from string
      bright = xVal.toInt();   
      if (bright > 255){ // respect range
        DEBUG_PRINTLN("Brightness value reduced from " + xVal + " to 255");
        bright = 255;
      } else if (bright < 0){ // respect range
        DEBUG_PRINTLN("Brightness value increased from " + xVal + " to 0");
        bright = 0;
      }
      bright = 255 - bright; // Range from 0 (off) to 255 (full brightness) instad of other way round
      EEPROM_update_write();
      LAMP_update(); 
      }
}

void LAMP_update(){
  analogWrite(PWM_PIN,bright); 
  DEBUG_PRINTLN("Brightness changed to ");
  DEBUG_PRINT(bright);
}

bool EEPROM_update_write() { // Writes all current values to EEPROM
  bool success = false;
  EEPROM.write(EEPROM_adr_bright, bright);
  if (EEPROM.commit()) {
    DEBUG_PRINTLN("EEPROM successfully committed");
    success = true;
  } else {
    DEBUG_PRINTLN("ERROR! EEPROM commit failed");
  }
  return success;
}

bool EEPROM_update_read() { // Writes all values from EEPROM
  bool success = false;
  if (true){ // TODO: 
    success = true;
    DEBUG_PRINTLN("ERROR read succeeded");
  } else {
    DEBUG_PRINTLN("ERROR read failed.");
  }
  bright = EEPROM.read(EEPROM_adr_bright);
  return success;
}

void sendType(const char * payload, size_t lenght){ // const char * payload, size_t leght necessary!
  Socket.emit("jsonObject", "{\"Type\":\"LED\"}");
}

void EEPROM_clear_all(){
  EEPROM.end();
  EEPROM.begin(512);
  // write a 0 to all 512 bytes of the EEPROM
  for (int i = 0; i < 512; i++) {
    EEPROM.write(i, 0);
  }
  EEPROM.end();
  EEPROM.begin(EEPROM_size);
}

void setup(void) {
  pinMode(PWM_PIN, OUTPUT);
  analogWriteRange(255); // use 8 bit range instead of 10 bit so it's easier to handle
  digitalWrite(PWM_PIN, LOW);
  Serial.begin(115200);
  EEPROM.begin(EEPROM_size); // [byte] Set size of EEPROM up to 512
  EEPROM_update_read(); // update status from EEPROM
  LAMP_update(); // Set lamp to status read from EEPROM
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet); // comment this line to use DHCP instead of static IP
  DEBUG_PRINTLN("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
  }
  DEBUG_PRINT("Connected to ");
  DEBUG_PRINTLN(ssid);
  DEBUG_PRINT("IP address: ");
  DEBUG_PRINTLN(WiFi.localIP());


  const char* host = "192.168.178.69";
  int port = 8080;
  Socket.begin(host, port);
  Socket.on("connect", sendType);
  Socket.on("event", event);
}

void loop(void) {
  Socket.loop();
  // need to check wifi status constantly?
}
