#include <M5Cardputer.h>
#include <vector>
#include <SD.h>

#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 135
#define SNAKE_SIZE 5
#define COLLISION_OFFSET 10
#define GROWTH_INCREMENT 2
#define SPEEDUP_THRESHOLD 5
#define SPEEDUP_PERCENTAGE 10

struct Position {
  int x;
  int y;
};

std::vector<Position> snake;
Position fruit;
int direction = 0; // 0: right, 1: down, 2: left, 3: up
bool isPaused = false;
bool gameOver = false;
int fruitsEaten = 0; // Licznik zjedzonych owoców
int speedMultiplier = 0; // Współczynnik przyspieszenia
int highScore = 0; // Rekord zjedzonych owoców
Position prevTail;
const char* recordFile = "/snake-record.txt";

void setup() {
  auto cfg = M5.config();
  M5Cardputer.begin(cfg);
  M5Cardputer.Display.setRotation(1);
  M5Cardputer.Display.setTextSize(2);
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.fillScreen(TFT_BLACK);
  Serial.begin(115200);
  initSD();
  initGame();
}

void initSD() {
  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    highScore = 0;
  } else {
    if (SD.exists(recordFile)) {
      File file = SD.open(recordFile);
      if (file) {
        highScore = file.parseInt();
        file.close();
      }
    } else {
      File file = SD.open(recordFile, FILE_WRITE);
      if (file) {
        file.println("0");
        file.close();
      }
      highScore = 0;
    }
  }
}

void saveHighScore() {
  if (highScore > 0) {
    File file = SD.open(recordFile, FILE_WRITE);
    if (file) {
      file.println(highScore);
      file.close();
    }
  }
}

void initGame() {
  snake.clear();
  snake.push_back({60, 60});
  for (int i = 1; i < 5; i++) {
    snake.push_back({60 - i * SNAKE_SIZE, 60});
  }
  
  direction = 0; // Ustaw kierunek domyślny na prawo
  fruitsEaten = 0; // Zresetuj licznik zjedzonych owoców
  speedMultiplier = 0; // Zresetuj współczynnik przyspieszenia
  placeFruit(); // Umieść owoc w losowym miejscu
  gameOver = false; // Reset stanu końca gry
  isPaused = false; // Reset stanu pauzy
  M5Cardputer.Display.clear();
  drawStaticElements();
}

void loop() {
  M5Cardputer.update();
  
  if (!gameOver) {
    if (!isPaused) {
      readButtons();
      moveSnake();
      checkCollision();
      draw();
      delay(100 - speedMultiplier * 10); // Zastosuj przyspieszenie
    } else {
      readPauseButton();
    }
  } else {
    if (!isPaused) { // Dodano, aby rysować napis "GAME OVER" tylko raz
      playGameOverSound();
      displayGameOver();
      isPaused = true; // Zatrzymanie dalszego rysowania
    }
    readRestartButton();
  }
}

void readButtons() {
  if (M5Cardputer.Keyboard.isKeyPressed(';') && direction != 1) {
    direction = 3; // Up
  }
  if (M5Cardputer.Keyboard.isKeyPressed('.') && direction != 3) {
    direction = 1; // Down
  }
  if (M5Cardputer.Keyboard.isKeyPressed(',') && direction != 0) {
    direction = 2; // Left
  }
  if (M5Cardputer.Keyboard.isKeyPressed('/') && direction != 2) {
    direction = 0; // Right
  }
  if (M5Cardputer.Keyboard.isKeyPressed('p')) { // Przycisk 'p' na klawiaturze
    isPaused = !isPaused;
    delay(200); // Opóźnienie, aby uniknąć podwójnego przełączania
  }
}

void readPauseButton() {
  if (M5Cardputer.Keyboard.isKeyPressed('p')) { // Przycisk 'p' na klawiaturze
    isPaused = !isPaused;
    delay(200); // Opóźnienie, aby uniknąć podwójnego przełączania
  }
}

void readRestartButton() {
  if (M5Cardputer.Keyboard.isKeyPressed('n')) { // Przycisk 'n' na klawiaturze
    delay(200); // Opóźnienie, aby uniknąć podwójnego przełączania
    initGame(); // Inicjalizuj grę od nowa
  }
}

