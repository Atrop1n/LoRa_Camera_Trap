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

#define FILE_PHOTO "/photo.jpg"
#define EEPROM_SIZE 1

int EEPROMPosition = 0;
//credentials for your access point
const char* ssid = "SSID";
const char* password = "password";

AsyncWebServer server(80);

String totalPackets;
const int chunkSize = 250;
byte chunk[chunkSize];
int packetNumber;
byte num = 0;
String lastPacket = "";
String stringChunk = "";
int lastNum = 0;
int factor = 0;
int lcdColumns = 16;
int lcdRows = 2;
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);
File file;
File photo;
int EEPROMCount = 0;
long int bandwidth = 500000;
int spreadFactor = 7;
String stringNum;
String rssi;
String snr;
String path;
String timestamp;
//html page
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
   <p class="p">Received packets: <span id="stringNum">%STRING_NUM%</span>/<span
         id="totalPackets">%TOTAL_PACKETS%</span></p>
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
               <option value="500000" selected>500</option>
            </select>
         </td>
         </tr>
            <tr class="parameter">
               <td style="text-align: left;"><label for="spreadFactor">Spread factor</label></td>
               <td>
               <select onchange="getSpreadFactor(), retain()" name="spreadFactor" id="spreadFactor" class="dropdown-menu">
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
      setInterval(updateValues, 5000, "totalPackets");
      setInterval(updateValues, 5000, "stringNum");
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
         document.getElementById("demo").innerHTML = "Bandwidth set to " + x+ "<br>Changes will take effect after a picture is fully transmitted";
         xhttp.open("GET", "/update?bandwidth="+x, true);
         xhttp.send();
      }
       function getSpreadFactor(value) {
         var xhttp = new XMLHttpRequest();
         var x = document.getElementById("spreadFactor").value;
         document.getElementById("demo").innerHTML = "Spread factor set to " + x+ "<br>Changes will take effect after a picture is fully transmitted";
         xhttp.open("GET", "/update?spreadFactor="+ x, true);
         xhttp.send();
      }
       function retain(value) {
         sessionStorage.setItem("sf", document.getElementById("spreadFactor").value);
         sessionStorage.setItem("bw", document.getElementById("bandwidth").value);
      }
      window.onload = function() {
         {
            document.getElementById('spreadFactor').value=sessionStorage.getItem("sf");
            document.getElementById('bandwidth').value=sessionStorage.getItem("bw");
         }
      };
   </script>
</body>
</html>)rawliteral";
//this will help with periodic upadating of values
String processor(const String& var) {
  if (var == "RSSI") {
    return String(rssi);
  } else if (var == "SNR") {
    return String(snr);
  } else if (var == "STRING_NUM") {
    return String(stringNum);
  } else if (var == "TOTAL_PACKETS") {
    return String(totalPackets);
  } else if (var == "PATH") {
    return String(path);
  } else if (var == "TIMESTAMP") {
    return timestamp;
  }
  return String();
}
//formats packet number
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
// when a packet indicating a new photo arrives
void newPhoto() {
  if (EEPROMCount == 1000) {
    EEPROMCount = 0;
    EEPROM.begin(EEPROM_SIZE);
    EEPROMPosition = EEPROM.read(0) + 1;
    EEPROM.write(0, EEPROMPosition);
    EEPROM.commit();
  }
  path = "/picture" + String(EEPROMPosition) + "_" + String(EEPROMCount) + ".jpg"; //path where image will be stored on SD card
  EEPROMCount++;
  Serial.printf("Picture file name: %s\n", path.c_str());
  photo = SD.open(path.c_str(), FILE_WRITE); //save photo to SD card
  if (!photo) {
    Serial.println("Failed to open photo in writing mode");
  }
  file = SPIFFS.open(FILE_PHOTO, FILE_WRITE); //save current photo to SPIFFS
  if (!file) {
    Serial.println("Failed to open file in writing mode");
  }
}
//when a photo is finished
void endPhoto() {
  file.close();
  if (!photo){
    Serial.println("Couldn't save to SD card");
  }
  else {
    photo.close();
    Serial.println("Picture saved to SD card");
    }
}
//updates LCD values
void lcdUpdate(String stringNum, String totalPackets, int rssi, float snr) {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(stringNum);
  lcd.setCursor(4, 0);
  lcd.print("/" + totalPackets);
  lcd.setCursor(0, 1);
  lcd.print("RSSI:");
  lcd.setCursor(5, 1);
  lcd.print(rssi);
  lcd.setCursor(10, 1);
  lcd.print(snr);
}
//sets LoRa parameters configuration to the sender, updates own parameters
void sendConfigPacket(long int bandwidth, int spreadFactor) {
  Serial.println("Sending config packet to the sender");
  for (int i = 0; i < 3; i++) {//send multiple times, to ensure the packet is received
    LoRa.beginPacket();
    LoRa.print(String(bandwidth) + ":" + String(spreadFactor));
    LoRa.endPacket();
    delay(1500);
    Serial.println(".");
  }
  LoRa.setSignalBandwidth(bandwidth);
  Serial.println("Bandwidth set to " + String(bandwidth));
  LoRa.setSpreadingFactor(spreadFactor);
  Serial.println("Spread factor set to " + String(spreadFactor));
}
//updates timestamp
void getTime() {
  time_t now = time(NULL);
  struct tm tm_now;
  localtime_r(&now, &tm_now);
  char buff[100];
  strftime(buff, sizeof(buff), "%Y-%m-%d  %H:%M:%S", &tm_now);
  timestamp = buff;
  Serial.println(timestamp);
}
//to create soft AP
void softAP() {
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}
//if connecting to a network
void wifiConnect() {
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());
}

