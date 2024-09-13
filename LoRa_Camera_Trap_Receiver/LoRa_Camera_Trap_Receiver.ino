/*********
  Based on tutorials from https://randomnerdtutorials.com
*********/
#include <SPI.h>

#include <SPIFFS.h>

#include <SD.h>

#include <LoRa.h>

#include <WiFi.h>

#include <LiquidCrystal_I2C.h>

#include <ESPAsyncWebServer.h>

#include <EEPROM.h>

#include <time.h>
// pins used by LoRa module
#define SS 15
#define RST 4
#define DIO0 2
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_SS 15

#define FILE_PHOTO "/photo.jpg"  //path to the image in SPIFFS
#define EEPROM_SIZE 1

int EEPROM_position = 0;

const char* ssid = "";
const char* password = "";

AsyncWebServer server(80);

uint16_t packet_number;
uint16_t total_packets;
const byte chunk_size = 250;
byte chunk[chunk_size];
byte current_packet_array[2];
byte lcd_columns = 16;
byte lcd_rows = 2;
LiquidCrystal_I2C lcd(0x27, lcd_columns, lcd_rows);
File file;
File photo;
int EEPROM_count = 0;
int bandwidth = 500000;
byte spread_factor = 7;
String rssi;
String snr;
String path;
String timestamp;
int ack_retransmission_period = 250;
unsigned long start_millis;
unsigned long current_millis;
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
   <meta name="viewport" content="width=device-width, initial-scale=1">
   <style>
      body {
         text-align: center;
         background-color: rgb(194, 212, 76);
         font-family: Verdana, Geneva, Tahoma, sans-serif;
         color: rgb(33, 27, 25);
      }
      .centered {
         text-align: center;
      }
      .indented {
         margin-right: 70px;
      }
      div.rssi {
         position: relative;
         left: 0;
         right: 0;
         font-size: medium;
         /* background-color: aquamarine; */
      }
      h2 {
         font-weight: 600;
         font-size: 32px;
         text-shadow: 1px 1px 1px rgb(223, 237, 76);
      }
      .p {
         padding: 0px;
         margin: 5px;
         /* background-color: blue; */
      }
      .file_name {
         padding: 0px;
         margin: 5px;
      }
      .title {
         margin: 5px;
      }
      img {
         max-width: 100%%;
         max-height: 100%%;
         width: 768px;
         height: 432px;
      }
      .sidebar {
         float: left;
         top: 104px;
         max-width: 17%%;
         position: absolute;
         left: 22px;
      }
      .options {
         padding: 3px;
         box-shadow: 1px 1px 5px rgb(0, 0, 0);
         border-radius: 1px;
      }
      .picture-holder {
         margin-top: 5px;
         margin-bottom: 5px;
         display: inline-block;
         position: relative;
         width: 768px;
         height: 432px;
         max-width: 100%%;
         border-radius: 1px;
         box-shadow: 1px 1px 12px rgb(0, 0, 0);
      }
      .dropdown-menu{
         width: 60px;
      }
      @media (max-width: 767px) {
         .rssi {
            display: block;
            left: auto;
            right: auto;
            text-align: center;
         }
         .indented {
            margin-right: auto;
            text-align: center;
            display: block;
         }
         .snr {
            text-align: center;
            display: block;
         }
         .p {
            padding: 0px;
            margin: auto;
         }
         .sidebar {
            position: relative;
            margin: auto;
            max-width: none;
            top: auto;
            margin-top: 8px;
            float: none;
            left: auto;
         }
         .options{
           display: inline-block
         }
         .picture-holder {
            height: auto;
            width: auto;
            border: none;
            box-shadow: none;
            margin-bottom: auto;
         }
         img {
            border: solid rgb(0, 0, 0);
            box-shadow: 1px 1px 6px rgb(0, 0, 0);
            max-width: 100%%;
            width: auto;
            height: auto;
         }
      }
   </style>
