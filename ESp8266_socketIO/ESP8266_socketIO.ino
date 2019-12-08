#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
//#include <ESP8266WebServer.h>
#include <SocketIoClient.h> // requires SocketIO package by Markus Sattler
#include "privates.h" // conrains your WiFi SSID and PW, must be adjusted
// content of privates.h:
// const char* ssid = "yourSSID";
// const char* password = "yourPW";

SocketIoClient Socket;

IPAddress staticIP(192, 168, 178, 22); // ESP8266's static IP adress
IPAddress gateway(192, 168, 178, 1); // Your Gateway
IPAddress subnet(255, 255, 255, 0);

void event(const char * payload, size_t lenght) {
        Serial.println("event");
        String text = String((char *) &payload[0]);
        if (text == "LED1") {
          digitalWrite(LED_BUILTIN, LOW);
          Serial.println("led just lit");
          //webSocket.sendTXT(num, "led just lit", lenght);
          Socket.emit("plainString", "\"this is a plain string\"");
        }
        if (text == "LED0") {
          digitalWrite(LED_BUILTIN, HIGH);
          Serial.println("led just lit");
          //webSocket.sendTXT(num, "led just lit", lenght);
        }
        if(text.startsWith("D")){
            String xVal=(text.substring(text.indexOf("D")+1,text.length())); // remove D from string
            int xInt = xVal.toInt();
            analogWrite(LED_BUILTIN,xInt); 
            Serial.println(xVal);
            //webSocket.sendTXT(num, "brightness changed", lenght);
           }


      //webSocket.sendTXT(num, payload, lenght);
      //webSocket.broadcastTXT(payload, lenght);
}

void sendType(const char * payload, size_t lenght){ // const char * payload, size_t leght necessary!
  Socket.emit("jsonObject", "{\"Type\":\"LED\"}");
}

void setup(void) {
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(staticIP, gateway, subnet); // comment this line to use DHCP instead of static IP
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  const char* host = "192.168.178.69";
  int port = 8081;
  Socket.begin(host, port);
  Socket.on("connect", sendType);
  Socket.on("event", event);
}

void loop(void) {
  Socket.loop();
}
