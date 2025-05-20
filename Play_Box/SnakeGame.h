#ifndef SNAKE_GAME_H
#define SNAKE_GAME_H

#include <Adafruit_SSD1306.h>
#include <Fonts/FreeSans9pt7b.h>

extern Adafruit_SSD1306 display;

#define BLOCK_SIZE 2
#define BORDER 1
#define SCORE_BOX_WIDTH 20

#define GAME_ORIGIN_X (SCORE_BOX_WIDTH + BORDER)
#define GAME_ORIGIN_Y (BORDER)
#define GAME_WIDTH    (128 - GAME_ORIGIN_X - BORDER)
#define GAME_HEIGHT   (32 - 2 * BORDER)

#define GRID_WIDTH  (GAME_WIDTH / BLOCK_SIZE)
#define GRID_HEIGHT (GAME_HEIGHT / BLOCK_SIZE)

#define BTN_DOWN 5
#define BTN_UP   6
#define BTN_RIGHT 7
#define BTN_LEFT  8

int snakeX[100], snakeY[100], snakeLength;
int foodX, foodY;
int dirX = 1, dirY = 0;
unsigned long lastMove = 0;
int snakeSpeed = 120;
bool running = true, gameOverShown = false, paused = false;
unsigned long btnHoldStart = 0;
bool btnHeld = false;

void drawSnakeBorders() {
  display.drawRect(0, 0, 128, 32, SSD1306_WHITE);
  display.drawRect(0, 0, SCORE_BOX_WIDTH, 32, SSD1306_WHITE);
}

void drawSnakeScore() {
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(6, 6); display.print("S");
  display.setCursor(6, 18); display.print(snakeLength - 3);
}

void drawSnakeBlock(int gx, int gy) {
  int px = GAME_ORIGIN_X + gx * BLOCK_SIZE;
  int py = GAME_ORIGIN_Y + gy * BLOCK_SIZE;
  display.fillRect(px, py, BLOCK_SIZE, BLOCK_SIZE, SSD1306_WHITE);
}

void drawSnakeGame() {
  display.clearDisplay();
  drawSnakeBorders();
  drawSnakeScore();
  for (int i = 0; i < snakeLength; i++) drawSnakeBlock(snakeX[i], snakeY[i]);
  drawSnakeBlock(foodX, foodY);
  display.display();
}

void generateFood() {
  while (true) {
    foodX = random(0, GRID_WIDTH);
    foodY = random(0, GRID_HEIGHT);
    bool clash = false;
    for (int i = 0; i < snakeLength; i++) {
      if (snakeX[i] == foodX && snakeY[i] == foodY) {
        clash = true;
        break;
      }
    }
    if (!clash) break;
  }
}

void startSnakeGame() {
  snakeLength = 3;
  dirX = 1; dirY = 0;
  for (int i = 0; i < snakeLength; i++) {
    snakeX[i] = 5 - i;
    snakeY[i] = 4;
  }
  generateFood();
  running = true;
  gameOverShown = false;
  paused = false;
  drawSnakeGame();
}

void snakeGameOver() {
  running = false;
}

void handleSnakeInput() {
  if (!digitalRead(BTN_UP) && dirY == 0)    { dirX = 0; dirY = -1; }
  if (!digitalRead(BTN_DOWN) && dirY == 0)  { dirX = 0; dirY = 1;  }
  if (!digitalRead(BTN_LEFT) && dirX == 0)  { dirX = -1; dirY = 0; }
  if (!digitalRead(BTN_RIGHT) && dirX == 0) { dirX = 1; dirY = 0;  }
}

void moveSnake() {
  for (int i = snakeLength - 1; i > 0; i--) {
    snakeX[i] = snakeX[i - 1];
    snakeY[i] = snakeY[i - 1];
  }

  snakeX[0] += dirX;
  snakeY[0] += dirY;

  if (snakeX[0] < 0 || snakeX[0] >= GRID_WIDTH || snakeY[0] < 0 || snakeY[0] >= GRID_HEIGHT) {
    snakeGameOver();
    return;
  }

  for (int i = 1; i < snakeLength; i++) {
    if (snakeX[0] == snakeX[i] && snakeY[0] == snakeY[i]) {
      snakeGameOver();
      return;
    }
  }

  if (snakeX[0] == foodX && snakeY[0] == foodY && snakeLength < 100) {
    snakeLength++;
    generateFood();
  }
}

void checkPauseSnake() {
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

void snakeGameOverAnimation() {
  display.clearDisplay();
  drawSnakeBorders();
  drawSnakeScore();

  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(26, 14);
  display.print("Game Over");

  display.setFont();
  display.setCursor(36, 22);
  display.print("Restart it.. >");

  display.display();
  delay(1500);
  gameOverShown = true;
}

void runSnakeGame() {
  startSnakeGame();

  while (true) {
    checkPauseSnake();

    if (running && !paused) {
      handleSnakeInput();
      if (millis() - lastMove > snakeSpeed) {
        moveSnake();
        drawSnakeGame();
        lastMove = millis();
      }
    } else if (!running) {
      if (!gameOverShown) snakeGameOverAnimation();
      if (!digitalRead(BTN_DOWN)) {
        delay(300);
        return; // Exit to menu
      }
    } else {
      display.setTextSize(1);
      display.setCursor(45, 10);
      display.print("Paused...");
      display.display();
      delay(100);
    }

    if (!digitalRead(BTN_UP) && !digitalRead(BTN_RIGHT)) {
      unsigned long holdStart = millis();
      while (!digitalRead(BTN_UP) && !digitalRead(BTN_RIGHT)) {
        if (millis() - holdStart > 1000) return; // Exit game
        delay(10);
      }
    }
  }
}

#endif
