#include "alc.h"
//#include <WiFiClientSecure.h>
//#include "FastLED.h"
bool done = false;

#define BRIGHTNESS_MAX 10 // Helligkeit für usb Betrieb begrenzen

JsonArray leds;
byte red;
byte green;
byte blue;

const char fingerprint[] PROGMEM = "d53a2c12f35f16c2e824c009bc4c077ef5d48a72";
byte lasteffect = 0;
/****************************************FOR ws2812FX***************************************/
//Number of LEDs
#define NUM_LEDS 43
//Pin of WS2812B strip
#define LED_PIN 0 //D3 = GIPO0 // D4 = GIPO 2
//Sound PIN
#define SOUND_PIN D1

/*
 Parameter 1 = number of pixels in strip
 Parameter 2 = Arduino pin number (most are valid)
 Parameter 3 = pixel type flags, add together as needed:
   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)
*/
WS2812FX ws2812fx = WS2812FX(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);
#define TIMER_MS 5000
unsigned long last_change = 0;
unsigned long now = 0;

void update_ws2812fx(){
  ws2812fx.service();
}

void setup()
{
  Serial.begin(115200);
  setup_wifi();
  /********************************** START SETUP ws2812FX*****************************************/
  Serial.print("WS2812FX Setup...");
  //initialize ws2812fx
  ws2812fx.init();

  // parameters: index, start, stop, mode, color, speed, reverse
  //ws2812fx.setSegment(0,  0,  NUM_LEDS, FX_MODE_STATIC, 0x007BFF, 1000, false); // segment 0 is leds 0 - 300
  ws2812fx.setMode(FX_MODE_BLINK_RAINBOW);
  ws2812fx.setBrightness(50);
  ws2812fx.setSpeed(1000);
  ws2812fx.setColor(0x0000ff);
  ws2812fx.setCustomMode(singleLEDS);
  ws2812fx.start();

  Serial.println("Done");
  pinMode(A0, INPUT);
  
}

/********************************** START SETUP WIFI*****************************************/

void setup_wifi()
{
  WiFiManager wifiManager;
  wifiManager.autoConnect("ALC_AP");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  connect();
}

void loop()
{
  now = millis();
  if (now - last_change > TIMER_MS)
  {
    //getUpdate();
    async_request();
    last_change = now;
  }

  ws2812fx.service();
}

/*
void getUpdate(){
  //test json erstellen
   DynamicJsonDocument jsonDoc(BUFFER_SIZE);
   jsonDoc["state"] = true;
   jsonDoc["mode"] = false;
   jsonDoc["brightness"] = 10;
   jsonDoc["color"]["red"] = 15;
   jsonDoc["color"]["green"] = 155;
   jsonDoc["color"]["blue"]= 15;
   JsonArray array = jsonDoc.createNestedArray("leds");
   array.add(1);
   array.add(3);
   array.add(5);
   array.add(10);
   array.add(25);
   array.add(35);
   jsonDoc["effect"]=12;
   jsonDoc["speed"]=1000;
   //serializeJsonPretty(jsonDoc, Serial);
   updateLED(jsonDoc);
}*/

//Durch asyncTCP ersetzt
/* void getUpdate()
{
  Serial.println(F("Connecting..."));

  // Connect to HTTP server
  WiFiClientSecure client;
  Serial.printf("Using fingerprint '%s'\n", fingerprint);
  client.setFingerprint(fingerprint);
  client.setTimeout(15000);
  ws2812fx.service();
  if (!client.connect("rikorick.de", 443))
  {
    Serial.println(F("Connection failed"));
    return;
  }

  Serial.println(F("Connected!"));

  // Send HTTP request
  client.println(F("GET /Users/Nils-presets.json HTTP/1.1"));
  client.println(F("Host: rikorick.de"));
  client.println(F("Connection: close"));
  if (client.println() == 0)
  {
    Serial.println(F("Failed to send request"));
    return;
  }

  // Check HTTP status
  char status[32] = {0};
  client.readBytesUntil('\r', status, sizeof(status));
  if (strcmp(status, "HTTP/1.1 200 OK") != 0)
  {
    Serial.print(F("Unexpected response: "));
    Serial.println(status);
    return;
  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client.find(endOfHeaders))
  {
    Serial.println(F("Invalid response"));
    return;
  }


  // Allocate the JSON document
  // Use arduinojson.org/v6/assistant to compute the capacity.
  
  DynamicJsonDocument doc(BUFFER_SIZE);

  // Parse JSON object
  DeserializationError error = deserializeJson(doc, client);
  if (error)
  {
    Serial.print("deserializeJson() failed: ");
    Serial.println(error.c_str());
    return;
  }
  else
  {
    // Extract values
    Serial.println("Response:");
    updateLED(doc);
  }
    // Disconnect
  client.stop();

} */

