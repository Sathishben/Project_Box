#include <Wire.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SDA_PIN 1
#define SCL_PIN 2

#define BTN_1 5
#define BTN_2 6
#define BTN_3 7
#define BTN_4 8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  pinMode(BTN_1, INPUT);  // Set to INPUT if external pull-up is used
  pinMode(BTN_2, INPUT);
  pinMode(BTN_3, INPUT);
  pinMode(BTN_4, INPUT);
}

void loop() {
  display.clearDisplay();
  display.setCursor(0, 0);
  display.println("Button Test:");

  if (digitalRead(BTN_1) == LOW) display.println("BTN 1 Pressed");
  if (digitalRead(BTN_2) == LOW) display.println("BTN 2 Pressed");
  if (digitalRead(BTN_3) == LOW) display.println("BTN 3 Pressed");
  if (digitalRead(BTN_4) == LOW) display.println("BTN 4 Pressed");

  display.display();
  delay(100);
}
