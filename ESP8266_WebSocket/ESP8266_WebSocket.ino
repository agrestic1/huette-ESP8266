#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266mDNS.h>
#include <WebSocketsServer.h> // requires WebSocket package by Markus Sattler
#include "privates.h" // conrains your WiFi SSID and PW, must be adjusted
// content of privates.h:
// const char* ssid = "yourSSID";
// const char* password = "yourPW";

WebSocketsServer webSocket = WebSocketsServer(81);

IPAddress staticIP(192, 168, 178, 22); // ESP8266's static IP adress
IPAddress gateway(192, 168, 178, 1); // Your Gateway
IPAddress subnet(255, 255, 255, 0);


void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t lenght) {

  switch (type) {
    case WStype_DISCONNECTED:

      break;
    case WStype_CONNECTED:
      {
        IPAddress ip = webSocket.remoteIP(num);
      }
      break;
    case WStype_TEXT:
      {

        String text = String((char *) &payload[0]);

        if (text == "LED1") {
          digitalWrite(LED_BUILTIN, LOW);
          Serial.println("led just lit");
          webSocket.sendTXT(num, "led just lit", lenght);
        }
        if (text == "LED0") {
          digitalWrite(LED_BUILTIN, HIGH);
          Serial.println("led just lit");
          webSocket.sendTXT(num, "led just lit", lenght);
        }
        if(text.startsWith("D")){
            String xVal=(text.substring(text.indexOf("D")+1,text.length())); // remove D from string
            int xInt = xVal.toInt();
            analogWrite(LED_BUILTIN,xInt); 
            Serial.println(xVal);
            webSocket.sendTXT(num, "brightness changed", lenght);
           }
      }


      webSocket.sendTXT(num, payload, lenght);
      webSocket.broadcastTXT(payload, lenght);
      break;

    case WStype_BIN:

      hexdump(payload, lenght);

      // echo data back to browser
      webSocket.sendBIN(num, payload, lenght);
      break;
  }

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

  webSocket.begin();
  webSocket.onEvent(webSocketEvent);
}

void loop(void) {
  webSocket.loop();
}
