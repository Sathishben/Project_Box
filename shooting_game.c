// Shooting Game UI layout updated

#include <Wire.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h>
#include <Fonts/FreeSans9pt7b.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define SDA_PIN 1
#define SCL_PIN 2

#define BTN_PAUSE 5
#define BTN_SHOOT 6
#define BTN_DOWN 7
#define BTN_UP 8

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

struct Bullet { int x, y; bool active; };
struct Enemy { int x, y; bool active; bool shooter; char type; };
struct EnemyBullet { int x, y; bool active; };

Bullet bullets[3];
Enemy enemies[5];
EnemyBullet enemyBullets[5];

int playerX = 4;
int playerY = 14;
int score = 0;
int lives = 3;
int stage = 1;
int bossHealth = 10;
int bossMaxHealth = 10;
bool paused = false;
bool lastPauseBtn = true;
bool gameOverShown = false;
bool bossFight = false;
int bossY = 10;
int bossDir = 1;

unsigned long lastShoot = 0;
unsigned long lastEnemy = 0;
unsigned long lastEnemyShot = 0;
unsigned long lastBossMove = 0;
unsigned long lastBossShot = 0;

void setup() {
  Serial.begin(115200);
  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  pinMode(BTN_PAUSE, INPUT);
  pinMode(BTN_SHOOT, INPUT);
  pinMode(BTN_DOWN, INPUT);
  pinMode(BTN_UP, INPUT);
}

void drawPlane(int x, int y) {
  display.drawPixel(x, y+1, SSD1306_WHITE);
  display.drawLine(x+1, y, x+1, y+2, SSD1306_WHITE);
  display.drawLine(x+2, y+1, x+3, y+1, SSD1306_WHITE);
}

void drawBoss() {
  if (!bossFight) return;
  display.drawRect(100, bossY, 6, 12, SSD1306_WHITE);
  display.drawLine(100, bossY, 106, bossY+12, SSD1306_WHITE);
  display.drawLine(106, bossY, 100, bossY+12, SSD1306_WHITE);
}

void drawEnemies() {
  for (auto &e : enemies)
    if (e.active) {
      switch (e.type) {
        case 'o': display.drawCircle(e.x + 2, e.y + 2, 2, SSD1306_WHITE); break;
        case 'x': display.drawLine(e.x, e.y, e.x + 3, e.y + 3, SSD1306_WHITE); display.drawLine(e.x + 3, e.y, e.x, e.y + 3, SSD1306_WHITE); break;
        case 's': display.fillTriangle(e.x, e.y + 4, e.x + 2, e.y, e.x + 4, e.y + 4, SSD1306_WHITE); break;
        case 'b': display.drawRect(e.x, e.y, 4, 4, SSD1306_WHITE); break;
        default: display.fillRect(e.x, e.y, 4, 4, SSD1306_WHITE); break;
      }
    }
}

void drawBullets() {
  for (auto &b : bullets)
    if (b.active) display.drawPixel(b.x, b.y, SSD1306_WHITE);
}

void drawEnemyBullets() {
  for (auto &eb : enemyBullets)
    if (eb.active) display.setCursor(eb.x, eb.y), display.print("-");
}

void drawHearts() {
  for (int i = 0; i < lives; i++) display.fillCircle(2 + i * 6, 4, 2, SSD1306_WHITE);
}

void drawTopUI() {
  display.drawLine(0, 9, 127, 9, SSD1306_WHITE);
  drawHearts();
  display.setTextSize(1);
  display.setFont();
  display.setCursor(44, 1); display.print("S:"); display.print(score);
  display.setCursor(70, 1); display.print("L:"); display.print(stage);
  if (bossFight) {
    display.drawRect(100, 1, 24, 5, SSD1306_WHITE);
    int fill = map(bossHealth, 0, bossMaxHealth, 0, 22);
    display.fillRect(101, 2, fill, 3, SSD1306_WHITE);
  }
}

void shootBullet() {
  for (auto &b : bullets)
    if (!b.active) {
      b.x = playerX + 4; b.y = playerY + 1; b.active = true;
      break;
    }
}

void shootEnemyBullet(int x, int y) {
  for (auto &eb : enemyBullets)
    if (!eb.active) {
      eb.x = x - 1; eb.y = y + 1; eb.active = true;
      break;
    }
}

void spawnEnemy() {
  for (auto &e : enemies)
    if (!e.active) {
      e.x = SCREEN_WIDTH - 4;
      e.y = random(10, SCREEN_HEIGHT - 8);
      e.shooter = score >= 5 && random(0, 2);
      char shapes[] = { 'o', 'x', 's', 'b' };
      e.type = shapes[random(0, 4)];
      e.active = true;
      break;
    }
}