//Json auswerten und LEDs updaten
void updateLED(DynamicJsonDocument jsonDoc)
{
  done = false;
  //Json auswerten
  //serializeJsonPretty(jsonDoc, Serial);
  bool state = jsonDoc["state"];
  //Serial.printf("State: %d", state);
  //bool mode = jsonDoc["mode"];
  byte brightness = jsonDoc["brightness"];
  brightness = map(brightness, 0,100,0,255);
  //Serial.printf("Brightness: %d", brightness);

  JsonObject color = jsonDoc["color"];
  red = color["red"];
  green = jsonDoc["color"]["green"];
  blue =  jsonDoc["color"]["blue"];
  // JsonArray color = jsonDoc["color"];
  // int red = color[0];   
  // int green = color[1]; 
  // int blue = color[2];  
  //Serial.printf("Color: red %d green %d blue %d", red, green, blue);

  leds = jsonDoc["leds"];//.as<JsonArray>(); // jsonDoc["leds"];
  
  byte effect = jsonDoc["effect"];

  //Serial.printf("Effekt: %s", ws2812fx.getModeName(effect));
  uint16_t speed = jsonDoc["speed"];
  speed = 100 - speed; //speed invertieren, da die Zeit zwischen aktualisierungen angegeben wird
  speed = map(speed, 0, 100, 0, 65535); //Geschwindigkeit auf einen anderen Bereich anpassen
  //Serial.printf("Speed: %d", speed);

  if (!state)
  {
    ws2812fx.clear();
    ws2812fx.strip_off();
    ws2812fx.stop();
    Serial.println("Turning off");
  }

  else if (state)
  {
    ws2812fx.setBrightness(brightness);
    ws2812fx.setColor(red, green, blue);
    ws2812fx.setSpeed(speed);
    if(!ws2812fx.isRunning()){
      ws2812fx.start();
    }
    
    //Setze vordefinierten Effekt
    if (effect != lasteffect)
    {
      lasteffect = effect;
      ws2812fx.setMode(effect);
      Serial.println("Effekt update");
      Serial.println(ws2812fx.getModeName(effect));
    }
  }
}

uint16_t singleLEDS(void){
  if(!done){
    ws2812fx.clear();
      //Array auslesen
      for (byte i = 0; i< leds.size();i++)
    {
     // ws2812fx.setPixelColor(leds[i-56], red, green, blue);
     ws2812fx.setPixelColor(leds[i].as<byte>(), red, green, blue); //  <-----Error 28/29 https://github.com/esp8266/Arduino/blob/master/doc/exception_causes.rst 
      Serial.printf("Set led %d\n", leds[i].as<byte>());
     // ws2812fx.show();
     delay(10); // kurzes delay
    }
    done = true;
  }
  // return the animation speed based on the ws2812fx speed setting
  return (ws2812fx.getSpeed() / NUM_LEDS);
}


//TODO: integrate FastLED
//CRGB leds_f[NUM_LEDS];
//Sound reactive
/*   void soundEffect()
  {
    if (digitalRead(SOUND_PIN) == 1 )
    {
      fill_solid(leds_f, NUM_LEDS, CHSV(random8(), 255, 255));
      FastLED.show();
      //showleds();

      while (digitalRead(SOUND_PIN) == 1 )
      {
        //wait...
        delay(2);
        yield();
      }
    }
    else
    { 
       FastLED.clear();
    }
  } */