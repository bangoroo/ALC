
#include "alc.h"



const char* host = "rikorick.de"; //IP des Java-Serversconst 
int serverPort = 8440; //Port des Java-Servers (ServerSocket)
WiFiClient wifiClient;

void connect(){
  if(!wifiClient.connect(host,serverPort)){
      Serial.print("Fehler beim Verbinden!");
      return;
  }
  Serial.print("Verbindung herrgestellt!");
}

void async_request(){
   if(wifiClient.connected){
    wifiClient.println("sendMessage"());
    Serial.print("Message send!");
    String line = wifiClient.readStringUntil('\n');
    wifiClient.print(line);
    wifiClient.flush();
  }
  
}

