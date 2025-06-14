#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define LEFT_BUTTON 2
#define RIGHT_BUTTON 3
#define UP_BUTTON 4
#define DOWN_BUTTON 5
#define A_BUTTON 6
#define B_BUTTON 7

#define BOARD_WIDTH 10
#define BOARD_HEIGHT 16
#define BLOCK_PIXEL 6

byte board[BOARD_HEIGHT][BOARD_WIDTH];
unsigned long lastFall = 0;
unsigned long fallInterval = 500;

const byte shapes[7][4][4] = {
  {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, // I
  {{0,0,0,0},{0,1,1,0},{0,1,1,0},{0,0,0,0}}, // O
  {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}}, // T
  {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}}, // S
  {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}}, // Z
  {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}}, // J
  {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}}  // L
};

int currentShape;
int currentRotation;
int currentX, currentY;
int nextShape;

int score = 0;

bool btnLeftPressed = false;
bool btnRightPressed = false;
bool btnUpPressed = false;
bool btnDownPressed = false;
bool btnAPressed = false;

void setup() {
  Serial.begin(115200);
  pinMode(LEFT_BUTTON, INPUT_PULLUP);
  pinMode(RIGHT_BUTTON, INPUT_PULLUP);
  pinMode(UP_BUTTON, INPUT_PULLUP);
  pinMode(DOWN_BUTTON, INPUT_PULLUP);
  pinMode(A_BUTTON, INPUT_PULLUP);
  pinMode(B_BUTTON, INPUT_PULLUP);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  display.clearDisplay();
  display.display();
  randomSeed(analogRead(A0));
  initGame();
}

void initGame() {
  for(int y=0; y<BOARD_HEIGHT; y++)
    for(int x=0; x<BOARD_WIDTH; x++)
      board[y][x] = 0;

  score = 0;
  nextShape = random(0, 7);
  spawnNewPiece();
}

void spawnNewPiece() {
  currentShape = nextShape;
  nextShape = random(0, 7);
  currentRotation = 0;
  currentX = BOARD_WIDTH/2 - 2;
  currentY = 0;

  if (checkCollision(currentX, currentY, currentRotation)) {
    gameOver();
  }
}

void gameOver() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 20);
  display.println("Game Over");

  display.setTextSize(1);
  display.setCursor(10, 50);
  display.print("Score: ");
  display.println(score);

  display.display();
  while(1);
}

void loop() {
  readButtons();
  unsigned long now = millis();

  if (btnDownPressed || (now - lastFall > fallInterval)) {
    if(!tryMove(0, 1, currentRotation)) {
      placePiece();
      int linesCleared = clearLines();
      if(linesCleared > 0) score += linesCleared * 100;
      spawnNewPiece();
    }
    lastFall = now;
  }

  displayGame();
  delay(50);
}

void readButtons() {
  btnLeftPressed = (digitalRead(LEFT_BUTTON) == LOW);
  btnRightPressed = (digitalRead(RIGHT_BUTTON) == LOW);
  btnUpPressed = (digitalRead(UP_BUTTON) == LOW);
  btnDownPressed = (digitalRead(DOWN_BUTTON) == LOW);
  btnAPressed = (digitalRead(A_BUTTON) == LOW);

  if (btnLeftPressed) tryMove(-1, 0, currentRotation);
  if (btnRightPressed) tryMove(1, 0, currentRotation);
  if (btnUpPressed) {
    int newRotation = (currentRotation + 1) % 4;
    if (!checkCollision(currentX, currentY, newRotation)) {
      currentRotation = newRotation;
    }
  }
}

bool tryMove(int dx, int dy, int rotation) {
  if (!checkCollision(currentX + dx, currentY + dy, rotation)) {
    currentX += dx;
    currentY += dy;
    return true;
  }
  return false;
}

bool checkCollision(int x, int y, int rotation) {
  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++) {
      byte block = getBlock(currentShape, rotation, j, i);
      if(block) {
        int bx = x + j;
        int by = y + i;
        if(bx < 0 || bx >= BOARD_WIDTH || by < 0 || by >= BOARD_HEIGHT)
          return true;
        if(board[by][bx] != 0)
          return true;
      }
    }
  return false;
}

byte getBlock(int shape, int rotation, int x, int y) {
  switch(rotation % 4) {
    case 0: return shapes[shape][y][x];
    case 1: return shapes[shape][3 - x][y];
    case 2: return shapes[shape][3 - y][3 - x];
    case 3: return shapes[shape][x][3 - y];
  }
  return 0;
}

void placePiece() {
  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++) {
      byte block = getBlock(currentShape, currentRotation, j, i);
      if(block) {
        int bx = currentX + j;
        int by = currentY + i;
        if(by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH) {
          board[by][bx] = 1;
        }
      }
    }
}

int clearLines() {
  int linesCleared = 0;
  for(int y=BOARD_HEIGHT-1; y>=0; y--) {
    bool full = true;
    for(int x=0; x<BOARD_WIDTH; x++) {
      if(board[y][x] == 0) {
        full = false;
        break;
      }
    }
    if(full) {
      linesCleared++;
      for(int yy=y; yy>0; yy--)
        for(int x=0; x<BOARD_WIDTH; x++)
          board[yy][x] = board[yy-1][x];
      for(int x=0; x<BOARD_WIDTH; x++) board[0][x] = 0;
      y++;
    }
  }
  return linesCleared;
}

void displayGame() {
  display.clearDisplay();

  // Skor
  int infoX = BOARD_WIDTH * BLOCK_PIXEL + 4;
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(infoX, 0);
  display.print("Score:");
  display.setCursor(infoX, 10);
  display.print(score);

  // Oyun alanı
  for(int y=0; y<BOARD_HEIGHT; y++)
    for(int x=0; x<BOARD_WIDTH; x++)
      if(board[y][x]) drawBlock(x, y);

  // Aktif parça
  for(int i=0; i<4; i++)
    for(int j=0; j<4; j++) {
      byte block = getBlock(currentShape, currentRotation, j, i);
      if(block) {
        int bx = currentX + j;
        int by = currentY + i;
        if(bx >= 0 && bx < BOARD_WIDTH && by >= 0 && by < BOARD_HEIGHT)
          drawBlock(bx, by);
      }
    }

  // Sıradaki şekil
  display.setCursor(infoX, 24);
  display.print("Next:");
  for(int i = 0; i < 4; i++)
    for(int j = 0; j < 4; j++)
      if (shapes[nextShape][i][j]) {
        int px = infoX + j * 3;
        int py = 34 + i * 3;
        display.fillRect(px, py, 2, 2, SSD1306_WHITE);
      }

  display.display();
}

void drawBlock(int x, int y) {
  int px = x * BLOCK_PIXEL;
  int py = y * 4;
  display.fillRect(px, py, BLOCK_PIXEL, 4, SSD1306_WHITE);
}
