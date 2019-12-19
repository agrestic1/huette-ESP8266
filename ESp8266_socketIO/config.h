#ifndef CONFIG_H /* START GUARD */
#define CONFIG_H

#include "privates.h" // conrains privat info like WiFi SSID and PW, must be adjusted
// content of privates.h:
// const char* ssid = "yourSSID";
// const char* password = "yourPW";


// Webserver
#define HOST "192.168.188.148" // Webservers IP
// #define HOST "192.168.178.26" // Webservers IP
#define PORT 8030 // Webservers Port to Device Socket

#endif