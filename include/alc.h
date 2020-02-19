#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include "WS2812FX.h"
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>  



const int BUFFER_SIZE = JSON_OBJECT_SIZE(50);

void setup();
void setup_wifi();
void getUpdate();
void updateLED(DynamicJsonDocument jsonDoc);
uint16_t singleLEDS();
void update_ws2812fx();
void async_request();
void connect();