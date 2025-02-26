



const int analogPin = A0;  // Пин для сигнала осциллографа
const int sampleRate = 100; 
const int bufferSize = 128;
int buffer[bufferSize];


unsigned long previousMillis = 0; // таймер




void setup() {
  Serial.begin(9600);
}



void loop() {
  
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
