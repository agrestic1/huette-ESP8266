<h1 align="center">huette-ESP8266</h1>

<p align = "center">NodeJS server running huette-raspberry communicates with ESP8266 to control it's LEDs</p>

## Index
- [Requirements](#requirements)
- [Install](#install)

## Requirements <a name = "requirements"></a>
In Adruino IDE add Packages:  
WebSockets Package by Markus Sattler  
SocketIoClient by Vincent Wyszynski, see also https://github.com/timum-viw/socket.io-client  
// WiFiManager by tzapi, see also https://github.com/tzapu/WiFiManager  
ArduinoJsnon package by Benoit Blanchon, see also https://arduinojson.org/  

Add ESP8266 Board:  
In Preferences you need to add additional Boards Manager source: https://arduino.esp8266.com/stable/package_esp8266com_index.json  
Then, in Board Manager add esp8266 by ESP8266 Community  

## Install <a name = "install"></a>  
Create a file "privates.h" that conrains the WiFi SSID and PW of your server  
content of privates.h:  
const char* ssid = "yourSSID";  
const char* password = "yourPW";  
 
Adjust staticIP, gateway and subnet according to your network or  
comment the following line to use DHCP instead of static IP:  
WiFi.config(staticIP, gateway, subnet);  

### 💡 The SocketIoClient library uses lstdc++
To compile you have to add a reference to the linker. 
To do so edit platform.txt in $ARDUINO_IDE/hardware/esp8266com/esp8266 and add '-lstdc++' to the line 
```
compiler.c.elf.libs= ...
```
💡 **Note:** See [ESP8266 for Arduino IDE (xtensa-lx106-elf-gcc) and std::map linking error](http://stackoverflow.com/questions/33450946/esp8266-for-arduino-ide-xtensa-lx106-elf-gcc-and-stdmap-linking-error).
