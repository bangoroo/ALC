#include "alc.h"

//#define USB_POWER 10  // Helligkeit für usb Betrieb begrenzen
#define NUM_COLORS 3  //Anzahl der Farben

//Json Objekt für die ansteuerung einzelner LEDs
JsonObject ledObj;

//Array für die Farben
uint32_t colorArray[NUM_COLORS] = {0xff0000, 0x00ff00, 0x0000ff};

//variable zum überprüfen, ob die einzelnen LEDs geschrieben wurden
bool done = false;           // für SingleLeds

//ON/OFF nach Helligkeit
bool autoMode = false;       // LDR (de)aktiviert
uint16_t switchValue = 500;  // Schwellwert LDR
unsigned long nowAuto, lastChange; //Variablen für Intervall

//Speicher des leztzen effects
byte lasteffect = -1;

// LDR PIN
#define LDR_PIN A0

/****************************************Für ws2812FX***************************************/
// Anzahl der LEDs
byte NUM_LEDS = 42;
byte previous_NUM_LEDS = 0; //Anzahl vor Update

// Pin des WS2812B strip
#define LED_PIN 0  // D3 = GIPO0 // D4 = GIPO 2

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

/*******************************RGB in HEX konvertieren************************************/
void rgbToHex(byte red, byte green, byte blue) {
  // Konvertiere Farbe
  //Verschiebe Farbwerte 
  uint32_t hexC = ((uint32_t)red << 16) | ((uint32_t)green << 8) | blue;
  // Farbarray verschieben
  if (hexC != colorArray[0]) {
    for (int c = NUM_COLORS - 1; c >= 0; c--) {
      colorArray[c + 1] = colorArray[c];
    }
    // update color
    colorArray[0] = hexC;
    ws2812fx.setColors(0, colorArray);
  }
}

/****************************************SETUP**********************************************/
void setup() {
  Serial.begin(115200);
  setup_wifi();
  /******************************* START SETUP ws2812FX*************************************/
  Serial.print("WS2812FX Setup...");
  // Initialiesiere ws2812fx
  ws2812fx.init();
  //Setze Helligkeit
  ws2812fx.setBrightness(10);
  // ws2812fx.setSpeed(1000);
  // ws2812fx.setColor(0x0000ff);

  //Setze Effekt für die Ansteuerung einzelner LEDs
  ws2812fx.setCustomMode(singleLEDS);
  //Segment konfigurieren
  // parameters: index, start, stop, mode, color, speed, reverse
  ws2812fx.setSegment(0, 0, NUM_LEDS, FX_MODE_BLINK_RAINBOW, colorArray, 1000,
                      false);
  //WS2812FX starten
  ws2812fx.start();

  Serial.println("Done");

  //Definiere LDR als Eingang
  pinMode(LDR_PIN, INPUT);
}

/********************************** START SETUP WIFI*****************************************/

void setup_wifi() {
  WiFiManager wifiManager;
  wifiManager.setTimeout(900);
  wifiManager.autoConnect("ALC_AP");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  delay(100);
  connect();
}


