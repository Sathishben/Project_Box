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

#include "SnakeGame.h"
#include "JumpGame.h"
#include "ShootingGame.h"

#define OLED_RESET     -1
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT  32
#define SDA_PIN        1
#define SCL_PIN        2
#define TEMP_PIN       4

#define BTN_SELECT     5
#define BTN_UP         6
#define BTN_DOWN       7
#define BTN_MENU       8

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

int currentSelection = 0;
const char *games[] = { "Snake Game", "Jump Game", "Shooting Game", "Back" };
const int numGames = sizeof(games) / sizeof(games[0]);
bool inClockScreen = true;

// Sleep Logic
unsigned long lastInteraction = 0;
const unsigned long sleepTimeout = 30000; // 30 seconds
bool displaySleeping = false;

void setupBLE() {
  BLEDevice::init("ClockWiFiSetup");
  BLEServer *pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService("1234");

  pSSID = pService->createCharacteristic("1235", BLECharacteristic::PROPERTY_WRITE);
  pPASS = pService->createCharacteristic("1236", BLECharacteristic::PROPERTY_WRITE);

  class SSIDCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      ssidReceived = pChar->getValue().c_str();
    }
  };
  class PASSCallback : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pChar) {
      passReceived = pChar->getValue().c_str();
      newCredsReceived = true;
    }
  };

  pSSID->setCallbacks(new SSIDCallback());
  pPASS->setCallbacks(new PASSCallback());

  pService->start();
  BLEDevice::getAdvertising()->start();
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
  sensors.begin();

  preferences.begin("wifi", false);
  setupBLE();
  connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    timeClient.begin();
    timeClient.update();
  }

  pinMode(BTN_SELECT, INPUT);
  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_MENU, INPUT);

  lastInteraction = millis();  // Start sleep timer
}

void drawClock() {
  sensors.requestTemperatures();
  float tempC = sensors.getTempCByIndex(0);

  if (WiFi.status() == WL_CONNECTED) timeClient.update();

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  display.setFont(&FreeSans9pt7b);
  display.setCursor(0, 14);
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

  display.setCursor(96, 24);
  display.print(String(tempC, 1) + "C");

  display.display();
}

void drawMenu() {
  display.clearDisplay();
  display.setFont();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  int startIdx = currentSelection;
  if (startIdx > numGames - 3) startIdx = numGames - 3;
  if (startIdx < 0) startIdx = 0;

  for (int i = 0; i < 3 && (startIdx + i) < numGames; i++) {
    int itemIndex = startIdx + i;
    display.setCursor(0, i * 10);
    if (itemIndex == currentSelection) display.print("> ");
    else display.print("  ");
    display.print(games[itemIndex]);
  }
  display.display();
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

  // Wake on any button
  bool anyButtonPressed = !digitalRead(BTN_MENU) || !digitalRead(BTN_SELECT) || !digitalRead(BTN_UP) || !digitalRead(BTN_DOWN);
  if (anyButtonPressed) {
    lastInteraction = millis();
    if (displaySleeping) {
      display.ssd1306_command(SSD1306_DISPLAYON);
      displaySleeping = false;
    }
  }

  if (inClockScreen) {
    drawClock();
    if (!digitalRead(BTN_MENU)) {
      inClockScreen = false;
      drawMenu();
      delay(300);
    }
    delay(1000);
  } else {
    if (!digitalRead(BTN_UP)) {
      currentSelection--;
      if (currentSelection < 0) currentSelection = numGames - 1;
      drawMenu();
      delay(200);
    }
    if (!digitalRead(BTN_DOWN)) {
      currentSelection++;
      if (currentSelection >= numGames) currentSelection = 0;
      drawMenu();
      delay(200);
    }

    if (!digitalRead(BTN_SELECT)) {
      if (currentSelection == 3) {
        inClockScreen = true;
      } else {
        display.clearDisplay();
        display.setCursor(0, 12);
        display.print(games[currentSelection]);
        display.display();
        delay(1000);

        switch (currentSelection) {
          case 0: runSnakeGame(); break;
          case 1: runJumpGame(); break;
          case 2: runShootingGame(); break;
        }
      }
      drawMenu();
      delay(200);
    }
  }

  // Sleep screen after timeout
  if (!displaySleeping && (millis() - lastInteraction > sleepTimeout)) {
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    displaySleeping = true;
  }
}
