
#include "alc.h"

const char *host = "212.227.9.150"; //IP des Java-Serversconst
uint16_t serverPort = 8440;         //Port des Java-Servers (ServerSocket)
WiFiClient wifiClient;
unsigned long last_change = 0;
unsigned long last_keepAlive = 0;
unsigned long now = 0;
bool messageRecived = false;
String username = "Nils";
bool sendUsername = false;

void connect()
{
  wifiClient.setTimeout(5000);
  if (!wifiClient.connect(host, serverPort))
  {
    Serial.println("Fehler beim Verbinden!");
    return;
  }
  
  wifiClient.setTimeout(1);
}

void async_request(){

  now = millis();
  
  if (now - last_keepAlive > 5000){
      last_keepAlive = now;
      wifiClient.println("--keepAlive--");
  }
  if (now - last_change > 1000){ 
    last_change = now;
    messageRecived = false;
    if (wifiClient.connected())
    {
      Serial.println("Server connection established!");
    }
    else
    {
    
      sendUsername = false;
      connect();
      return;
    }
    if(!sendUsername){
      Serial.println("Send Username");
    
      wifiClient.println(username);
      sendUsername = true;
      return;
    }

    
    Serial.println("EYYY");
    long l = millis();
    wifiClient.setTimeout(1);
    String line = wifiClient.readStringUntil('\n');
    Serial.println(line);
    
    if(line != NULL){
        Serial.println("ICH MUSS JETZT UPDATE");
        updateLED(line);
    }
    
    messageRecived = true;
    long l1 = millis();
    long neededDelay = l1 - l;
    Serial.println(neededDelay);
  }
}
