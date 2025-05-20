#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>


#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SDA_PIN 1
#define SCL_PIN 2

#define BTN_DOWN 5
#define BTN_UP   6
#define BTN_RIGHT 7
#define BTN_LEFT  8

#define BLOCK_SIZE 2
#define BORDER 1
#define SCORE_BOX_WIDTH 20

#define GAME_ORIGIN_X (SCORE_BOX_WIDTH + BORDER)
#define GAME_ORIGIN_Y (BORDER)
#define GAME_WIDTH    (SCREEN_WIDTH - GAME_ORIGIN_X - BORDER)
#define GAME_HEIGHT   (SCREEN_HEIGHT - 2 * BORDER)

#define GRID_WIDTH  (GAME_WIDTH / BLOCK_SIZE)
#define GRID_HEIGHT (GAME_HEIGHT / BLOCK_SIZE)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define MAX_LENGTH 100
int snakeX[MAX_LENGTH];
int snakeY[MAX_LENGTH];
int length;
int dirX, dirY;
int foodX, foodY;
unsigned long lastMove = 0;
int speed = 120;
bool running = true;
bool gameOverShown = false;
bool paused = false;
unsigned long btnHoldStart = 0;
bool btnHeld = false;

void drawBorders() {
  display.drawRect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
  display.drawRect(0, 0, SCORE_BOX_WIDTH, SCREEN_HEIGHT, SSD1306_WHITE);
}

void drawScore() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(6, 6);
  display.print("S");
  display.setCursor(6, 18);
  display.print(length - 3);
}

void drawBlock(int gx, int gy) {
  int px = GAME_ORIGIN_X + gx * BLOCK_SIZE;
  int py = GAME_ORIGIN_Y + gy * BLOCK_SIZE;
  display.fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, SSD1306_WHITE);
}

void drawGame() {
  display.clearDisplay();
  drawBorders();
  drawScore();

  for (int i = 0; i < length; i++) drawBlock(snakeX[i], snakeY[i]);
  drawBlock(foodX, foodY);

  display.display();
}

void generateFood() {
  while (true) {
    foodX = random(0, GRID_WIDTH);
    foodY = random(0, GRID_HEIGHT);
    bool clash = false;
    for (int i = 0; i < length; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        clash = true;
        break;
      }
    }
    if (!clash) break;
  }
}

void startGame() {
  length = 3;
  dirX = 1; dirY = 0;
  for (int i = 0; i < length; i++) {
    snakeX[i] = 5 - i;
    snakeY[i] = 4;
  }
  generateFood();
  running = true;
  gameOverShown = false;
  paused = false;
  drawGame();
}

void gameOver() {
  running = false;
}

void handleInput() {
  if (!digitalRead(BTN_UP) && dirY == 0)    { dirX = 0; dirY = -1; }
  if (!digitalRead(BTN_DOWN) && dirY == 0)  { dirX = 0; dirY = 1;  }
  if (!digitalRead(BTN_LEFT) && dirX == 0)  { dirX = -1; dirY = 0; }
  if (!digitalRead(BTN_RIGHT) && dirX == 0) { dirX = 1; dirY = 0;  }
}

void moveSnake() {
  for (int i = length - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] += dirX;
  snakeY[0] += dirY;

  if (snakeX[0] < 0 || snakeX[0] >= GRID_WIDTH || snakeY[0] < 0 || snakeY[0] >= GRID_HEIGHT) {
    gameOver();
    return;
  }

  for (int i = 1; i < length; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      gameOver();
      return;
    }
  }

  if (snakeX[0] == foodX && snakeY[0] == foodY && length < MAX_LENGTH) {
    length++;
    generateFood();
  }
}

void checkPause() {
  if (!digitalRead(BTN_DOWN)) {
    if (!btnHeld) {
      btnHoldStart = millis();
      btnHeld = true;
    } else if (millis() - btnHoldStart > 1000) {
      paused = !paused;
      btnHeld = false;
    }
  } else {
    btnHeld = false;
  }
}

void gameOverAnimation() {
  display.clearDisplay();
  drawBorders();
  drawScore();

  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(26, 14);     // Centered smaller
  display.print("Game Over");

  display.setFont();  // back to default
  display.setCursor(36, 22);
  display.print("Restart it..>");

  display.display();
  delay(1500);
  gameOverShown = true;
}


void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);

  pinMode(BTN_UP, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_LEFT, INPUT);
  pinMode(BTN_RIGHT, INPUT);

  randomSeed(millis());
  startGame();
}

void loop() {
  checkPause();

  if (running && !paused) {
    handleInput();
    if (millis() - lastMove > speed) {
      moveSnake();
      drawGame();
      lastMove = millis();
    }
  } else if (!running) {
    if (!gameOverShown) gameOverAnimation();
    if (!digitalRead(BTN_DOWN)) {
      delay(300);
      startGame();
    }
  } else {
    display.setTextSize(1);
    display.setCursor(45, 10);
    display.print("Paused...");
    display.display();
    delay(100);
  }
}
