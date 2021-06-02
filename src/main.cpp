// Наземная станция для нашего аппарата
#include <SPI.h>
#include <SD.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include "nRF24L01.h"
#include "RF24.h"
#include "printf.h"
#define SCREEN_WIDTH 128

#define OLED_DC     25
#define OLED_CS     31
#define OLED_RESET  29
Adafruit_SSD1306 displayBig(SCREEN_WIDTH, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);
Adafruit_SSD1306 displaySmall(SCREEN_WIDTH, 32, &Wire, -1);

RF24 radio(5, 6);
const uint8_t num_channels = 126;
uint8_t values[num_channels];


struct frstSrt
{
  uint8_t id = 0;   // 1 byte
  int16_t x_acs;    // 2 bytes
  int16_t y_acs;    // 2 bytes
  int16_t z_acs;    // 2 bytes
  int16_t trsh1;    // 2 bytes
  float trsh2;
  float trsh3;
  float trsh4;
  float presssure;  // 4 bytes
  uint32_t counter; // 4 bytes
} fastData, midData;

struct secStr
{
  uint8_t id = 1;   // 1 byte
  int16_t pm25;    // 2 bytes
  int16_t tVOC;    // 2 bytes
  int16_t rad_qw;  // 2 bytes
  int16_t co2_ppm; // 2 bytes
  float lanGPS;     // 4 bytes
  float lonGPS;     // 4 bytes
  float humidVal;   // 4 bytes
  float temp;       // 4 bytes
  uint32_t counter; // 4 bytes
} slowData;           // 29 bytes: 29 / 250Kbps = 0.9ms - max max delay (in theory)


File myFile;

