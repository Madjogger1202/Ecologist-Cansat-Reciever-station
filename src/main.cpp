// Наземная станция для нашего аппарата
/*
 функции кода:        1) Приём телеметрии с нрф и лора модулей
                      2) Запись всего на сд карту
                      3) Скидываем всю телеметрию в последовательный порт
                      4) Всю информацию выводим на oled дисплей( или два дисплея)
                      5) Счётчик пакетов + рассчётная плотность пакетов по времени

*/

// todo: система контроля "свежести" пакетов
//       светодиодная индикация 
//       тестовые режимы работы для проверки исправности перед пуском
//       сделать отправку основной телеметрии и на комп через usb-ttl
//       создать схему-распиновку для подключения модулей 
//       добавить оповещение о большой разнице в количестве разных типов пакетов 

#include <Arduino.h>

#include <SPI.h>
#include <SD.h>

#include <nRF24L01.h>
#include <RF24.h>

#include <Wire.h>

#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <FastLED.h>

#include <LoRa.h>

#include <DFPlayer_Mini_Mp3.h>

#define LORA_D0 42
#define LORA_NSS 43
#define LORA_RST 44

#define NRF_CSN 40
#define NRF_CE 41

#define SD_CS 25

#define LED1 10
#define LED2 11

#define LED_PIN 45
#define NUM_LEDS    8
#define BRIGHTNESS  20
#define LED_TYPE    WS2811
#define COLOR_ORDER GRB
CRGB leds[NUM_LEDS];

#define UPDATES_PER_SECOND 100 

CRGBPalette16 currentPalette;
TBlendType    currentBlending;

RF24 radio(NRF_CE, NRF_CSN);

Adafruit_SSD1306 display(128, 32, &Wire, -1);



int8_t buttons[4] = { 31, 32, 33, 34};
int8_t mod_sw[8] = { A14, A13, A12, A11, A10, A9, A8, A7 };

uint8_t mode; // 0-256

struct sec_strctr
{
  uint8_t id = 0;
  uint8_t x_acs;    // 1 byte
  uint16_t z_acs;   // 2 byte
  uint16_t timeGPS; // 2 bytes
  uint16_t co2_ppm; // 2 bytes
  float temp;       // 4 bytes
  float pressure;   // 4 bytes
  float lonGPS;     // 4 bytes
  uint32_t counter; // 4 bytes
}radioData1, midData;

struct trd_strctr
{
  uint8_t id = 1;
  uint8_t y_acs;    // 1 byte
  uint16_t pm25_qw; // 2 bytes
  uint16_t tVOC;    // 2 bytes
  uint16_t rad_qw;  // 2 bytes
  float trsh;       // 2 bytes
  float lanGPS;     // 4 bytes
  float humidVal;   // 4 bytes
  uint32_t counter; // 4 bytes
}radioData2;

void writeSD();
void recieveNRF();
void recieveLoRa();
void displayInfo();
void displayLEDs();
void readMode();
void printSerial();
void startErr();
void noNrfDataErr();
void noLoRaDataErr();
void SDwriteErr();




void setup() 
{
  for(int i=0;i<8;i++)
  pinMode(mod_sw[i], INPUT_PULLUP);  
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  analogWrite(LED1, 10);
  analogWrite(LED2, 100);

  pinMode(LORA_NSS, OUTPUT);
  pinMode(LORA_RST, OUTPUT);
  pinMode(NRF_CSN, OUTPUT);
  pinMode(NRF_CE, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  
  pinMode(LED_PIN, OUTPUT);

  Serial.begin(115200);
  SPI.begin();                                               // инициализируем работу с SPI
  SPI.setDataMode(SPI_MODE3);   
  
  if (!SD.begin(SD_CS)) {
			Serial.println("Card Failure");
      startErr();
	}
  else
    Serial.println("Card Ready");
  

  if (!LoRa.begin(433E6)) 
  {
    Serial.println("Starting LoRa failed!");
    startErr();
  }
  else
    Serial.println("LoRa started without errors");
  
  mp3_set_serial(Serial2);
  mp3_set_volume(25);
  mp3_play (1);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) 
  { 
    Serial.println(F("SSD1306 allocation failed"));
    startErr();
  }
  else
    Serial.println("Display started sucsessfully");

  display.display();
  display.clearDisplay();
  
  display.display();
  if(!radio.begin())
  {
      Serial.println("nrf err");
      startErr();
  }
  else
    Serial.println("NRF started without errors");
  radio.setChannel(100);                               
  radio.setDataRate     (RF24_250KBPS);                   
  radio.setPALevel      (RF24_PA_HIGH);                 
  radio.openWritingPipe (0x1231163366LL);               
  radio.setAutoAck(false);
  delay(2000);
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS).setCorrection( TypicalLEDStrip );
    FastLED.setBrightness(  BRIGHTNESS );
  
}