</head>
<body>
   <h2 class="title">LoRa camera trap</h2>
   <p class="file_name" id="file_name">Last photo <strong><span id="path">%PATH%</span></strong> was captured at
      <strong></strong><span id="timestamp" style="font-weight: 600;">%TIMESTAMP%</span></strong>
   </p>
   <p class="p"></p>
   <p class="p">Received packets: <span id="string_num">%STRING_NUM%</span>/<span
         id="total_packets">%TOTAL_PACKETS%</span></p>
         <div class="picture-holder">
   <img src="saved-photo" id="photo">
        </div>
   <div class="rssi"><span class="indented">Last packet RSSI: <strong><span
               id="rssi">%RSSI%</span></strong>&nbspdBm</span><span class="snr">Last packet SNR: <strong> <span
               id="snr"> %SNR%</span></strong> dB</span></div>
  <div class="sidebar">
      <table class="options">
         <tbody>
         <tr class="parameter">
            <td>
            <label for="bandwidth">Bandwidth (kHz)</label></td>
            <td>
            <select onchange="getBandwidth(), retain()" name="bandwidth" id="bandwidth" class="dropdown-menu">
               <option value="7800">7.8</option>
               <option value="10400">10.4</option>
               <option value="15600">15.6</option>
               <option value="20800">20.8</option>
               <option value="31250">31.25</option>
               <option value="41700">41.7</option>
               <option value="62500">62.5</option>
               <option value="125000">125</option>
               <option value="250000">250</option>
               <option value="500000">500</option>
            </select>
         </td>
         </tr>
            <tr class="parameter">
               <td style="text-align: left;"><label for="spread_factor">Spread factor</label></td>
               <td>
               <select onchange="getspread_factor(), retain()" name="spread_factor" id="spread_factor" class="dropdown-menu">
                  <option value="7"  selected>7</option>
                  <option value="8">8</option>
                  <option value="9">9</option>
                  <option value="10">10</option>
                  <option value="11">11</option>
                  <option value="12">12</option>               
               </select>
            </td>
            </tr>
         </tbody>
         </table>
         <p id="demo" style="margin-top:5px"></p>
      </div>
   <script>
      setInterval(updateValues, 5000, "rssi");
      setInterval(updateValues, 5000, "total_packets");
      setInterval(updateValues, 5000, "string_num");
      setInterval(updateValues, 5000, "snr");
      setInterval(updateValues, 5000, "path");
      setInterval(updateValues, 5000, "timestamp");

      function updateValues(value) {
         var xhttp = new XMLHttpRequest();
         xhttp.onreadystatechange = function () {
            if (this.readyState == 4 && this.status == 200) {
               document.getElementById(value).innerHTML = this.responseText;
            }
         };
         xhttp.open("GET", "/" + value, true);
         xhttp.send();
      }
      function getBandwidth(value) {
         var xhttp = new XMLHttpRequest();
         var x = document.getElementById("bandwidth").value;
         document.getElementById("demo").innerHTML = "Bandwidth set to " + x;
         xhttp.open("GET", "/update?bandwidth="+x, true);
         xhttp.send();
      }
       function getspread_factor(value) {
         var xhttp = new XMLHttpRequest();
         var x = document.getElementById("spread_factor").value;
         document.getElementById("demo").innerHTML = "Spread factor set to " + x;
         xhttp.open("GET", "/update?spread_factor="+ x, true);
         xhttp.send();
      }
       function retain(value) {
         sessionStorage.setItem("sf", document.getElementById("spread_factor").value);
         sessionStorage.setItem("bw", document.getElementById("bandwidth").value);
      }
      window.onload = function() {
         {
            document.getElementById('spread_factor').value=sessionStorage.getItem("sf");
            document.getElementById('bandwidth').value=sessionStorage.getItem("bw");
         }
      };
   </script>
</body>
</html>)rawliteral";
//this will process periodic value updates on the webpage
String processor(const String& var) {
  if (var == "RSSI") {
    return String(rssi);
  } else if (var == "SNR") {
    return String(snr);
  } else if (var == "STRING_NUM") {
    return String(packet_number);
  } else if (var == "TOTAL_PACKETS") {
    return String(total_packets);
  } else if (var == "PATH") {
    return String(path);
  } else if (var == "TIMESTAMP") {
    return timestamp;
  }
  return String();
}


// when a packet indicating a new photo arrives
void new_photo() {
  if (EEPROM_count == 1000) {
    EEPROM_count = 0;
    EEPROM.begin(EEPROM_SIZE);
    EEPROM_position = EEPROM.read(0) + 1;
    EEPROM.write(0, EEPROM_position);
    EEPROM.commit();
  }
  path = "/picture" + String(EEPROM_position) + "_" + String(EEPROM_count) + ".jpg";  //path where image will be stored on SD card
  EEPROM_count++;
  Serial.printf("Picture file name: %s\n", path.c_str());
  photo = SD.open(path.c_str(), FILE_WRITE);  //save photo to SD card
  if (!photo) {
    Serial.println("Failed to open photo in writing mode");
  }
  file = SPIFFS.open(FILE_PHOTO, FILE_WRITE);  //save current photo to SPIFFS
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
}
//when a photo is fully transmitted
void end_photo() {
  file.close();
  if (!photo) {
    Serial.println("Couldn't save to SD card");
  } else {
    photo.close();
    Serial.println("Picture saved to SD card");
  }
}
void send_ack(const byte* packet_number_array) { 
  Serial.println("Sending ack for packet " + String(packet_number_array[1] << 8 | packet_number_array[0]));
  LoRa.beginPacket();
  LoRa.write(packet_number_array[0]);  //write packet number as two byte number
  LoRa.write(packet_number_array[1]);
  LoRa.endPacket();
}
//updates LCD values
void lcd_update(String string_num, String total_packets, int rssi, float snr) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(string_num);
  lcd.setCursor(4, 0);
  lcd.print("/" + total_packets);
  lcd.setCursor(0, 1);
  lcd.print("RSSI:");
  lcd.setCursor(5, 1);
  lcd.print(rssi);
  lcd.setCursor(10, 1);
  lcd.print(snr);
}


