#define OLED_SOFT_BUFFER_64
#include <GyverOLED.h>
#include <FastLED.h>
#include <ESP8266WiFi.h>
#include "GyverButton.h"
#include <IRremote.h>


const int IR_LED_PIN = 3; // Пин для ИК-светодиода
IRsend irsend;


unsigned long powerCodes[] = {
    0x20DF10EF, // LG
    0xE0E040BF, // Samsung
    0xA90,      // Sony
    0x20DF8877, // Panasonic
    0x20DFD02F, // Philips
    0x20DF13EC, // Toshiba
    0x20DF1EE1  // Sharp
};

const int numCodes = sizeof(powerCodes) / sizeof(powerCodes[0]);
const long intervalIR = 500;
int currentCodeIndex = 0;
unsigned long previousMillisIR = 0;


GButton up;
GButton ok;
GButton down;
GyverOLED<SSD1306_128x64> oled;

#define LED_COUNT 1
#define DATA_PIN 3
CRGB leds[LED_COUNT];

int max_bright = 100;

int MenusLen[] = {11, 3, 3, 4, 4, 4, 4, 8, 4};
char* MainMenu[] = {"LED", "Wi-Fi", "IR", "Pins", "2.4", "termo", "sensors", "NFC", "RGB", "ARGB", "RFID"};
char* FlashlightMenu[] = {"white color", "red color", "back"}; //Flashlight white and red
char* WiFiMenu[] = {"Wi-Fi spammer", "Wi-Fi scanner", "back"}; //WiFi
char* IR[] = {"read", "emulate", "off device", "back"};
char* Pins[] = {"Pin 0", "Pin 1", "Pin 2", "back"}; //Pins
char* G2_4[] = {"2.4 jammer", "send pkg", "read pkg", "back"}; //2.4 GHz
char* termo[] = {"DHT11", "DHT22", "ds18b20", "back"};
char* sensors[] = {"pH", "rain sensor", "heartbit", "soil moisture", "HC-SR04", "CO2(MH-Z19b)", "Sound sensor", "back"};
char* NFC[] = {"read", "emulate", "write", "back"};
// char* RGB[] = {};
// char* ARGB[] = {};



bool whiteLED = false;
bool redLED = false;

bool oscilloscope = false;

bool WiFiSpam = false;

bool isoffTV = false;


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
  "Wi-Fi1\n"
  "Wi-Fi2\n"
  "Wi-Fi3\n"
  "Wi-Fi4\n"
  "Wi-Fi5\n"
  "Wi-Fi6\n"
  "Wi-Fi7\n"
  "Wi-Fi8\n"
  "Wi-Fi9\n"
  "Wi-Fi10\n"
  "Wi-Fi11\n"
  "Wi-Fi12\n"
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

  //buttons
  // up.setType(LOW_PULL);
  // ok.setType(LOW_PULL);
  // down.setType(LOW_PULL);

  up.setDebounce(50);
  ok.setDebounce(50);
  down.setDebounce(50);



  for (int i = 0; i < 32; i++)
    emptySSID[i] = ' ';
  randomSeed(os_random());
  packetSize = sizeof(beaconPacket);
  if (wpa2) {
    beaconPacket[34] = 0x31;
  } else {
    beaconPacket[34] = 0x21;
    packetSize -= 26;
  }

  // randomMac();

  DrawKentaiIcon();
  delay(500);

  currentTime = millis();

  WiFi.mode(WIFI_OFF);
  wifi_set_opmode(STATION_MODE);

  wifi_set_channel(channels[0]);
  PrintMenu();

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, HIGH); 

}

void loop() {
  // PinSelectMenu(0, 1);
  // PrintMenu();
  RollMenu();
  
  if (WiFiSpam){
    WiFispammer();
  } else if (isoffTV) {
    offTV();
  }
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
    case 3:
      if (pos == -1){
          pos = 0;
      } else if (pos == 4){
          pos = 3;
      }
        for(int i = 0; i < 4; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(IR[i]);
        }
      break;
    case 4:
      if (pos == -1){
          pos = 0;
      } else if (pos == 4){
          pos = 3;
      }
      for(int i = 0; i < 4; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(Pins[i]);
        }
      break;
    case 5:
      if (pos == -1){
          pos = 0;
      } else if (pos == 4){
          pos = 3;
      }
      for(int i = 0; i < 4; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(G2_4[i]);
        }
      break;
    case 6:
      if (pos == -1){
          pos = 0;
      } else if (pos == 4){
          pos = 3;
      }
      for(int i = 0; i < 4; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(termo[i]);
        }
      break;
    case 7:
      if (pos == -1){
          pos = 0;
      } else if (pos == 8){
          pos = 7;
      }
      for(int i = 0; i < 8; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(sensors[i]);
        }
      break;
    case 8:
      if (pos == -1){
          pos = 0;
      } else if (pos == 4){
          pos = 3;
      }
      for(int i = 0; i < 4; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(NFC[i]);
        }
      break;
    case 9:
      break;
    case 10:
      break;
    case 11:
      if (pos == -1){
          pos = 0;
      } else if (pos == 4){
          pos = 3;
      }
      for(int i = 0; i < 4; i++){
          if(i == pos){
            oled.print(">");
          } else {
            oled.print(" ");
          }
          oled.println(NFC[i]);
        }
      break;
  } 
  oled.update();
}

