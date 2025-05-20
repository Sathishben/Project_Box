#ifndef JUMP_GAME_H
#define JUMP_GAME_H

#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

bool jumpOver = false;
int jumpPlayerY = 20;
int velocity = 0;
bool jumping = false;
int obstacleX = 128;
int jumpScore = 0;

void drawJumpScene() {
  display.clearDisplay();
  display.drawLine(0, 30, 128, 30, SSD1306_WHITE);
  display.fillRect(5, jumpPlayerY, 5, 10, SSD1306_WHITE);  // Dinosaur
  display.fillRect(obstacleX, 22, 5, 8, SSD1306_WHITE);  // Obstacle
  display.setCursor(0, 0);
  display.print("S-");
  display.print(jumpScore);
  display.display();
}

void gameOverJump() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setFont(&FreeSans9pt7b);
  display.setCursor(25, 14);
  display.print("Game Over");
  display.setFont();
  display.setCursor(34, 24);
  display.print("Restart it..>");
  display.display();
  delay(1500);
  jumpOver = true;
}

void runJumpGame() {
  jumpPlayerY = 20;
  velocity = 0;
  jumping = false;
  obstacleX = 128;
  jumpScore = 0;
  jumpOver = false;

  while (!jumpOver) {
    if (!digitalRead(5) && !jumping) {
      jumping = true;
      velocity = -6;
    }

    if (jumping) {
      jumpPlayerY += velocity;
      velocity += 1;
      if (jumpPlayerY >= 20) {
        jumpPlayerY = 20;
        velocity = 0;
        jumping = false;
      }
    }

    obstacleX -= 3;
    if (obstacleX < -5) {
      obstacleX = 128;
      jumpScore++;
    }

    if (obstacleX < 10 && obstacleX + 5 > 5 && jumpPlayerY + 10 > 22) {
      gameOverJump();
      break;
    }

    drawJumpScene();
    delay(30);
  }
}
#endif