//updates timestamp
void get_time() {
  time_t now = time(NULL);
  struct tm tm_now;
  localtime_r(&now, &tm_now);
  char buff[100];
  strftime(buff, sizeof(buff), "%Y-%m-%d  %H:%M:%S", &tm_now);
  timestamp = buff;
  Serial.println(timestamp);
}
//if creating soft AP
void soft_AP() {
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}
//if connecting to a network
void wifi_connect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  Serial.begin(115200);

  //soft_AP();
  wifi_connect();
  server.begin();

  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  //HSPI is used by LoRa, VSPI is used by SD card reader
  SPIClass* hspi = NULL;
  hspi = new SPIClass(HSPI);
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
  LoRa.setSPI(*hspi);
  SPI.begin(18, 19, 23, 5);
  LoRa.setPins(SS, RST, DIO0);
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
  //enable error check, highly recommended
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spread_factor);
  Serial.println("LoRa Initializing OK!");
  delay(500);
  if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
  }
  EEPROM.begin(EEPROM_SIZE);
  EEPROM_position = EEPROM.read(0) + 1;
  EEPROM.write(0, EEPROM_position);
  EEPROM.commit();
  // http methods
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("bandwidth")) {
      bandwidth = (request->getParam("bandwidth")->value()).toInt();
      LoRa.setSignalBandwidth(bandwidth);
      Serial.println("BW is " + String(bandwidth));
    }
    if (request->hasParam("spread_factor")) {
      spread_factor = (request->getParam("spread_factor")->value()).toInt();
      LoRa.setSpreadingFactor(spread_factor);
      Serial.println("SF is " + String(spread_factor));
    }
    request->send(200, "text/plain", "OK");
  });
  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });
  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", rssi.c_str());
  });
  server.on("/snr", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", snr.c_str());
  });
  server.on("/string_num", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(packet_number).c_str());
  });
  server.on("/total_packets", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", String(total_packets).c_str());
  });
  server.on("/path", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", path.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", timestamp.c_str());
  });
  lcd.init();
  lcd.backlight();
  lcd.print("Init OK");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  //clear SPIFFS
  SPIFFS.remove("/photo.jpg");
  configTime(3600, 0, "ntp.telekom.sk");  //choose correct ntp server for your region, first argument is time shift in seconds
  Serial.println("Waiting for a new image");
  start_millis = millis();
}

void loop() {
  // try to parse packet
    int packet_size = LoRa.parsePacket();
    if (packet_size) {
      while (LoRa.available()) {
        for (int i = 0; i < chunk_size; i++) {
          chunk[i] = LoRa.read();  //read packet byte by byte
        }
        byte packet_info[4];
        LoRa.readBytes(packet_info, 4);
        packet_number = (packet_info[1] << 8) | packet_info[0]; //read packet number
        total_packets = (packet_info[3] << 8) | packet_info[2]; //read total packets info
      }
      if (packet_number == 1)  //this packet indicates start of new photo
      {
        new_photo();
        get_time();
      }
      Serial.print("Got packet " + String(packet_number) + "/" + String(total_packets));
      rssi = String(LoRa.packetRssi());
      snr = String(LoRa.packetSnr());
      Serial.println(" with RSSI: " + rssi + ", SNR: " + snr + " ,packet length= " + String(packet_size));
      if (!file) {
        Serial.println("Failed to open file in writing mode");
      } else {
        for (int i = 0; i < chunk_size; i++) {
          file.write(chunk[i]);   //write to SPIFFS
          photo.write(chunk[i]);  //write to SD card
          //Serial.print(String(chunk[i],HEX)); //for debugging purposes
        }
        //Serial.println("");
      }
      lcd_update(String(packet_number), String(total_packets), LoRa.packetRssi(), LoRa.packetSnr());
      current_packet_array[0] = packet_number;
      current_packet_array[1] = packet_number >> 8;
      send_ack(current_packet_array);
      if (packet_number == total_packets) {  //if received a final packet
        current_packet_array[0] = 0;
        current_packet_array[1] = 0;
        end_photo();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Finished");
      }
      start_millis = millis();
    }
  current_millis = millis();
  if (current_millis - start_millis >= ack_retransmission_period)  //If no packet is received after a specified period, request retransmission
  {
    Serial.println("Sending acknowledgment again for packet "+String((current_packet_array[1] << 8) | current_packet_array[0]));
    send_ack(current_packet_array);
    start_millis = current_millis;  //IMPORTANT to save the start time
  }
}
