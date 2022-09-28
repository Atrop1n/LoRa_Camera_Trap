/*********
  Based on tutorials from https://randomnerdtutorials.com
*********/
#include <SPI.h>

#include <LoRa.h>

#include "esp_camera.h"

#define PIR_PIN 3
//define LoRa pins
#define SS 15
#define RST -1
#define DIO0 2
#define SCK 14
#define MISO 12
#define MOSI 13
//define camera pins
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22

static int chunkSize = 250; //size of jpeg data in a single packet, max 250
static int redundancy = 4; //amount of times each packet is sent, higher number means more reliability, min 3
int movementStatus; 
int packetDelay = 100; //delay after sending a packet
long int bandwidth = 500000; 
int spreadFactor = 7; //7-12
unsigned long previousMillis = 0;
//formats the packet number
String addZeroes(String num) {
  if (num.length() == 1) {
    num = "000" + num;
  } else if (num.length() == 2) {
    num = "00" + num;
  } else if (num.length() == 3) {
    num = "0" + num;
  }
  return num;
}

void sendChunk(int packetNumber, camera_fb_t* fb) {
  uint8_t chunk[chunkSize];
  memcpy(chunk, (fb->buf) + (chunkSize * packetNumber), chunkSize); 
  Serial.println("Sending packet: " + String(packetNumber));
  for (int r = 0; r < redundancy; r++) {
    LoRa.beginPacket();
    for (int j = 0; j < chunkSize; j++) {
      LoRa.write(chunk[j]);  //sending the payload
    }
    LoRa.write(packetNumber);
    LoRa.endPacket();
  }
}

void waitForConfigPacket() {
  Serial.println("Waiting for config packet");
  previousMillis = millis();
  while (true) {
    unsigned long currentMillis = millis();
    if (currentMillis - previousMillis >= 10000) {
      previousMillis = currentMillis;
      Serial.println("Nothing arrived, moving on..");
      return;
    }
    int packetSize = LoRa.parsePacket();   // try to parse packet
    if (packetSize) {   // received a packet
      String loraData;
      while (LoRa.available()) {
        loraData = LoRa.readString();
      }
      bandwidth = loraData.substring(0, loraData.indexOf(':')).toInt(); //extract bandwidth from a config packet
      spreadFactor = loraData.substring(loraData.lastIndexOf(':')+1,loraData.length()).toInt(); //extract spread factor from a config packet
      LoRa.setSignalBandwidth(bandwidth);
      Serial.println("Bandwidth set to "+String(bandwidth));
      LoRa.setSpreadingFactor(spreadFactor);
      Serial.println("Spread factor set to "+String(spreadFactor));
      return;
    }
  }
}


void takePic(bool transmit) {
  if (transmit == false) { //takes a picture without transmission
    Serial.println("Taking pic ");
    camera_fb_t* fb = NULL;
    fb = esp_camera_fb_get(); 
    if (!fb) {
      Serial.println("Camera capture failed");
      Serial.println("Restarting");
      ESP.restart();
      return;
    }
    int size = fb->len; 
    Serial.println("Size of picture is: " + String(size) + " bytes");
    esp_camera_fb_return(fb);
    delay(1000);
  }
  if (transmit == true) { //takes and transmits a picture
    Serial.println("Taking and sending pic ");
    camera_fb_t* fb = NULL;
    fb = esp_camera_fb_get();  
    if (!fb) {
      Serial.println("Camera capture failed");
      Serial.println("Restarting");
      ESP.restart();
      return;
    }
    int size = fb->len;  //gets size of jpeg
    Serial.println("Size of picture is: " + String(size) + " bytes");
    int totalPackets = (size / chunkSize);
    Serial.println("Total number of packets to transmit: " + addZeroes(String(totalPackets + 1)));
    LoRa.beginPacket();
    LoRa.print(addZeroes(String(totalPackets + 1))); //sends initial packet
    LoRa.endPacket();
    delay(500);
    for (int i = 0; i < totalPackets; i++) { //sends all the chunks
      sendChunk(i, fb); 
      delay(packetDelay);
       if (i==0) //send initial packet twice, because it sometimes gets lost
      {
        sendChunk(i,fb);
        delay(packetDelay);
      }
    }
    //send remainder
    int remainder = size % chunkSize;
    uint8_t chunk[remainder];
    memcpy(chunk, (fb->buf) + (chunkSize * totalPackets), remainder);  
    Serial.println("Sending packet: " + String(totalPackets));
    for (int r = 0; r < redundancy; r++) {
      LoRa.beginPacket();
      for (int j = 0; j < remainder; j++) {
        LoRa.write(chunk[j]);  
      }
      for (int j = remainder; j < chunkSize; j++) {
        LoRa.write(255);
      }
      LoRa.write(totalPackets);
      LoRa.endPacket();
    }

    Serial.println("Finished transimssion");

    esp_camera_fb_return(fb);
  }
}

void setup() {
  Serial.begin(115200);
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_HD;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }
  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  sensor_t* s = esp_camera_sensor_get();

  SPI.begin(SCK, MISO, MOSI, SS);
  //LoRa init
  LoRa.setPins(SS,RST,DIO0);
  //replace the LoRa.begin(---E-) argument with your location's frequency
  //433E6 for Asia
  //866E6 for Europe
  //915E6 for North America
  while (!LoRa.begin(866E6)) {
    Serial.println(".");
    delay(500);
  }
  // Change sync word (0xF3) to match the receiver
  // The sync word assures you don't get LoRa messages from other LoRa transceivers
  // ranges from 0-0xFF
  LoRa.setSyncWord(0x6C);
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spreadFactor);
  Serial.println("LoRa Initializing OK!");
  // init PIR data pin
  pinMode(PIR_PIN, INPUT);
}

void loop() {
  movementStatus = digitalRead(PIR_PIN); 
  if(movementStatus == 1) //if movement detected
  {
  delay(1000);
  takePic(false); //take a few dummy pictures without transmission, to adjust exposure time and other parameters
  delay(2000);
  takePic(false);
  delay(2000);
  takePic(true); //take and transmit picture
  Serial.println("Finished taking picture");
  waitForConfigPacket(); 
  delay(2500);
  }
  Serial.println(movementStatus);
}