void loop() 
{
  recieveNRF();
  recieveLoRa();
  readMode();
  displayInfo();
  displayLEDs();
}

void writeSD()
{
    File allData = SD.open("Eco.csv", FILE_WRITE);
    if(!allData)
    {
      SDwriteErr();
    }
   else if (allData)
    {
    
      allData.print(radioData1.x_acs);
      allData.print(";");
      allData.print(radioData2.y_acs);
      allData.print(";");
      allData.print(radioData1.z_acs);
      allData.print(";");
      allData.print(radioData1.temp);
      allData.print(";");
      allData.print(radioData1.pressure);
      allData.print(";");
      allData.print(radioData1.lonGPS);
      allData.print(";");
      allData.print(radioData2.lanGPS);
      allData.print(";");
      allData.print(radioData1.timeGPS);
      allData.print(";");
      allData.print(radioData1.co2_ppm);
      allData.print(";");
      allData.print(radioData2.pm25_qw);
      allData.print(";");
      allData.print(radioData2.tVOC);
      allData.print(";");
      allData.print(radioData2.rad_qw);
      allData.print(";");
      allData.print(radioData2.humidVal);
      allData.print(";");
      allData.print(radioData1.counter);
      allData.print(";");
      allData.print(radioData2.counter);
      allData.print(";");
      allData.println(millis()/100);
      allData.close();
    }
}
void recieveNRF()
{
  if(radio.available())
  {
    radio.read(&midData, sizeof(midData));
    if(midData.id == 0)
    {
      radioData1 = midData;
    }
    else if(midData.id == 1)
    {
      radioData2.y_acs = midData.x_acs;
      radioData2.pm25_qw = midData.z_acs;
      radioData2.tVOC = midData.timeGPS;
      radioData2.rad_qw = midData.co2_ppm;
      radioData2.trsh = midData.temp;
      radioData2.lanGPS = midData.pressure;
      radioData2.humidVal = midData.lonGPS;
      radioData2.counter = midData.counter;
      writeSD();
    }
  }
}

void recieveLoRa()
{
  int packetSize = LoRa.parsePacket();
  if (packetSize) 
  {
    while (LoRa.available()) 
    {
      Serial.print((char)LoRa.read());
    }
  }
}

void displayInfo()
{
  display.clearDisplay();
  display.setTextSize(1);             
  display.setTextColor(WHITE);       
  display.setCursor(0, 0);            
  display.println(radioData1.lonGPS, 7);            
  display.println(radioData2.lanGPS);
  display.print(radioData1.temp);      
  display.print("  ");  
  display.println(radioData1.pressure);
  display.print(radioData1.counter);
  display.display();
}



void displayLEDs()
{
  // todo - система индикации через светодиодную ленту
}


void readMode()
{
  bitWrite(mode, 0, (!digitalRead(A14)));
  bitWrite(mode, 1, (!digitalRead(A13)));
  bitWrite(mode, 2, (!digitalRead(A12)));
  bitWrite(mode, 3, (!digitalRead(A11)));
  bitWrite(mode, 4, (!digitalRead(A10)));
  bitWrite(mode, 5, (!digitalRead(A9)));
  bitWrite(mode, 6, (!digitalRead(A8)));
  bitWrite(mode, 7, (!digitalRead(A7)));
}

void printSerial()
{
  // todo - решить что лучше слать на комп
}

void startErr()
{
  mp3_play(9);
  Serial.println("startErr, need reboot");
}

void noNrfDataErr()
{
  mp3_play(10);
  Serial.println("no packages from NRF for a long time");
}

void noLoRaDataErr()
{
  mp3_play(11);
  Serial.println("no packages from LoRa for a long time");
}

void SDwriteErr()
{
  mp3_play(12);
  Serial.println("Can't open SD for writing");
}


