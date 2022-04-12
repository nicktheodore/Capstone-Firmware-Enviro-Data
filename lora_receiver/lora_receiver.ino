//Libraries for LoRa
#include <SPI.h>
#include <LoRa.h>

//Libraries for OLED Display
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Libraries to get time from NTP Server
#include <NTPClient.h>
#include <WiFiUdp.h>

// Import Wi-Fi library
#include <WiFi.h>

// Data transfer
#include <HTTPClient.h>
#include <ArduinoJson.h>

//define the pins used by the LoRa transceiver module
#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 915E6

//OLED pins
//#define OLED_SDA 4
//#define OLED_SCL 15 
#define OLED_RST 4
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Home network credentials
const char* ssid     = "iPhone";
const char* password = "nicswifi";

//Your Domain name with URL path or IP address with path
const char* serverName = "http://10.0.0.138:5000/data";


// Define NTP Client to get time
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

// Variables to save date and time
String formattedDate;
String day;
String hour;
String timestamp;

const size_t capacity = JSON_OBJECT_SIZE(16);

// Initialize variables to get and save LoRa data
int rssi;
String loRaMessage;
String temperature;
String humidity;
String pressure;
String readingID;
String macAddr;

int packetsReceived;

HTTPClient http;   

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);

// Data doc
DynamicJsonDocument data(2048);

//Initialize OLED display
void startOLED(){

  //initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  } else {
    Serial.println(F("SSD1306 allocation success"));
  }
  
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA RECEIVER");
}

//Initialize LoRa module
void startLoRA(){
  int counter;
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  while (!LoRa.begin(BAND) && counter < 10) {
    Serial.print(".");
    counter++;
    delay(500);
  }
  if (counter == 10) {
    // Increment readingID on every new reading
    Serial.println("Starting LoRa failed!"); 
  }
  
  LoRa.setSyncWord(0xC4);
  
  Serial.println("LoRa Initialization OK!");
  display.setCursor(0,10);
  display.clearDisplay();
  display.print("LoRa Initializing OK!");
  display.display();
  delay(2000);
}

void connectWiFi(){
  bool connected = false;
  
  // Connect to Wi-Fi network with SSID and password
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
    
  while(WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
   }

  // Print local IP address and start web server
  Serial.println("");
  Serial.println("WiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("Posting to endpoint:");
  Serial.println(serverName);

  // Print endpoint we're POSTing to
  display.setCursor(0,30);
  display.print("Posting to endpoint:");
  display.setCursor(0,40);
  display.println(serverName);
  display.display();
}

// Read LoRa packet and get the sensor readings
void getLoRaData() {
  Serial.print("Lora packet received: ");
  // Read packet
  while (LoRa.available()) {
    String LoRaData = LoRa.readString();
    // LoRaData format: readingID/temperature&soilMoisture#batterylevel
    // String example: 1/27.43&654#95.34
    Serial.print(LoRaData); 
    
    // Get readingID, temperature and soil moisture
    int pos1 = LoRaData.indexOf('/');
    int pos2 = LoRaData.indexOf('&');
    int pos3 = LoRaData.indexOf('#');
    int pos4 = LoRaData.indexOf('@');
    readingID = LoRaData.substring(0, pos1);
    temperature = LoRaData.substring(pos1+1, pos2);
    humidity = LoRaData.substring(pos2+1, pos3);
    pressure = LoRaData.substring(pos3+1, pos4);
    macAddr  = LoRaData.substring(pos4+1, LoRaData.length());    
  }
  
  // Get RSSI
  rssi = LoRa.packetRssi();
  Serial.print(" with RSSI ");    
  Serial.println(rssi);

}

// Function to get date and time from NTPClient
void getTimeStamp() {
  while(!timeClient.update()) {
    timeClient.forceUpdate();
  }
  // The formattedDate comes with the following format:
  // 2018-05-28T16:00:13Z
  // We need to extract date and time
  formattedDate = timeClient.getFormattedDate();
  Serial.println(formattedDate);

//  // Extract date
//  int splitT = formattedDate.indexOf("T");
//  day = formattedDate.substring(0, splitT);
//  Serial.println(day);
//  // Extract time
//  hour = formattedDate.substring(splitT+1, formattedDate.length()-1);
//  Serial.println(hour);
//  timestamp = day + " " + hour;
}

void stashLoRaData() {
    StaticJsonDocument<capacity> doc;
    
    // Add values in the document
    doc["macAddr"]     = macAddr;
    doc["temperature"] = temperature;
    doc["humidity"]    = humidity;
    doc["pressure"]    = pressure;
    doc["readingID"]   = readingID;
    doc["timestamp"]   = formattedDate;
    doc["rssi"]        = String(rssi);

    data.add(doc);
}

void postDataToServer() {
  Serial.println("Posting JSON data to server...");
  
  // Block until we are able to connect to the WiFi access point
  if (WiFi.status() == WL_CONNECTED) {

    if (usingHotspot) {
      http.begin(serverName_hs);
    } else {
      http.begin(serverName);
    }
    
    http.addHeader("Content-Type", "application/json");         
     
    String requestBody;
    serializeJson(data, requestBody);
     
    int httpResponseCode = http.POST(requestBody);
 
    if(httpResponseCode>0) {
      String response = http.getString();                       
       
      Serial.println(httpResponseCode);   
      Serial.println(response);
      
      if (httpResponseCode == 200) {
        data.clear();
      } else {
        Serial.print("Error occurred while sending HTTP POST <");
        Serial.print(httpResponseCode);
        Serial.println(">");
      }
      Serial.println(requestBody);

    }
    else {
      Serial.println("Error occurred while sending HTTP POST (Invalid Response)");
    }
  }
}

void displayInfo() {
  display.clearDisplay();
  display.setCursor(0,5);
  display.println("Posting to Endpoint:");
  display.println(serverName);
  display.setCursor(0,35);
  display.println("Received Packet from:");
  display.println(macAddr);
  display.setCursor(0,55);
  display.print("Total: ");
  display.println(packetsReceived);
  display.display();
}

void setup() { 
  // Must start this before serial monitor
  startOLED();
  
  // Initialize Serial Monitor
  Serial.begin(115200);

  // Initialize LoRa and WiFi
  startLoRA();
  connectWiFi();
  
  // Initialize a NTPClient to get time
  timeClient.begin();
  
  // Set offset time in seconds to adjust for your timezone, for example:
  // GMT +1 = 3600
  // GMT +8 = 28800
  // GMT -1 = -3600
  // GMT 0 = 0
  timeClient.setTimeOffset(-5*3600);
}

void loop() {
  // Check if there are LoRa packets available on air
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    packetsReceived++;
    getLoRaData();
    getTimeStamp();
    stashLoRaData();
    displayInfo();
   }

   if ((data.size() % 10 == 0) && (!data.isNull())) {
    Serial.println(data.size());
    postDataToServer();
   }
}