void moveObjects() {
  for (auto &b : bullets)
    if (b.active && (b.x += 2) >= SCREEN_WIDTH) b.active = false;

  for (auto &e : enemies)
    if (e.active && (e.x -= 1) <= 0) e.active = false;

  for (auto &eb : enemyBullets)
    if (eb.active && (eb.x -= 2) <= 0) eb.active = false;

  if (bossFight && millis() - lastBossMove > 100) {
    bossY += bossDir;
    if (bossY <= 10 || bossY >= SCREEN_HEIGHT - 14) bossDir *= -1;
    lastBossMove = millis();
  }
}

void checkCollisions() {
  for (auto &b : bullets) {
    if (!b.active) continue;
    for (auto &e : enemies) {
      if (e.active && b.x >= e.x && b.x <= e.x + 4 && b.y >= e.y && b.y <= e.y + 4) {
        b.active = false; e.active = false; score++;
      }
    }
    for (auto &eb : enemyBullets) {
      if (eb.active && abs(b.x - eb.x) <= 1 && abs(b.y - eb.y) <= 1) {
        b.active = false; eb.active = false;
      }
    }
  }

  for (auto &eb : enemyBullets)
    if (eb.active && eb.x <= playerX + 3 && eb.y >= playerY && eb.y <= playerY + 4)
      eb.active = false, lives--;

  for (auto &e : enemies)
    if (e.active && e.x <= playerX + 3 && e.y <= playerY + 4 && e.y + 4 >= playerY)
      e.active = false, lives--;

  if (bossFight) {
    for (auto &b : bullets)
      if (b.active && b.x >= 100 && b.x <= 106 && b.y >= bossY && b.y <= bossY + 12) {
        b.active = false; bossHealth--;
      }
    if (bossHealth <= 0) {
      bossFight = false;
      stage++;
      score = 0;
    }
  }
}

void gameOverAnimation() {
  display.clearDisplay();
  display.setFont(&FreeSans9pt7b);
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(26, 14);
  display.print("Game Over");
  display.setFont();
  display.setCursor(32, 24);
  display.print("Restart it.. >");
  display.display();
  delay(1500);
  gameOverShown = true;
}

void loop() {
  if (!digitalRead(BTN_PAUSE) && lastPauseBtn) paused = !paused, lastPauseBtn = false, delay(300);
  if (digitalRead(BTN_PAUSE)) lastPauseBtn = true;
  if (paused) {
    display.clearDisplay();
    drawTopUI();
    display.setCursor(36, 14); display.print("Game Paused");
    display.display();
    delay(50); return;
  }

  if (lives <= 0) {
    if (!gameOverShown) gameOverAnimation();
    if (!digitalRead(BTN_PAUSE)) {
      gameOverShown = false;
      lives = 3; score = 0; stage = 1; bossFight = false; bossHealth = 10; bossMaxHealth = 10; bossY = 10;
      for (auto &b : bullets) b.active = false;
      for (auto &e : enemies) e.active = false;
      for (auto &eb : enemyBullets) eb.active = false;
    }
    return;
  }

  if (!digitalRead(BTN_UP) && playerY > 10) playerY--;
  if (!digitalRead(BTN_DOWN) && playerY < SCREEN_HEIGHT - 6) playerY++;
  if (!digitalRead(BTN_SHOOT) && millis() - lastShoot > 250) shootBullet(), lastShoot = millis();
  if (!bossFight && millis() - lastEnemy > 1000) spawnEnemy(), lastEnemy = millis();
  if (millis() - lastEnemyShot > 1500) {
    for (auto &e : enemies) if (e.active && e.shooter) shootEnemyBullet(e.x, e.y);
    lastEnemyShot = millis();
  }
  if (bossFight && millis() - lastBossShot > 1200) {
    shootEnemyBullet(100, bossY + 5);
    lastBossShot = millis();
  }

  moveObjects();
  checkCollisions();

  if (!bossFight && score >= 10) {
    bossFight = true;
    for (auto &e : enemies) e.active = false;
    bossMaxHealth = 10 + stage * 5;
    bossHealth = bossMaxHealth;
  }

  display.clearDisplay();
  drawTopUI();
  drawPlane(playerX, playerY);
  drawBullets();
  drawEnemies();
  drawEnemyBullets();
  drawBoss();
  display.display();
  delay(30);
}
