#include <WiFi.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Preferences.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define OLED_RESET     -1
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  32
#define SDA_PIN        1
#define SCL_PIN        2
#define TEMP_PIN       4

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
OneWire oneWire(TEMP_PIN);
DallasTemperature sensors(&oneWire);
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 19800);  // IST offset
Preferences preferences;

BLECharacteristic *pSSID;
BLECharacteristic *pPASS;

bool newCredsReceived = false;
String ssidReceived = "", passReceived = "";

void setupBLE() {
  BLEDevice::init("ClockWiFiSetup");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("1234");

  pSSID = pService->createCharacteristic("1235", BLECharacteristic::PROPERTY_WRITE);
  pPASS = pService->createCharacteristic("1236", BLECharacteristic::PROPERTY_WRITE);

  class SSIDCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      ssidReceived = pCharacteristic->getValue().c_str();
    }
  };
  class PASSCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      passReceived = pCharacteristic->getValue().c_str();
      newCredsReceived = true;
    }
  };

  pSSID->setCallbacks(new SSIDCallback());
  pPASS->setCallbacks(new PASSCallback());

  pService->start();
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->start();
}

void connectWiFi() {
  String savedSSID = preferences.getString("ssid", "");
  String savedPASS = preferences.getString("pass", "");
  if (savedSSID != "") {
    WiFi.begin(savedSSID.c_str(), savedPASS.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
      delay(100);
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.display();

  sensors.begin();
  preferences.begin("wifi", false);
  setupBLE();
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    timeClient.update();
  }
}

void loop() {
  if (newCredsReceived) {
    WiFi.begin(ssidReceived.c_str(), passReceived.c_str());
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
      delay(100);
    if (WiFi.status() == WL_CONNECTED) {
      preferences.putString("ssid", ssidReceived);
      preferences.putString("pass", passReceived);
      timeClient.begin();
    }
    newCredsReceived = false;
  }

  if (WiFi.status() == WL_CONNECTED) timeClient.update();

  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 15);
  String timeStr = timeClient.getFormattedTime();
  int hour = timeStr.substring(0, 2).toInt();
  String ampm = "AM";
  if (hour >= 12) { ampm = "PM"; if (hour > 12) hour -= 12; }
  if (hour == 0) hour = 12;
  timeStr = (hour < 10 ? " " : "") + String(hour) + timeStr.substring(2) + " " + ampm;
  display.print(timeStr);

  display.setFont();
  display.setCursor(0, 24);
  time_t rawTime = timeClient.getEpochTime();
  struct tm *ti = localtime(&rawTime);
  char dateBuf[20];
  strftime(dateBuf, sizeof(dateBuf), "%d %b (%a)", ti);
  display.print(dateBuf);

  display.setCursor(90, 24);
  display.print(String(tempC, 1) + "C");

  display.display();
  delay(1000);
}
