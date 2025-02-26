#include <SPI.h>
#include <Wire.h>

#include <RF24.h>
#include <nRF24L01.h>
#include "printf.h"

#define CE  7
RF24 radio(7, 8);

#define CHANNELS  64
int channel[CHANNELS];


int  line;
char grey[] = " .:-=+*aRW";

#define _NRF24_CONFIG      0x00
#define _NRF24_EN_AA       0x01
#define _NRF24_RF_CH       0x05
#define _NRF24_RF_SETUP    0x06
#define _NRF24_RPD         0x09


byte count;
byte sensorArray[128];
byte drawHeight;

char filled = 'F'; 
char drawDirection = 'R'; 
char slope = 'W'; 

const uint8_t num_channels = 64;
int values[num_channels];
int valuesDisplay[32];
int channels = 0;
const byte address[6] = "00001";
const int num_reps = 50;
bool jamming = false;

byte getRegister(byte r)
{
 byte c;
 
 PORTB &=~_BV(2);
 c = SPI.transfer(r&0x1F);
 c = SPI.transfer(0);  
 PORTB |= _BV(2);

 return(c);
}


const int analogPin = A0;  // Пин для сигнала осциллографа
const int sampleRate = 100; 
const int bufferSize = 128;
int buffer[bufferSize];


unsigned long previousMillis = 0; // таймер




void setup() {
  Serial.begin(9600);

  radio.begin();
  radio.startListening();
  radio.stopListening();

  Serial.println("Channel Layout");
  printChannels();
  
  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  SPI.setBitOrder(MSBFIRST);
  
  pinMode(CE,OUTPUT);
  disable();
  
  powerUp();
  
  setRegister(_NRF24_EN_AA,0x0);

  setRegister(_NRF24_RF_SETUP,0x0F);
}



void loop() {
  

  if (jamming){
    radio.setPALevel(RF24_PA_HIGH);
    radio.setDataRate(RF24_2MBPS);
    jammer();
  }
}

void oscill(){
  unsigned long currentMillis = millis();

  static int bufferIndex = 0;
  if (currentMillis - previousMillis >= sampleRate) {
    previousMillis = currentMillis;

    buffer[bufferIndex] = analogRead(analogPin);
    bufferIndex++;

    if (bufferIndex >= bufferSize) {
      bufferIndex = 0;
      
      for (int i = 0; i < bufferSize - 1; i++) {
          int y1 = map(buffer[i], 0, 1023, SCREEN_HEIGHT - 1, 0);
          int y2 = map(buffer[i + 1], 0, 1023, SCREEN_HEIGHT - 1, 0);

          String data = "OSC/" + String(i) + "/" + String(y1) + "/" + String(y2);
          Serial.println(data); //отправка сигналов для отрисовки
        }
    }
}


void setRegister(byte r, byte v)
{
 PORTB &=~_BV(2);
 SPI.transfer((r&0x1F)|0x20);
 SPI.transfer(v);
 PORTB |= _BV(2);
}
 
void powerUp(void)
{
 setRegister(_NRF24_CONFIG,getRegister(_NRF24_CONFIG)|0x02);
 delayMicroseconds(130);
}

void powerDown(void)
{
 setRegister(_NRF24_CONFIG,getRegister(_NRF24_CONFIG)&~0x02);
}

void enable(void)
{
   PORTB |= _BV(1);
}

void disable(void)
{
   PORTB &=~_BV(1);
}

void setRX(void)
{
 setRegister(_NRF24_CONFIG,getRegister(_NRF24_CONFIG)|0x01);
 enable();

 delayMicroseconds(100);
}

void scanChannels(void)
{
 disable();
 for( int j=0 ; j<255  ; j++)
 {
   for( int i=0 ; i< channels; i++)
   {
     setRegister(_NRF24_RF_CH,(128*i)/ channels);
     
     setRX();
     
     delayMicroseconds(40);
     
     disable();
     
     if( getRegister(_NRF24_RPD)>0 )   channel[i]++;
   }
 }
}

void printChannels(void)
{
 Serial.println(">      1 2  3 4  5  6 7 8  9 10 11 12 13  14                     <");
}


void jammer() {
  const char text[] = "xxxxxxxxxxxxxxxx"; // send the noise
  for (int i = ((channels * 5) + 1); i < ((channels * 5) + 23); i++) {
    radio.setChannel(i);
    radio.write( & text, sizeof(text));
  }
 }




