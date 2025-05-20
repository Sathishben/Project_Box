#ifndef SHOOTING_GAME_H
#define SHOOTING_GAME_H

#include <Adafruit_SSD1306.h>

extern Adafruit_SSD1306 display;

struct Bullet { int x, y; bool active; };
struct Enemy { int x, y; bool active; bool shooter; };
struct EnemyBullet { int x, y; bool active; };

Bullet bullets[3];
Enemy enemies[5];
EnemyBullet enemyBullets[5];

int shootPlayerY = 10;
int shootScore = 0;
int shootLives = 3;
bool shootGameOver = false;

void drawShootingScene() {
  display.clearDisplay();
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);

  for (int i = 0; i < shootLives; i++)
    display.fillCircle(2 + i * 6, 4, 2, SSD1306_WHITE);

  display.setCursor(44, 1);
  display.print("S-");
  display.print(shootScore);

  display.fillRect(4, shootPlayerY, 3, 5, SSD1306_WHITE);

  for (auto &b : bullets)
    if (b.active) display.drawPixel(b.x, b.y, SSD1306_WHITE);

  for (auto &e : enemies)
    if (e.active) display.fillRect(e.x, e.y, 4, 4, SSD1306_WHITE);

  for (auto &eb : enemyBullets)
    if (eb.active) display.setCursor(eb.x, eb.y), display.print("-");

  display.display();
}

void shootingGameOver() {
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setCursor(26, 14);
  display.print("Game Over");
  display.setFont();
  display.setCursor(32, 24);
  display.print("Restart it.. >");
  display.display();
  delay(1500);
  shootGameOver = true;
}

void runShootingGame() {
  shootScore = 0;
  shootLives = 3;
  shootPlayerY = 10;
  shootGameOver = false;
  for (auto &b : bullets) b.active = false;
  for (auto &e : enemies) e.active = false;
  for (auto &eb : enemyBullets) eb.active = false;

  unsigned long lastShoot = 0, lastEnemy = 0, lastEnemyShoot = 0;

  while (!shootGameOver) {
    if (!digitalRead(8) && shootPlayerY > 10) shootPlayerY--;
    if (!digitalRead(7) && shootPlayerY < 26) shootPlayerY++;
    if (!digitalRead(6) && millis() - lastShoot > 300) {
      for (auto &b : bullets)
        if (!b.active) {
          b.x = 8; b.y = shootPlayerY + 2; b.active = true; break;
        }
      lastShoot = millis();
    }

    if (millis() - lastEnemy > 1000) {
      for (auto &e : enemies)
        if (!e.active) {
          e.x = 124;
          e.y = random(10, 24);
          e.shooter = random(0, 2);
          e.active = true;
          break;
        }
      lastEnemy = millis();
    }

    if (millis() - lastEnemyShoot > 1500) {
      for (auto &e : enemies)
        if (e.active && e.shooter)
          for (auto &eb : enemyBullets)
            if (!eb.active) {
              eb.x = e.x - 1;
              eb.y = e.y + 2;
              eb.active = true;
              break;
            }
      lastEnemyShoot = millis();
    }

    for (auto &b : bullets)
      if (b.active && (b.x += 2) > 127) b.active = false;

    for (auto &e : enemies)
      if (e.active && (e.x -= 1) <= 0) e.active = false;

    for (auto &eb : enemyBullets)
      if (eb.active && (eb.x -= 2) <= 0) eb.active = false;

    for (auto &b : bullets) {
      if (!b.active) continue;
      for (auto &e : enemies)
        if (e.active && b.x >= e.x && b.x <= e.x + 4 && b.y >= e.y && b.y <= e.y + 4)
          { b.active = false; e.active = false; shootScore++; }

      for (auto &eb : enemyBullets)
        if (eb.active && abs(b.x - eb.x) <= 1 && abs(b.y - eb.y) <= 1)
          { b.active = false; eb.active = false; }
    }

    for (auto &eb : enemyBullets)
      if (eb.active && eb.x <= 8 && eb.y >= shootPlayerY && eb.y <= shootPlayerY + 5)
        { eb.active = false; shootLives--; }

    for (auto &e : enemies)
      if (e.active && e.x <= 8 && e.y >= shootPlayerY && e.y <= shootPlayerY + 5)
        { e.active = false; shootLives--; }

    if (shootLives <= 0) {
      shootingGameOver();
      break;
    }

    drawShootingScene();
    delay(30);
  }
}

#endif
