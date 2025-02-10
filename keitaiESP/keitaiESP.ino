#define OLED_SOFT_BUFFER_64
#include <GyverOLED.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
GyverOLED<SSD1306_128x64> oled;

#define LED_COUNT 1
#define DATA_PIN 3
CRGB leds[LED_COUNT];

int max_bright = 100;

int MenusLen[] = {10, 3, 3};
char* MainMenu[] = {"LED", "Wi-Fi", "ARGB", "4", "5", "6", "7", "8", "9", "10"};
char* FlashlightMenu[] = {"white color", "red color", "back"};
char* WiFiMenu[] = {"Wi-Fi spammer", "Wi-Fi scanner", "back"};

bool whiteLED = false;
bool redLED = false;


extern "C" {
#include "user_interface.h"
  typedef void (*freedom_outside_cb_t)(uint8 status);
  int wifi_register_send_pkt_freedom_cb(freedom_outside_cb_t cb);
  void wifi_unregister_send_pkt_freedom_cb(void);
  int wifi_send_pkt_freedom(uint8 *buf, int len, bool sys_seq);
}

const uint8_t channels[] = {1, 6, 11}; // used Wi-Fi channels (available: 1-14)
const bool wpa2 = false; // WPA2 networks
const bool appendSpaces = true; // makes all SSIDs 32 characters long to improve performance

const char ssids[] PROGMEM = {
  "Free Wi-Fi\n"
  "FREE WIFI\n"
};

char emptySSID[32];
uint8_t channelIndex = 0;
uint8_t macAddr[6];
uint8_t wifi_channel = 1;
uint32_t currentTime = 0;
uint32_t packetSize = 0;
uint32_t packetCounter = 0;
uint32_t attackTime = 0;
uint32_t packetRateTime = 0;

uint8_t beaconPacket[109] = {
  /*  0 - 3  */ 0x80, 0x00, 0x00, 0x00,             // Type/Subtype: managment beacon frame
  /*  4 - 9  */ 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // Destination: broadcast
  /* 10 - 15 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
  /* 16 - 21 */ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, // Source
  // Fixed parameters
  /* 22 - 23 */ 0x00, 0x00,                         // Fragment & sequence number (will be done by the SDK)
  /* 24 - 31 */ 0x83, 0x51, 0xf7, 0x8f, 0x0f, 0x00, 0x00, 0x00, // Timestamp
  /* 32 - 33 */ 0xe8, 0x03,                         // Interval: 0x64, 0x00 => every 100ms - 0xe8, 0x03 => every 1s
  /* 34 - 35 */ 0x31, 0x00,                         // capabilities Tnformation
  // Tagged parameters
  // SSID parameters
  /* 36 - 37 */ 0x00, 0x20,                         // Tag: Set SSID length, Tag length: 32
  /* 38 - 69 */ 0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,
  0x20, 0x20, 0x20, 0x20,                           // SSID
  // Supported Rates
  /* 70 - 71 */ 0x01, 0x08,                         // Tag: Supported Rates, Tag length: 8
  /* 72 */ 0x82,                    // 1(B)
  /* 73 */ 0x84,                    // 2(B)
  /* 74 */ 0x8b,                    // 5.5(B)
  /* 75 */ 0x96,                    // 11(B)
  /* 76 */ 0x24,                    // 18
  /* 77 */ 0x30,                    // 24
  /* 78 */ 0x48,                    // 36
  /* 79 */ 0x6c,                    // 54
  // Current Channel
  /* 80 - 81 */ 0x03, 0x01,         // Channel set, length
  /* 82 */      0x01,               // Current Channel
  // RSN information
  /*  83 -  84 */ 0x30, 0x18,
  /*  85 -  86 */ 0x01, 0x00,
  /*  87 -  90 */ 0x00, 0x0f, 0xac, 0x02,
  /*  91 -  92 */ 0x02, 0x00,
  /*  93 - 100 */ 0x00, 0x0f, 0xac, 0x04, 0x00, 0x0f, 0xac, 0x04, /*Fix: changed 0x02(TKIP) to 0x04(CCMP) is default. WPA2 with TKIP not supported by many devices*/
  /* 101 - 102 */ 0x01, 0x00,
  /* 103 - 106 */ 0x00, 0x0f, 0xac, 0x02,
  /* 107 - 108 */ 0x00, 0x00
};



int pos = 0;
int numMenu = 0;

int keysPin = A0;
int keysValue;


// <---------------------------------------------------------------------------------->
void setup() {
  // FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS); 
  pinMode(4, OUTPUT);
  Serial.begin(9600);
  Serial.setTimeout(50);
  oled.init();    
  oled.setContrast(255); 




  for (int i = 0; i < 32; i++)
    emptySSID[i] = ' ';
  // for random generator
  randomSeed(os_random());
  packetSize = sizeof(beaconPacket);
  if (wpa2) {
    beaconPacket[34] = 0x31;
  } else {
    beaconPacket[34] = 0x21;
    packetSize -= 26;
  }

  randomMac();

  currentTime = millis();

  WiFi.mode(WIFI_OFF);
  wifi_set_opmode(STATION_MODE);

  wifi_set_channel(channels[0]);
  PrintMenu();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); 
}

void loop() {
  // PrintMenu();
  RollMenu();
  // WiFiSpammer();
}