void setup() {
  Serial.begin(115200);
  if (!displayBig.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  if (!displaySmall.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for (;;); // Don't proceed, loop forever
  }
  SPI.begin();                                               // инициализируем работу с SPI
  SPI.setDataMode(SPI_MODE3);
  Serial.begin(115200);
  printf_begin();
  LoRa.setPins(4, 3, 2);

  Serial.println("LoRa Receiver");

  if (!LoRa.begin(433E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  radio.begin();
  radio.setChannel(120);                                // Указываем канал приёма данных (от 0 до 127), 5 - значит приём данных осуществляется на частоте 2,405 ГГц (на одном канале может быть только 1 приёмник и до 6 передатчиков)
  radio.setDataRate     (RF24_250KBPS);                   // Указываем скорость передачи данных (RF24_250KBPS, RF24_1MBPS, RF24_2MBPS), RF24_1MBPS - 1Мбит/сек
  radio.setPALevel      (RF24_PA_MAX);                 // Указываем мощность передатчика (RF24_PA_MIN=-18dBm, RF24_PA_LOW=-12dBm, RF24_PA_HIGH=-6dBm, RF24_PA_MAX=0dBm)
  radio.openReadingPipe (0, 0x1234567899LL);            // Открываем 1 трубу с идентификатором 0x1234567890 для приема данных (на ожном канале может быть открыто до 6 разных труб, которые должны отличаться только последним байтом идентификатора)
  radio.setAutoAck(false);
  radio.startListening  ();
  radio.printDetails();
  Serial.print("Initializing SD card...");

  if (!SD.begin(7)) {
    Serial.println("initialization failed!");
    return;
  }
  Serial.println("initialization done.");

  myFile = SD.open("test.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to test.txt...");
    myFile.println("hello there!");
    // close the file:
    myFile.close();
    Serial.println("done.");
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }

  // re-open the file for reading:
  myFile = SD.open("test.txt");
  if (myFile) {
    Serial.println("test.txt:");

    // read from the file until there's nothing else in it:
    while (myFile.available()) {
      Serial.write(myFile.read());
    }
    // close the file:
    myFile.close();
  } else {
    // if the file didn't open, print an error:
    Serial.println("error opening test.txt");
  }
    displayBig.clearDisplay();
    displayBig.setTextSize(2);      // 2:1 pixel scale
    displayBig.setTextColor(WHITE);
    displayBig.setCursor(0, 0);
    displayBig.cp437(true);
    
    displayBig.println(fastData.presssure, 2);
    displayBig.print(fastData.x_acs);
    displayBig.print(" ");
    displayBig.print(fastData.y_acs);
    displayBig.print(" ");
    displayBig.println(fastData.z_acs);
    displayBig.println(fastData.counter);
    displayBig.println(slowData.temp);
    displaySmall.clearDisplay();
    displaySmall.setTextSize(1);      // Normal 1:1 pixel scale
    displaySmall.setTextColor(WHITE);
    displaySmall.setCursor(0, 0);
    displaySmall.cp437(true);
    displaySmall.println(slowData.lanGPS, 7);
    displaySmall.println(slowData.lonGPS, 7);
    displaySmall.print(slowData.pm25);
    displaySmall.print(" ");
    displaySmall.print(slowData.tVOC);
    displaySmall.print(" ");
    displaySmall.println(slowData.humidVal);
    displaySmall.print(slowData.rad_qw);
    displaySmall.print(" ");
    displaySmall.print(slowData.co2_ppm);
    displaySmall.display();
    displayBig.display();
  delay(300);

}


void loop()
{
  if (radio.available())
  {
    radio.read(&midData, sizeof(midData));
    if(midData.id == 0)
    {
      fastData=midData;
      Serial.print(millis());
      Serial.print(",");
      Serial.print(fastData.x_acs);
      Serial.print(",");
      Serial.print(fastData.y_acs);
      Serial.print(",");
      Serial.print(fastData.z_acs);
      Serial.print(",");
      Serial.print(fastData.presssure);
      Serial.print(",");
      Serial.println(fastData.counter);
      myFile = SD.open("its_wednesday_dd.csv", FILE_WRITE);

      // if the file opened okay, write to it:
      if (myFile) 
      {
        myFile.print(millis());
        myFile.print(",");
        myFile.print(fastData.x_acs);
        myFile.print(",");
        myFile.print(fastData.y_acs);
        myFile.print(",");
        myFile.print(fastData.z_acs);
        myFile.print(",");
        myFile.print(fastData.presssure);
        myFile.print(",");
        myFile.println(fastData.counter);
        myFile.close();
      }
    }
    else
    {
      slowData.pm25 = midData.x_acs;
      slowData.tVOC = midData.y_acs;
      slowData.rad_qw = midData.z_acs;
      slowData.co2_ppm = midData.trsh1;
      slowData.lanGPS = midData.trsh2;
      slowData.lonGPS = midData.trsh3;
      slowData.humidVal = midData.trsh4;
      slowData.temp = midData.presssure;
      slowData.counter = midData.counter;
      Serial.print(millis());
      Serial.print(",");
      Serial.print(",");
      Serial.print(",");
      Serial.print(",");
      Serial.print(",");
      Serial.print(slowData.counter);
      Serial.print(",");
      Serial.print(slowData.pm25);
      Serial.print(",");
      Serial.print(slowData.tVOC);
      Serial.print(",");
      Serial.print(slowData.rad_qw);
      Serial.print(",");
      Serial.print(slowData.co2_ppm);
      Serial.print(",");
      Serial.print(slowData.humidVal);
      Serial.print(",");
      Serial.print(slowData.temp);
      Serial.print(",");
      Serial.print(slowData.lanGPS, 7);
      Serial.print(",");
      Serial.println(slowData.lonGPS, 7);
      myFile = SD.open("its_wednesday_dd_bt_sloww.csv", FILE_WRITE);

      // if the file opened okay, write to it:
      if (myFile) 
      {
        myFile.print(millis());
        myFile.print(",");
        myFile.print(",");
        myFile.print(",");
        myFile.print(",");
        myFile.print(",");
        myFile.print(slowData.counter);
        myFile.print(",");
        myFile.print(slowData.pm25);
        myFile.print(",");
        myFile.print(slowData.tVOC);
        myFile.print(",");
        myFile.print(slowData.rad_qw);
        myFile.print(",");
        myFile.print(slowData.co2_ppm);
        myFile.print(",");
        myFile.print(slowData.humidVal);
        myFile.print(",");
        myFile.print(slowData.temp);
        myFile.print(",");
        myFile.print(slowData.lanGPS, 7);
        myFile.print(",");
        myFile.println(slowData.lonGPS, 7);
        myFile.close();
      }
      
    } 
    displayBig.clearDisplay();
    displayBig.setTextSize(2);      // 2:1 pixel scale
    displayBig.setTextColor(WHITE);
    displayBig.setCursor(0, 0);
    displayBig.cp437(true);
    
    displayBig.println(fastData.presssure, 2);
    displayBig.print(fastData.x_acs);
    displayBig.print(" ");
    displayBig.print(fastData.y_acs);
    displayBig.print(" ");
    displayBig.println(fastData.z_acs);
    displayBig.println(fastData.counter);
    displayBig.println(slowData.temp);
    displaySmall.clearDisplay();
    displaySmall.setTextSize(1);      // Normal 1:1 pixel scale
    displaySmall.setTextColor(WHITE);
    displaySmall.setCursor(0, 0);
    displaySmall.cp437(true);
    displaySmall.println(slowData.lanGPS, 7);
    displaySmall.println(slowData.lonGPS, 7);
    displaySmall.print(slowData.pm25);
    displaySmall.print(" ");
    displaySmall.print(slowData.tVOC);
    displaySmall.print(" ");
    displaySmall.println(slowData.humidVal);
    displaySmall.print(slowData.rad_qw);
    displaySmall.print(" ");
    displaySmall.print(slowData.co2_ppm);
    displaySmall.display();
    displayBig.display();
  }
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // received a packet
    Serial.print("Received packet '");

    // read packet
    while (LoRa.available()) {
      Serial.print((char)LoRa.read());
    }

    // print RSSI of packet
    Serial.print("' with RSSI ");
    Serial.println(LoRa.packetRssi());
  }
}

