// Webserver
#define HOST "192.168.178.45" // Webservers IP
#define PORT 8081 // Webservers Port to Device Socket

// Output
#define DEFLED_BUILTIN // Route PWM to LED_BUILTIN instead PWM_PIN
#define PWM_RANGE 100 // range for analogwrite
#define PWM_PIN 13 // Pin to Output, ATTENTION: not used ifdef DEFLED_BUILTIN