void PrintMenu() {
  oled.clear();
  oled.home();
  oled.setScale(1);
  switch(numMenu){
    case 0: oled.setScale(2);oled.println(MainMenu[pos]); 
    oled.setScale(1);oled.setCursor(0,2);if (whiteLED){oled.println("white light");} else if(redLED){oled.println("red light");} else {oled.println("flashlight off");}
    break;
    case 1:
      if (pos == -1){
        pos = 0;
      } else if (pos == 3){
        pos = 2;
      }
      for(int i = 0; i < 3; i++){
        if(i == pos){
          oled.print(">");
        } else {
          oled.print(" ");
        }
        oled.println(FlashlightMenu[i]);
      }
      break;
    case 2:
      if (pos == -1){
        pos = 0;
      } else if (pos == 3){
        pos = 2;
      }
      for(int i = 0; i < 3; i++){
        if(i == pos){
          oled.print(">");
        } else {
          oled.print(" ");
        }
        oled.println(WiFiMenu[i]);
      }
      break;
  }
  oled.update();
}

void RollMenu(){
  keysValue = analogRead(keysPin);
  if (Serial.available() || keysValue != 1024) {
    String serData = Serial.readString();
    Serial.println(keysValue);
    
    if (serData == "down\n" || (keysValue > 105 && keysValue < 125)) {
      // Serial.print(serData);
      pos++;
      if(pos == MenusLen[numMenu]){
        pos = 0;
      }
    } else if(serData == "up\n" || (keysValue > 3 && keysValue < 40)) {
      pos--;
      if(pos == -1){
        pos = MenusLen[numMenu] - 1;
      }
    } else if(serData == "ok\n" || (keysValue > 50 && keysValue < 80)) {
      // {"LED"("white color", "red color", "back"),
      // "Wi-Fi"("Wi-Fi spammer", "Wi-Fi scanner", "back"),
      // "ARGB"(), 
      //"4", "5", "6", "7", "8", "9", "10"};
      if(numMenu == 0){
        numMenu = pos + 1;
        pos = 0;
      } else if(pos == MenusLen[numMenu] - 1){
        pos = numMenu - 1;
        numMenu = 0;
      } else if(numMenu == 1 && pos == 0){ //white led func
        whiteLED = !whiteLED;
        redLED = false;
        Serial.println(whiteLED);
        if(whiteLED) {
          leds[0] = CRGB::White;
          FastLED.show();
          digitalWrite(LED_BUILTIN, LOW); 
        } else {
          leds[0] = CRGB::Black;
          FastLED.show();
          digitalWrite(LED_BUILTIN, HIGH); 
        }
      } else if (numMenu == 1 && pos == 1){ //red led func
        redLED = !redLED;
        whiteLED = false;
        if(whiteLED) {
          leds[0] = CRGB::Red;
          FastLED.show();
        } else {
          leds[0] = CRGB::Black;
          FastLED.show();
        }
      // wi-fi
      } else if (numMenu == 2 && pos == 0) {
        
      }
    }
    PrintMenu();
  }  
}




void WiFispammer(){
  currentTime = millis();

  // send out SSIDs
  if (currentTime - attackTime > 100) {
    attackTime = currentTime;

    // temp variables
    int i = 0;
    int j = 0;
    int ssidNum = 1;
    char tmp;
    int ssidsLen = strlen_P(ssids);
    bool sent = false;

    // Go to next channel
    nextChannel();

    while (i < ssidsLen) {
      // Get the next SSID
      j = 0;
      do {
        tmp = pgm_read_byte(ssids + i + j);
        j++;
      } while (tmp != '\n' && j <= 32 && i + j < ssidsLen);

      uint8_t ssidLen = j - 1;

      // set MAC address
      macAddr[5] = ssidNum;
      ssidNum++;

      // write MAC address into beacon frame
      memcpy(&beaconPacket[10], macAddr, 6);
      memcpy(&beaconPacket[16], macAddr, 6);

      // reset SSID
      memcpy(&beaconPacket[38], emptySSID, 32);

      // write new SSID into beacon frame
      memcpy_P(&beaconPacket[38], &ssids[i], ssidLen);

      // set channel for beacon frame
      beaconPacket[82] = wifi_channel;

      // send packet
      if (appendSpaces) {
        for (int k = 0; k < 3; k++) {
          packetCounter += wifi_send_pkt_freedom(beaconPacket, packetSize, 0) == 0;
          delay(1);
        }
      }

      // remove spaces
      else {

        uint16_t tmpPacketSize = (packetSize - 32) + ssidLen; // calc size
        uint8_t* tmpPacket = new uint8_t[tmpPacketSize]; // create packet buffer
        memcpy(&tmpPacket[0], &beaconPacket[0], 38 + ssidLen); // copy first half of packet into buffer
        tmpPacket[37] = ssidLen; // update SSID length byte
        memcpy(&tmpPacket[38 + ssidLen], &beaconPacket[70], wpa2 ? 39 : 13); // copy second half of packet into buffer

        // send packet
        for (int k = 0; k < 3; k++) {
          packetCounter += wifi_send_pkt_freedom(tmpPacket, tmpPacketSize, 0) == 0;
          delay(1);
        }

        delete tmpPacket; // free memory of allocated buffer
      }

      i += j;
    }
  }

  // show packet-rate each second
  if (currentTime - packetRateTime > 1000) {
    packetRateTime = currentTime;
    Serial.print("Packets/s: ");
    Serial.println(packetCounter);
    packetCounter = 0;
  }
}

void nextChannel() {
  if (sizeof(channels) > 1) {
    uint8_t ch = channels[channelIndex];
    channelIndex++;
    if (channelIndex > sizeof(channels)) channelIndex = 0;

    if (ch != wifi_channel && ch >= 1 && ch <= 14) {
      wifi_channel = ch;
      wifi_set_channel(wifi_channel);
    }
  }
}

void randomMac() {
  for (int i = 0; i < 6; i++){
     macAddr[i] = random(256);
  }
}