void loop() {
  async_request();
  ws2812fx.service();

  if (autoMode) {
    nowAuto = millis();
    if (nowAuto - lastChange > 1000) {
      lastChange = nowAuto;
      ldrMode();
    }
  }
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



// Json auswerten und LEDs updaten
void updateLED(String config) {
  //reset done
  done = false;

  //Erstelle JSONDocument
  DynamicJsonDocument jsonDoc(BUFFER_SIZE);
  //String als JSON parsen
  DeserializationError error = deserializeJson(jsonDoc, config);
  // Json auswerten
  // serializeJsonPretty(jsonDoc, Serial); //JSON ausgeben
  if (error)  // JSON File konnte nicht verarbeitet werden
  {
    Serial.print("Fehler in der config: "); //Fehler ausgeben
    Serial.println(error.c_str());
  }
  // Json auslesen
  else {
    //Status auslesen
    uint state = jsonDoc["state"];
    // Serial.printf("State: %d", state);
    
    // Prüfen ob Helligkeit limitiert werden muss
    #ifdef USB_POWER
        byte brightness = jsonDoc["brightness"];
        if (brightness >= USB_POWER) {
          brightness = USB_POWER;
        }
    #else
        byte brightness = jsonDoc["brightness"];
        brightness = map(brightness, 0, 100, 0, 255);
    #endif
    // Serial.printf("Brightness: %d", brightness);
    
    // Farbe auslesen
    JsonObject color = jsonDoc["color"];
    byte red = color["red"];
    byte green = jsonDoc["color"]["green"];
    byte blue = jsonDoc["color"]["blue"];
    // Serial.printf("Color: red %d green %d blue %d", red, green, blue);

    //Farbe konvertieren
    rgbToHex(red, green, blue);
    
    // leds = jsonDoc["leds"]; //Array für Custom LEDs
    //JSON objekt mit einzelnen LEDs
    ledObj = jsonDoc["leds"];

    //ändert die anzahl der LEDs
    if (NUM_LEDS != jsonDoc["ledcount"]) {
      NUM_LEDS = jsonDoc["ledcount"];
      ws2812fx.setLength(NUM_LEDS);
    }

    // Effekt auslesen
    byte effect = jsonDoc["effect"];
    // Serial.printf("Effekt: %s", ws2812fx.getModeName(effect));

    // Animationsgeschwindigkeit auslesen
    uint16_t speed = jsonDoc["speed"];
    speed = 100 - speed;  // speed invertieren, da die Zeit zwischen
                          // aktualisierungen angegeben wird
    speed = map(speed, 0, 100, 0,
                65535);  // Geschwindigkeit auf einen anderen Bereich anpassen
    // Serial.printf("Speed: %d", speed);

    //Schwellwert für LDR anpassen
    if (jsonDoc.containsKey("switchValue")) {
      switchValue = jsonDoc["switchValue"];
    }


    // Modus prüfen 0=OFF, 1=ON, 2=AUTO
    if (state == 0) {
      ws2812fx.clear();
      ws2812fx.strip_off();
      ws2812fx.stop();
      Serial.println("Turning off");
      autoMode = false;
    } else if (state == 1) {
      ws2812fx.setBrightness(brightness);
      // ws2812fx.setColor(red, green, blue);
      ws2812fx.setSpeed(speed);
      // ws2812fx.setColors(0, colorArray);
      autoMode = false;
      if (!ws2812fx.isRunning()) {
        ws2812fx.start();
      }
    }
    // steuerung via LDR
    else if (state == 2) {
      autoMode = true;
      // ws2812fx.setColor(red, green, blue);
      ws2812fx.setSpeed(speed);
    }

    // Setze Effekt
    if (effect != lasteffect) {
      lasteffect = effect;
      ws2812fx.clear();
      ws2812fx.setMode(effect);
      Serial.println("Effekt update");
      Serial.println(ws2812fx.getModeName(effect));
    }
  }
}

// Steuerung nach Helligkeit
void ldrMode() {
  uint16 ldrValue = 0;
  // Mehrfachmessung für bessere Ergebnisse
  for (int i = 0; i < 10; i++) {
    ldrValue += analogRead(LDR_PIN);
  }
  Serial.printf("Mittelwert LDR: %u \n", ldrValue / 10);

  // Prüfen ob Schwellwert überschritten wird
  if (ldrValue / 10 >= switchValue) {
    if (!ws2812fx.isRunning()) {
      ws2812fx.start();
    }
  #ifdef USB_POWER
      ws2812fx.setBrightness(USB_POWER);
  #else
      ws2812fx.setBrightness(
          BRIGHTNESS_MAX - map(ldrValue, 0, 255, BRIGHTNESS_MIN, BRIGHTNESS_MAX));
  #endif

  } else {
    if (ws2812fx.isRunning()) {
      ws2812fx.stop();
      ws2812fx.clear();
    }
    // Serial.println("Auto off");
  }
}

// Einzelne LEDs ansteuern
uint16_t singleLEDS(void) {
  if (!done) {
    ws2812fx.clear();
    // Array auslesen
    /* for (byte i = 0; i < leds.size(); i++)
    {
      // ws2812fx.setPixelColor(leds[i-56], red, green, blue);
      JsonObject color = leds[i]["color"];
      Serial.print(color["red"].as<byte>());
      ws2812fx.setPixelColor(leds[i]["num"].as<byte>(), color["red"],
    color["green"], color["blue"]);
      Serial.printf("Set led %d\n", leds[i].as<byte>());
      // ws2812fx.show();
      delay(10); // kurzes delay
    } */

    //Loop durch einzelne Objekte
    for (JsonPair pair : ledObj) {
      //Key auslesen
      String numS = pair.key().c_str();
      //Key in INT konvertieren
      uint32_t num = numS.toInt();
      //HEX-Farbe auslesen
      String hexcolor = pair.value().as<char*>();
      hexcolor.remove(0, 1);  //# entfernen
      //HEX-String in Integer konvertieren
      uint32_t color = strtoul(hexcolor.c_str(), nullptr, 16);

      //Farbe updaten
      ws2812fx.setPixelColor(num, color);
      // Serial.printf("Led: %d , Farbe: %d \n", num, color);
    }

    done = true;
  }
  // return Animationsspeed basierend auf Geschwindigkeitseinstellung
  return (ws2812fx.getSpeed() / NUM_LEDS);
}