void RollMenu(){
  int analog = analogRead(keysPin);
  keysValue = analogRead(keysPin);

  up.tick(analog < 18 && analog > 0);
  ok.tick(analog < 100 && analog > 40);
  down.tick(analog < 150 && analog > 100);

  if (Serial.available() || analog != 1024) {
    String serData = Serial.readString();
    Serial.println(analog);
    // (keysValue > 105 && keysValue < 125) ||
    // (keysValue > 3 && keysValue < 40) || 
    // (keysValue > 40 && keysValue < 100) || 
    if (serData == "down\n" || down.isPress()) {
      // Serial.print(serData);
      pos++;
      if(pos == MenusLen[numMenu]){
        pos = 0;
      }
    } else if(serData == "up\n" || up.isPress()) {
      pos--;
      if(pos == -1){
        pos = MenusLen[numMenu] - 1;
      }
    } else if(serData == "ok\n" || ok.isPress()) {
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
      } else if(numMenu == 1){ //white led func
        if (pos == 0){
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
        } else if (pos == 1){ //red led func
          redLED = !redLED;
          whiteLED = false;
          if(whiteLED) {
            leds[0] = CRGB::Red;
            FastLED.show();
          } else {
            leds[0] = CRGB::Black;
            FastLED.show();
          }
        }
      } // wi-fi
       else if (numMenu == 2) {
        if (pos == 0) {
          WiFiSpam = !WiFiSpam;
        } else if (pos == 1){

        }
        
      } // IR remote
      else if (numMenu == 3 && pos == 0) { 
        if (pos == 0){

        } else if (pos == 1){

        } else if (pos == 2){
          isoffTV = !isoffTV;
        }
      } else if (numMenu == 4) { // Pins
        // if (pos == 0){

        // } else if (pos == 1){

        // } else if (pos == 2){

        // } else if (pos == 3){

        // }


      }
    } else if (serData != "") {
      readPKG(serData);
    }
    PrintMenu();
  }  
}


void readPKG(String incData) {
  if (incData.startsWith("OSC")) {
    Serial.println("done");
      // Разделение строки на части
      int firstSlash = incData.indexOf('/');
      int secondSlash = incData.indexOf('/', firstSlash + 1);
      int thirdSlash = incData.indexOf('/', secondSlash + 1);

      String XStr = incData.substring(firstSlash + 1, secondSlash);
      String Y1Str = incData.substring(secondSlash + 1, thirdSlash);
      String Y2Str = incData.substring(thirdSlash + 1);

      // Преобразование строк в числа
      int Xint = XStr.toInt();
      int Y1int = Y1Str.toInt();
      int Y2int = Y2Str.toInt();
      draw(Xint, Y1int, Y2int);
  }
}

void draw(int x, int y1, int y2) {
  oled.clear();
  if (oscilloscope){
    oled.line(x, y1, x + 1, y2);
  }
  oled.update();
}


void PinSelectMenu(int strPinNum, int SelectedPinNum){
  oled.clear();

  for(int i = 0; i < 2; i++){
    int startY = 26 + i * 7;
    for(int j = 0; j < 10; j++) {
      int startX = 30 + j * 7;
      if(j == SelectedPinNum + 3 && i == strPinNum) {
        oled.line(startX, startY, startX + 4, startY);
        oled.line(startX, startY + 1, startX + 1, startY + 1); oled.line(startX + 3, startY + 1, startX + 4, startY + 1);
        oled.line(startX, startY + 2, startX, startY + 2); oled.line(startX + 4, startY + 2, startX + 4, startY + 2);
        oled.line(startX, startY + 3, startX + 1, startY + 3); oled.line(startX + 3, startY + 3, startX + 4, startY + 3);
        oled.line(startX, startY + 4, startX + 4, startY + 4);
      } else {
        oled.line(startX + 2, startY + 1, startX + 2, startY + 3);
        oled.line(startX + 1, startY + 2, startX + 3, startY + 2);
      }
    } 
  }
  oled.update();
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


void DrawKentaiIcon() {
  oled.clear();
  oled.setScale(3);
  oled.setCursor(0,0);
  oled.print("Kentai");
  oled.update();
  oled.setScale(1);
}


void offTV() {
  unsigned long currentMillisIR = millis();


  if (currentMillisIR - previousMillisIR >= intervalIR) {
    previousMillisIR = currentMillisIR; 

    irsend.sendNEC(powerCodes[currentCodeIndex], 32);
    currentCodeIndex++;
    if (currentCodeIndex >= numCodes) {
      currentCodeIndex = 0; 
    }
  }
}

