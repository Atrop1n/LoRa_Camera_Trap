#include <SPI.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <RadioLib.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <time.h>
#include <SD.h>

#define FILE_PHOTO "/photo.jpg" //path to the image in SPIFFS
#define EEPROM_SIZE 1
#define HSPI_MISO 12
#define HSPI_MOSI 13
#define HSPI_SCLK 14
#define HSPI_SS 15
#define VSPI_MISO 19
#define VSPI_MOSI 23
#define VSPI_SCLK 18
#define VSPI_SS 5

int EEPROMPosition = 0;

const char* ssid = "wifi_ssid";
const char* password = "wifi_password";
AsyncWebServer server(80);
SPIClass hspi = SPIClass(HSPI);

//hspi = new SPIClass(HSPI);
SX1280 radio = new Module(HSPI_SS, 2, 27, 33, hspi, RADIOLIB_DEFAULT_SPI_SETTINGS);
int packet_number = 0;
int total_packets = 0;
int last_packet = -1;
String confirmation = "0";
unsigned long startMillis;  //some global variables available anywhere in the program
unsigned long currentMillis;
File file;
File photo;
int EEPROMCount = 0;
String stringNum;
String rssi;
String snr;
String path;
String timestamp;
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
   <h2 class="title">LoRa camera trap 2.4</h2>
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
               id="snr"> %SNR%</span></strong> dB</span>&nbsp<span class="packet_loss">
               </div>
               
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

String processor(const String &
  var) {
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
//when a photo is fully transmitted
void endPhoto() {
  file.close();
  Serial.println("Photo transmitted");
  if (!photo){
    Serial.println("Couldn't save to SD card");
  }
  else {
    photo.close();
    Serial.println("Picture saved to SD card");
    }
}

void getTime() {
  time_t now = time(NULL);
  struct tm tm_now;
  localtime_r( & now, & tm_now);
  char buff[100];
  strftime(buff, sizeof(buff), "%Y-%m-%d  %H:%M:%S", & tm_now);
  timestamp = buff;
  Serial.println(timestamp);
}

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
  Serial.begin(115200);
  wifiConnect();
  server.begin();
  SPIFFS.begin(true);
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  }
  hspi.begin(HSPI_SCLK,HSPI_MISO,HSPI_MOSI,HSPI_SS);
     if (!SD.begin()) {
    Serial.println("SD Card Mount Failed");
  } 
  else {
    Serial.println("SD OK");
  }
  //SPI.begin(HSPI_SCLK, HSPI_MISO, HSPI_MOSI, HSPI_SS);
  EEPROM.begin(EEPROM_SIZE);
  EEPROMPosition = EEPROM.read(0) + 1;
  EEPROM.write(0, EEPROMPosition);
  EEPROM.commit();
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/html", index_html, processor);
  });
  server.on("/update", HTTP_GET, [](AsyncWebServerRequest * request) {
    //if (request -> hasParam("bandwidth")) {
    // bandwidth = (request -> getParam("bandwidth") -> value()).toInt();
    // }
    //if (request -> hasParam("spreadFactor")) {
    //spreadFactor = (request -> getParam("spreadFactor") -> value()).toInt();
    //}
    request -> send(200, "text/plain", "OK");
  });
  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send(SPIFFS, FILE_PHOTO, "image/jpg", false);
  });
  server.on("/rssi", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/plain", rssi.c_str());
  });
  server.on("/snr", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/plain", snr.c_str());
  });
  //server.on("/packet_loss", HTTP_GET, [](AsyncWebServerRequest * request) {
  // request -> send_P(200, "text/plain", String(packet_loss).c_str());
  //});
  server.on("/stringNum", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/plain", String(packet_number).c_str());
  });
  server.on("/totalPackets", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/plain", String(total_packets).c_str());
  });
  server.on("/path", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/plain", path.c_str());
  });
  server.on("/timestamp", HTTP_GET, [](AsyncWebServerRequest * request) {
    request -> send_P(200, "text/plain", timestamp.c_str());
  });
  SPIFFS.remove("/photo.jpg");
  configTime(3600, 0, "ntp.telekom.sk"); //choose correct ntp server for your region, first argument is time shift in seconds
  // initialize SX1280 with default settings
  Serial.print(F("[SX1280] Initializing ... "));
  int state = radio.begin();
  //int state = radio.begin(2400.0, 1625.0 , 9, 7, 13, 12);
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }

  
  // set the function that will be called
  // when new packet is received
  radio.setDio1Action(setFlag);

  // start listening for LoRa packets
  Serial.print(F("[SX1280] Starting to listen ... "));
  state = radio.startReceive();
  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
  startMillis = millis();
}

// flag to indicate that a packet was received
volatile bool receivedFlag = false;

// disable interrupt when it's not needed
volatile bool enableInterrupt = true;

// this function is called when a complete packet
// is received by the module
// IMPORTANT: this function MUST be 'void' type
//            and MUST NOT have any arguments!
#if defined(ESP8266) || defined(ESP32)
ICACHE_RAM_ATTR
#endif
void setFlag(void) {
  if (!enableInterrupt) {
    return;
  }
  receivedFlag = true;
}

void loop() {
  currentMillis = millis();
  if (currentMillis - startMillis >= 500)  //test whether the period has elapsed
  {
    enableInterrupt = false;
    receivedFlag = false;
    Serial.println("Sent confirmation again: "+confirmation);
    Serial.println("Last packet is: "+String(last_packet));
    radio.transmit(confirmation);
    enableInterrupt = true;
    radio.startReceive();
    startMillis = currentMillis;  //IMPORTANT to save the start time 
  }
  if (receivedFlag) {
    enableInterrupt = false;
    receivedFlag = false;
    byte byteArr[255];
    int state = radio.readData(byteArr, 255);
    if (state == RADIOLIB_ERR_NONE) {
      packet_number = (byteArr[251] + 256 * byteArr[252]);
      total_packets = (byteArr[253] + 256 * byteArr[254]);
      //Serial.println(String(packet_number) + "/" + String(total_packets) + ": ");
      if (packet_number!=last_packet)
      {
      if (packet_number == 1) { //this packet indicates start of new photo
        newPhoto();
        getTime();
      }
      Serial.println(String(packet_number) + "/" + String(total_packets));
     
      for (int i = 0; i < 250; i++) {
        file.write(byteArr[i]); //write to SPIFFS
        photo.write(byteArr[i]); //write to SD card
      }
      last_packet = packet_number;
      }
      rssi = String(radio.getRSSI());
      snr = String(radio.getSNR());
      confirmation = String(packet_number);
      radio.transmit(confirmation);
      Serial.println("I am confirming that I just have got packet "+confirmation);
      startMillis = millis();
      if (packet_number == total_packets) { //if received a final packet
        confirmation = "0";
        endPhoto();
      }
    } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
      // packet was received, but is malformed
      Serial.println(F("CRC error!"));
    } else {
      // some other error occurred
      Serial.print(F("failed, code "));
      Serial.println(state);
    }
    // put module back to listen mode
    radio.startReceive();
    // we're ready to receive more packets,
    // enable interrupt service routine
    enableInterrupt = true;
  }

}