void setup() {
  //initialize Serial Monitor
  Serial.begin(115200);

  //softAP();
  wifiConnect();
  server.begin();
  
  //initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  } 
  //initialize both SPI interfaces, hspi is used by LoRa, vspi is used by SD card reader
  SPIClass* hspi = NULL;
  hspi = new SPIClass(HSPI);
  hspi->begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
  LoRa.setSPI(*hspi);
  SPI.begin(18, 19, 23, 5);
  //set LoRa pins
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
  //enable error check
  LoRa.enableCrc();
  LoRa.setSignalBandwidth(bandwidth);
  LoRa.setSpreadingFactor(spreadFactor);
  Serial.println("LoRa Initializing OK!");
  delay(500);
  //initialize SD card
  if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
  } 
  //initialize EEPROM
  EEPROM.begin(EEPROM_SIZE);
  EEPROMPosition = EEPROM.read(0) + 1;
  EEPROM.write(0, EEPROMPosition);
  EEPROM.commit();
  // initialize http methods
  server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest* request) {
    if (request->hasParam("bandwidth")) {
      bandwidth = (request->getParam("bandwidth")->value()).toInt();
    }
    if (request->hasParam("spreadFactor")) {
      spreadFactor = (request->getParam("spreadFactor")->value()).toInt();
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
  server.on("/stringNum", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", stringNum.c_str());
  });
  server.on("/totalPackets", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", totalPackets.c_str());
  });
  server.on("/path", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", path.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/plain", timestamp.c_str());
  });
  //initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.print("Init OK");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP());
  //clear SPIFFS
  SPIFFS.remove("/photo.jpg");
  //set current time
  configTime(7200, 0, "ntp.telekom.sk"); //choose correct ntp server for your area
  Serial.println("Waiting for a new image");
}

void loop() {
  // try to parse packet
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    if (packetSize == 4) { //this packet indicates start of new photo
      newPhoto();
      totalPackets = LoRa.readString();
      Serial.println(totalPackets);
      factor = 0;
      lastPacket = totalPackets;
      //update the timestamp
      getTime();
    }
    if (packetSize > 250) { //got a packet containing jpeg data
      while (LoRa.available()) {
        String thisNum;
        for (int i = 0; i < chunkSize; i++) {
          chunk[i] = LoRa.read(); //read packet byte by byte
          thisNum = String(chunk[i], HEX); //converts byte to string, for serial output
          if (thisNum.length() == 1) {
            thisNum = "0" + thisNum;
          }
          stringChunk += thisNum;
        }
        num = LoRa.read(); //reads packet number
      }
      packetNumber = (factor * 256) + num; //since packet number is stored in one byte ('num'), numbers greater than 255 must be incremented by ('factor'*256)
      if (num == 255 && num != lastNum) {
        factor++;
        lastNum = num;
      }
      lastNum = num;
      stringNum = addZeroes(String(packetNumber + 1, DEC));

      if (stringChunk != lastPacket) { //because each packet was sent multiple times, we process only packets that are not the same as previous one
        if (!file) {
          Serial.println("Failed to open file in writing mode");
        } else {
          for (int i = 0; i < chunkSize; i++) {
            file.write(chunk[i]); //write to SPIFFS
            photo.write(chunk[i]); //write to SD card
          }
        }

        Serial.print(stringNum + ":");
        Serial.print(stringChunk); //prints HEX values of a packet, for debugging purposes
        rssi = String(LoRa.packetRssi());
        snr = String(LoRa.packetSnr());
        Serial.println(" with RSSI: " + rssi + ", SNR: " + snr + " ,packet length= " + String(packetSize));
        lastPacket = stringChunk;
      }
      lcdUpdate(stringNum, totalPackets, LoRa.packetRssi(), LoRa.packetSnr());
      if (packetNumber + 1 == totalPackets.toInt()) { //if received a final packet
        endPhoto();
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("Finished");
        sendConfigPacket(bandwidth, spreadFactor);
      }
      stringChunk = "";
    }
  }
}
