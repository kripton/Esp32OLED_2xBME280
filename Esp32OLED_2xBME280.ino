// OLED
#include <U8x8lib.h>
#include <U8g2lib.h>

// BME280 environment sensors
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Data publishing
#include <WiFi.h>
#include <HTTPClient.h>
#include "ArduinoJson.h"


// Location of the sensors
#define LOCATIONNAME "Waschküche"

// Interval to push data
#define uS_TO_S_FACTOR 1000000
int DEEPSLEEP_SECONDS = 180; // 3 minutes

// Replace with your network credentials
const char* ssid     = "Netgear";
const char* password = "NIXDA";

// Unique device information
uint64_t chipid;
static char deviceid[21];

// Deep-sleep persistent counters
RTC_DATA_ATTR uint32_t loopCount = 0;
RTC_DATA_ATTR uint32_t successCount = 0;

// Define and configure our pins and drivers
U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);
Adafruit_BME280 bme76;
Adafruit_BME280 bme77;


void setup() {
  // Read unique device id
  chipid = ESP.getEfuseMac();
  sprintf(deviceid, "%" PRIu64, chipid);

  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.print("Setting up OLED...");
  u8g2.initDisplay();
  u8g2.setPowerSave(0);
  Serial.println("DONE");

  if (!bme76.begin(0x76)) {  
   Serial.println("Could not find a valid BME280 sensor at address 0x76, check wiring!");
  }
  if (!bme77.begin(0x77)) {  
   Serial.println("Could not find a valid BME280 sensor at address 0x77, check wiring!");
  }

  // Configure the deep-sleep wakeup-interval
  esp_sleep_enable_timer_wakeup(DEEPSLEEP_SECONDS * uS_TO_S_FACTOR);
}

void connectWiFi()
{
  int count = 0;
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
    count++;
    if (count > 25) {
      Serial.println("WiFi connection FAILED. Start sleeping now ...");
      WiFi.disconnect(true);
      delay(1000);
      esp_deep_sleep_start();
    }
  }
  Serial.println("");
  Serial.print("WiFi connected, IP address: ");
  Serial.println(WiFi.localIP());
}


void loop() {
  loopCount++;

  Serial.println("Beginning loop");
  connectWiFi();
  HTTPClient http;
  http.begin("http://192.168.2.76:3005/");
  http.addHeader("Content-Type", "application/json");

  const int capacity = 4096;
  StaticJsonBuffer <capacity> jb;
  
  JsonObject& root = jb.createObject();
  String output;

  root["deviceId"] = deviceid;
  root["loopCount"] = loopCount;

  JsonArray& sensorValues = jb.createArray();

  JsonObject& so1 = sensorValues.createNestedObject();
  so1["name"]  = String(LOCATIONNAME) + String(".oben");
  so1["type"] = "temperature";
  float temp76 = bme76.readTemperature();
  so1["value"] = temp76;
  so1["unit"]  = "°C";

  JsonObject& so2 = sensorValues.createNestedObject();
  so2["name"]  = String(LOCATIONNAME) + String(".oben");
  so2["type"] = "pressure";
  float pres76 = bme76.readPressure() / 100.0F;
  so2["value"] = pres76;
  so2["unit"]  = "hPa";

  JsonObject& so3 = sensorValues.createNestedObject();
  so3["name"]  = String(LOCATIONNAME) + String(".oben");
  so3["type"] = "humidity";
  float hum76 = bme76.readHumidity();
  so3["value"] = hum76;
  so3["unit"]  = "%";

  String line1 = String("To ") + String(temp76) +  String("°C Ho ") + String(hum76) +  String("%");
  String line2 = String("Po ") + String(pres76) +  String("hPa");


  JsonObject& so4 = sensorValues.createNestedObject();
  so4["name"]  = String(LOCATIONNAME) + String(".unten");
  so4["type"] = "temperature";
  float temp77 = bme77.readTemperature();
  so4["value"] = temp77;
  so4["unit"]  = "°C";

  JsonObject& so5 = sensorValues.createNestedObject();
  so5["name"]  = String(LOCATIONNAME) + String(".unten");
  so5["type"] = "pressure";
  float pres77 = bme77.readPressure() / 100.0F;
  so5["value"] = pres77;
  so5["unit"]  = "hPa";

  JsonObject& so6 = sensorValues.createNestedObject();
  so6["name"]  = String(LOCATIONNAME) + String(".unten");
  so6["type"] = "humidity";
  float hum77 = bme77.readHumidity();
  so6["value"] = hum77;
  so6["unit"]  = "%";

  String line3 = String("Tu ") + String(temp77) +  String("°C Hu ") + String(hum77) +  String("%");
  String line4 = String("Pu ") + String(pres77) +  String("hPa");

  root["sensors"] = sensorValues;


  int stringLength = root.printTo(output);
  Serial.println("OUTPUT:\n");
  Serial.println(output);
  Serial.println();
  Serial.print("POSTing ");
  Serial.print(stringLength);
  Serial.print(" Bytes of data ...");

  int ret = http.POST(output);
  Serial.print(" return: ");
  Serial.println(ret);

  if (ret == 200) {
    successCount++;
  }

  String line5 = String("OK ") + String(successCount) + String("/") + String(loopCount);
  String line6 = String("Last result: ") + String(ret);

  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.drawUTF8(0, 7,line1.c_str());
  u8g2.drawUTF8(0,18,line2.c_str());
  u8g2.drawUTF8(0,29,line3.c_str());
  u8g2.drawUTF8(0,40,line4.c_str());
  u8g2.drawUTF8(0,51,line5.c_str());
  u8g2.drawUTF8(0,62,line6.c_str());
  u8g2.sendBuffer();

  WiFi.disconnect(true);
  delay(1000);
  Serial.println("Going to Deep Sleep...");
  delay(1000);
  esp_deep_sleep_start();
}