void moveSnake() {
  prevTail = snake.back();
  Position next = { snake[0].x, snake[0].y };
  switch (direction) {
    case 0: next.x += SNAKE_SIZE; break;
    case 1: next.y += SNAKE_SIZE; break;
    case 2: next.x -= SNAKE_SIZE; break;
    case 3: next.y -= SNAKE_SIZE; break;
  }

  // Handle screen wrapping
  if (next.x >= SCREEN_WIDTH) next.x = 0;
  if (next.x < 0) next.x = SCREEN_WIDTH - SNAKE_SIZE;
  if (next.y >= SCREEN_HEIGHT) next.y = 0;
  if (next.y < 0) next.y = SCREEN_HEIGHT - SNAKE_SIZE;

  // move tail to front
  snake.insert(snake.begin(), next);
  snake.pop_back();

  // Check if snake eats the fruit
  if (abs(snake[0].x - fruit.x) < COLLISION_OFFSET && abs(snake[0].y - fruit.y) < COLLISION_OFFSET) {
    for (int i = 0; i < GROWTH_INCREMENT; i++) {
      snake.push_back(snake.back()); // Zwiększ długość węża
    }
    // Play sound
    M5Cardputer.Speaker.tone(4000, 100); // Zagraj krótki dźwięk przy zjedzeniu owocu
    // Clear the previous fruit
    M5Cardputer.Display.fillCircle(fruit.x, fruit.y, SNAKE_SIZE, TFT_BLACK);
    placeFruit();
    fruitsEaten++; // Zwiększ licznik zjedzonych owoców
    // Sprawdź, czy przekroczono próg dla przyspieszenia
    if (fruitsEaten > highScore) {
      highScore = fruitsEaten;
      saveHighScore();
    }
    if (fruitsEaten % SPEEDUP_THRESHOLD == 0) {
      speedMultiplier++; // Zwiększ współczynnik przyspieszenia
    }
  }
}

void checkCollision() {
  for (int i = 1; i < snake.size(); i++) {
    if (snake[0].x == snake[i].x && snake[0].y == snake[i].y) {
      gameOver = true;
      isPaused = false; // Umożliwia wyświetlenie napisu "GAME OVER" tylko raz
    }
  }
}

void playGameOverSound() {
  // Zagraj trzy krótkie dźwięki o rosnącej częstotliwości
  M5Cardputer.Speaker.tone(2000, 100);
  delay(150);
  M5Cardputer.Speaker.tone(3000, 100);
  delay(150);
  M5Cardputer.Speaker.tone(4000, 100);
  delay(150);
}

void displayGameOver() {
  M5Cardputer.Display.clear();
  M5Cardputer.Display.setTextSize(2);
  
  M5Cardputer.Display.setTextColor(TFT_RED, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 40); // Przesunięto w górę
  M5Cardputer.Display.print("GAME OVER");
  
  M5Cardputer.Display.setTextColor(TFT_GREEN, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 70); // Przesunięto w górę
  M5Cardputer.Display.print("n - new game");

  M5Cardputer.Display.setTextColor(TFT_CYAN, TFT_BLACK);
  M5Cardputer.Display.setCursor(50, 100); // Dodano napis
  M5Cardputer.Display.print("p - pause");
  
  M5Cardputer.Display.setTextColor(TFT_WHITE, TFT_BLACK);
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);
}

void drawStaticElements() {
  M5Cardputer.Display.setTextSize(2); // Ustaw rozmiar czcionki na 2
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);
  M5Cardputer.Display.fillCircle(fruit.x, fruit.y, SNAKE_SIZE, TFT_RED);
}

void draw() {
  // Update fruit count without clearing the entire screen
  M5Cardputer.Display.fillRect(0, 0, SCREEN_WIDTH, 20, TFT_BLACK); // Zmniejszono wysokość obszaru czyszczenia
  M5Cardputer.Display.setTextSize(2); // Ustaw rozmiar czcionki na 2
  M5Cardputer.Display.setCursor(0, 0);
  M5Cardputer.Display.print("Fruits ");
  M5Cardputer.Display.print(fruitsEaten);
  M5Cardputer.Display.print(" Record ");
  M5Cardputer.Display.println(highScore);

  // Redraw snake
  M5Cardputer.Display.fillRect(prevTail.x, prevTail.y, SNAKE_SIZE, SNAKE_SIZE, TFT_BLACK);
  for (auto &p : snake) {
    M5Cardputer.Display.fillRect(p.x, p.y, SNAKE_SIZE, SNAKE_SIZE, TFT_GREEN);
  }
}

void placeFruit() {
  bool safePlacement;
  do {
    safePlacement = true;
    fruit.x = random(0, SCREEN_WIDTH / SNAKE_SIZE) * SNAKE_SIZE;
    fruit.y = random(20 / SNAKE_SIZE, SCREEN_HEIGHT / SNAKE_SIZE) * SNAKE_SIZE; // Adjusted to start at y = 20
    // Sprawdź czy owoc nie jest zbyt blisko węża
    for (auto &p : snake) {
      if (abs(p.x - fruit.x) < COLLISION_OFFSET && abs(p.y - fruit.y) < COLLISION_OFFSET) {
        safePlacement = false;
        break;
      }
    }
  } while (!safePlacement);
  
  // Draw the fruit only once after placing it
  M5Cardputer.Display.fillCircle(fruit.x, fruit.y, SNAKE_SIZE, TFT_RED);
}
