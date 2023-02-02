/*********
  Based on tutorials from https://randomnerdtutorials.com
*********/
#include <SPI.h>
#include <RadioLib.h>
#include "esp_camera.h"

#define SS 15
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
#define PIR 1

SX1280 radio = new Module(15, 2, 4, 3);
static int chunkSize = 250; //size of jpeg data in a single packet, max 250
int packetDelay = 10; //delay after sending a packet
unsigned long startMillis = 0;
unsigned long currentMillis = 0;
int packet_rate = 0;
int packet_number_byte = 0;
byte total_packets_array[2];

void sendChunk(int packetNumber, camera_fb_t* fb) {
  uint8_t chunk[chunkSize+4];
  memcpy(chunk, (fb->buf) + (chunkSize * (packetNumber-1)), chunkSize); 
  
  Serial.println("Sending packet: " + String(packetNumber)+"/"+String(total_packets_array[0] + 256 * total_packets_array[1]));
  chunk[chunkSize+1]=packetNumber;
  chunk[chunkSize+2]=packet_number_byte;
  chunk[chunkSize+3]=total_packets_array[0];
  chunk[chunkSize+4]=total_packets_array[1];


  for (int i =0;i<=chunkSize+4;i++)
    {
      Serial.print(chunk[i]);
    }
    Serial.println();
    int state = radio.transmit(chunk,chunkSize+5);
  packet_rate++;
      currentMillis = millis(); //calculating packet rate/10 s
      if (currentMillis - startMillis >= 10000)  
        {
          startMillis = currentMillis; 
          Serial.println("Packet rate: "+String(packet_rate)+" packets/10 seconds");
          packet_rate = 0;
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
    int totalPackets = (size / chunkSize)+1;
    Serial.println("Total number of packets to transmit: " + String(totalPackets));
    
    total_packets_array[0] = totalPackets%256;
    total_packets_array[1] = totalPackets/256;

    delay(500);
    int i = 1;
    while (i <= totalPackets) { //sends all the chunks
    if (i%256==0 && i!=0)
    {
      packet_number_byte++;
    }
      sendChunk(i, fb); 
      while(true)
      {
        String packet_confirmation = "";
        int state = radio.receive(packet_confirmation);
        if (state == RADIOLIB_ERR_NONE) {
        // packet was successfully received
          if (packet_confirmation!=String(i))
          {
            Serial.println("Expected packet "+String(i)+", got confirmation for packet "+packet_confirmation+". Sending packet "+String(i)+" again. \nHopefully it is the same as "+String(packet_confirmation.toInt()+1)+"...");
            i=packet_confirmation.toInt();
          }
          else{
            Serial.println(packet_confirmation+" OK!");
          }
          break;
        }
        else {
          Serial.println("Radiolib error");
        }
      }
      i++;
    }
    
   
    Serial.println("Finished transimssion");
    packet_number_byte = 0;
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
  Serial.print(F("[SX1280] Initializing ... "));
  int state = radio.begin();
  //int state = radio.begin(2400.0, 1625.0 , 9, 7, 13, 12);
  if (state == RADIOLIB_ERR_NONE) {
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  Serial.println("LoRa Initializing OK!");
  // init PIR data pin
  startMillis = millis(); 
  pinMode(PIR, INPUT);
}

void loop() {
  int movement_status = digitalRead(PIR);
  Serial.println(String(movement_status));
  if (movement_status == 1)
  {
  delay(1000);
  takePic(false); //take a few dummy pictures without transmission, to adjust exposure time and other parameters
  delay(2000);
  takePic(false);
  delay(2000);
  takePic(true); //take and transmit picture
  Serial.println("Finished taking picture"); 
  delay(10000);
  }
  delay(1000);
}
