#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket
// ATtiny85 + LCD 16x2 I2C + 3 botones
//
// PB0 -> SDA
// PB2 -> SCL
// PB3 -> LEFT
// PB4 -> RIGHT
// PB1 -> OK
//
// Botones conectados entre el pin y GND.
// Se utiliza INPUT_PULLUP.
// ============================================================

// -------------------- LCD --------------------

#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// -------------------- Pines --------------------

#define PIN_BTN_LEFT  PB3
#define PIN_BTN_RIGHT PB4
#define PIN_BTN_OK    PB1

// -------------------- Configuración --------------------

#define DEBOUNCE_MS 35UL

#define PP_WIN_SCORE 5

#define PET_HUNGER_DECAY_MS 10000UL
#define PET_HAPPY_DECAY_MS 15000UL
#define PET_SURVIVE_WIN_MS 60000UL

#define BJ_TARGET_WINS 3
#define BJ_TARGET_LOSSES 3
#define BJ_DEALER_STEP_MS 1000UL

#define SNAKE_WIN_LENGTH 12
#define SNAKE_BASE_STEP_MS 700UL
#define SNAKE_MIN_STEP_MS 300UL

// ============================================================
// BOTONES
// ============================================================

struct Button {
  uint8_t pin;
  bool stableState;
  bool lastRead;
  uint32_t lastChange;
  bool pressed;
};

Button btnLeft = {
  PIN_BTN_LEFT,
  HIGH,
  HIGH,
  0,
  false
};

Button btnRight = {
  PIN_BTN_RIGHT,
  HIGH,
  HIGH,
  0,
  false
};

Button btnOk = {
  PIN_BTN_OK,
  HIGH,
  HIGH,
  0,
  false
};

void updateButton(Button &button, uint32_t now) {
  bool currentRead = digitalRead(button.pin);

  if (currentRead != button.lastRead) {
    button.lastRead = currentRead;
    button.lastChange = now;
  }

  if ((now - button.lastChange) >= DEBOUNCE_MS) {
    if (button.stableState != currentRead) {
      button.stableState = currentRead;

      if (button.stableState == LOW) {
        button.pressed = true;
      }
    }
  }
}

void updateButtons(uint32_t now) {
  updateButton(btnLeft, now);
  updateButton(btnRight, now);
  updateButton(btnOk, now);
}

bool consumePressed(Button &button) {
  if (button.pressed) {
    button.pressed = false;
    return true;
  }

  return false;
}

// ============================================================
// ESTADOS
// ============================================================

enum AppState : uint8_t {
  STATE_MENU,
  STATE_PINGPONG,
  STATE_MASCOTA,
  STATE_BLACKJACK,
  STATE_SNAKE
};

enum MiniStatus : uint8_t {
  STATUS_RUNNING,
  STATUS_WIN,
  STATUS_GAME_OVER
};

AppState appState = STATE_MENU;

// ============================================================
// FUNCIONES LCD
// ============================================================

void clearLine(uint8_t row) {
  lcd.setCursor(0, row);
  lcd.print("                ");
}

void printLine(uint8_t row, const char *text) {
  clearLine(row);
  lcd.setCursor(0, row);
  lcd.print(text);
}

void printCentered(uint8_t row, const char *text) {
  clearLine(row);

  uint8_t length = strlen(text);

  if (length > 16) {
    length = 16;
  }

  uint8_t start = (16 - length) / 2;

  lcd.setCursor(start, row);

  for (uint8_t i = 0; i < length; i++) {
    lcd.print(text[i]);
  }
}

void gotoMenu() {
  appState = STATE_MENU;
  lcd.clear();
}

// ============================================================
// MENU
// ============================================================

const char *menuItems[] = {
  "PingPong",
  "Mascota",
  "Blackjack",
  "Snake"
};

uint8_t menuIndex = 0;

void renderMenu() {
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print(">");

  lcd.print(menuItems[menuIndex]);

  uint8_t next = (menuIndex + 1) % 4;

  lcd.setCursor(0, 1);
  lcd.print(" ");
  lcd.print(menuItems[next]);
}

void handleMenu() {

  if (consumePressed(btnLeft)) {
    if (menuIndex == 0) {
      menuIndex = 3;
    } else {
      menuIndex--;
    }

    renderMenu();
  }

  if (consumePressed(btnRight)) {
    menuIndex++;

    if (menuIndex >= 4) {
      menuIndex = 0;
    }

    renderMenu();
  }

  if (consumePressed(btnOk)) {

    switch (menuIndex) {

      case 0:
        startPingPong();
        break;

      case 1:
        startMascota();
        break;

      case 2:
        startBlackjack();
        break;

      case 3:
        startSnake();
        break;
    }
  }
}

// ============================================================
// PING PONG
// ============================================================

MiniStatus ppStatus = STATUS_RUNNING;

int8_t ppBallX = 7;
int8_t ppVX = 1;
uint8_t ppBallY = 0;
int8_t ppVY = 1;

uint8_t ppPlayerY = 0;
uint8_t ppAiY = 0;

uint8_t ppPlayerScore = 0;
uint8_t ppAiScore = 0;

uint32_t ppLastStep = 0;

void resetPingPongBall(int8_t direction) {

  ppBallX = 7;
  ppBallY = random(0, 2);

  ppVX = direction;

  ppVY = random(0, 2) == 0 ? -1 : 1;
}

void startPingPong() {

  appState = STATE_PINGPONG;

  ppStatus = STATUS_RUNNING;

  ppPlayerScore = 0;
  ppAiScore = 0;

  ppPlayerY = 0;
  ppAiY = 0;

  ppLastStep = millis();

  resetPingPongBall(
    random(0, 2) == 0 ? -1 : 1
  );

  lcd.clear();
}

void renderPingPong() {

  char line0[17];
  char line1[17];

  for (uint8_t i = 0; i < 16; i++) {
    line0[i] = ' ';
    line1[i] = ' ';
  }

  line0[16] = '\0';
  line1[16] = '\0';

  char score[17];

  snprintf(
    score,
    sizeof(score),
    "P:%u AI:%u",
    ppPlayerScore,
    ppAiScore
  );

  for (uint8_t i = 0; i < strlen(score) && i < 16; i++) {
    line0[i] = score[i];
  }

  uint8_t ballPosition = ppBallX;

  if (ballPosition > 15) {
    ballPosition = 15;
  }

  line1[ballPosition] = 'o';

  // Jugador
  line1[0] = '|';

  // IA
  line1[15] = '|';

  lcd.setCursor(0, 0);
  lcd.print(line0);

  lcd.setCursor(0, 1);
  lcd.print(line1);
}

void renderPingPongEnd() {

  if (ppStatus == STATUS_WIN) {
    printCentered(0, "YOU WIN!");
  } else {
    printCentered(0, "GAME OVER");
  }

  char score[17];

  snprintf(
    score,
    sizeof(score),
    "P:%u AI:%u",
    ppPlayerScore,
    ppAiScore
  );

  printCentered(1, score);
}

void updatePingPong() {

  ppBallX += ppVX;
  ppBallY += ppVY;

  if (ppBallY < 0) {
    ppBallY = 0;
    ppVY = 1;
  }

  if (ppBallY > 1) {
    ppBallY = 1;
    ppVY = -1;
  }

  // IA
  if (random(0, 100) < 80) {
    ppAiY = ppBallY;
  }

  // Jugador
  if (ppBallX <= 0) {

    if (ppBallY == ppPlayerY) {

      ppBallX = 0;
      ppVX = 1;

      ppVY = random(0, 2) == 0 ? -1 : 1;

    } else {

      ppAiScore++;

      resetPingPongBall(1);
    }
  }

  // IA
  else if (ppBallX >= 15) {

    if (ppBallY == ppAiY) {

      ppBallX = 15;
      ppVX = -1;

      ppVY = random(0, 2) == 0 ? -1 : 1;

    } else {

      ppPlayerScore++;

      resetPingPongBall(-1);
    }
  }

  if (ppPlayerScore >= PP_WIN_SCORE) {
    ppStatus = STATUS_WIN;
  }

  if (ppAiScore >= PP_WIN_SCORE) {
    ppStatus = STATUS_GAME_OVER;
  }
}

void handlePingPong(uint32_t now) {

  if (consumePressed(btnOk)) {
    gotoMenu();
    renderMenu();
    return;
  }

  if (ppStatus != STATUS_RUNNING) {

    if (consumePressed(btnOk)) {
      gotoMenu();
      renderMenu();
    }

    return;
  }

  if (consumePressed(btnLeft)) {
    ppPlayerY = 0;
  }

  if (consumePressed(btnRight)) {
    ppPlayerY = 1;
  }

  if ((now - ppLastStep) >= 500UL) {

    ppLastStep = now;

    updatePingPong();

    if (ppStatus == STATUS_RUNNING) {
      renderPingPong();
    } else {
      renderPingPongEnd();
    }
  }
}

// ============================================================
// MASCOTA
// ============================================================

MiniStatus petStatus = STATUS_RUNNING;

uint8_t petHunger = 9;
uint8_t petHappy = 9;

uint32_t petStart = 0;
uint32_t petLastHunger = 0;
uint32_t petLastHappy = 0;

void startMascota() {

  appState = STATE_MASCOTA;

  petStatus = STATUS_RUNNING;

  petHunger = 9;
  petHappy = 9;

  petStart = millis();
  petLastHunger = petStart;
  petLastHappy = petStart;

  lcd.clear();

  printCentered(0, "Mascota ^_^");

  delay(500);
}

void renderMascota(uint32_t now) {

  char line0[17];
  char line1[17];

  uint16_t seconds =
    (uint16_t)((now - petStart) / 1000UL);

  snprintf(
    line0,
    sizeof(line0),
    "H:%u P:%u T:%us",
    petHunger,
    petHappy,
    seconds
  );

  snprintf(
    line1,
    sizeof(line1),
    "L=Feed R=Play"
  );

  printLine(0, line0);
  printLine(1, line1);
}

void renderMascotaEnd(uint32_t now) {

  uint16_t score =
    (uint16_t)((now - petStart) / 1000UL);

  if (petStatus == STATUS_WIN) {
    printCentered(0, "PET WIN!");
  } else {
    printCentered(0, "PET GAME OVER");
  }

  char line[17];

  snprintf(
    line,
    sizeof(line),
    "Score: %us",
    score
  );

  printCentered(1, line);
}

void handleMascota(uint32_t now) {

  if (consumePressed(btnOk)) {
    gotoMenu();
    renderMenu();
    return;
  }

  if (petStatus != STATUS_RUNNING) {
    return;
  }

  // Alimentar
  if (consumePressed(btnLeft)) {

    if (petHunger <= 7) {
      petHunger += 2;
    } else {
      petHunger = 9;
    }
  }

  // Jugar
  if (consumePressed(btnRight)) {

    if (petHappy <= 7) {
      petHappy += 2;
    } else {
      petHappy = 9;
    }
  }

  // Hambre
  if ((now - petLastHunger) >= PET_HUNGER_DECAY_MS) {

    petLastHunger += PET_HUNGER_DECAY_MS;

    if (petHunger > 0) {
      petHunger--;
    }
  }

  // Felicidad
  if ((now - petLastHappy) >= PET_HAPPY_DECAY_MS) {

    petLastHappy += PET_HAPPY_DECAY_MS;

    if (petHappy > 0) {
      petHappy--;
    }
  }

  if (petHunger == 0 || petHappy == 0) {

    petStatus = STATUS_GAME_OVER;

    renderMascotaEnd(now);

    return;
  }

  if ((now - petStart) >= PET_SURVIVE_WIN_MS) {

    petStatus = STATUS_WIN;

    renderMascotaEnd(now);

    return;
  }

  renderMascota(now);
}

// ============================================================
// BLACKJACK
// ============================================================

enum BJPhase : uint8_t {
  BJ_PLAYER_TURN,
  BJ_DEALER_TURN,
  BJ_ROUND_OVER,
  BJ_MATCH_WIN,
  BJ_MATCH_LOSE
};

BJPhase bjPhase = BJ_PLAYER_TURN;

uint8_t bjPlayer = 0;
uint8_t bjDealer = 0;
uint8_t bjDealerShown = 0;

uint8_t bjWins = 0;
uint8_t bjLosses = 0;

uint32_t bjLastDealerStep = 0;

char bjMessage[17];

uint8_t randomCard() {

  // Cartas simplificadas:
  // 1 = As
  // 2..10 = valor correspondiente
  return random(1, 11);
}

void bjStartRound() {

  bjPlayer =
    randomCard() +
    randomCard();

  bjDealerShown = randomCard();

  bjDealer =
    bjDealerShown +
    randomCard();

  bjPhase = BJ_PLAYER_TURN;

  strcpy(bjMessage, "Round Start");

  lcd.clear();
}

void startBlackjack() {

  appState = STATE_BLACKJACK;

  bjWins = 0;
  bjLosses = 0;

  bjStartRound();
}

void bjEndRound(const char *message, bool playerWon) {

  strncpy(
    bjMessage,
    message,
    16
  );

  bjMessage[16] = '\0';

  if (playerWon) {
    bjWins++;
  } else {
    bjLosses++;
  }

  if (bjWins >= BJ_TARGET_WINS) {

    bjPhase = BJ_MATCH_WIN;

  } else if (bjLosses >= BJ_TARGET_LOSSES) {

    bjPhase = BJ_MATCH_LOSE;

  } else {

    bjPhase = BJ_ROUND_OVER;
  }
}

void renderBlackjack() {

  char line0[17];
  char line1[17];

  if (bjPhase == BJ_PLAYER_TURN) {

    snprintf(
      line0,
      sizeof(line0),
      "P:%u D:?",
      bjPlayer
    );

    snprintf(
      line1,
      sizeof(line1),
      "L=Hit R=Stand"
    );

  } else if (bjPhase == BJ_DEALER_TURN) {

    snprintf(
      line0,
      sizeof(line0),
      "P:%u D:%u",
      bjPlayer,
      bjDealer
    );

    snprintf(
      line1,
      sizeof(line1),
      "Dealer..."
    );

  } else if (bjPhase == BJ_ROUND_OVER) {

    snprintf(
      line0,
      sizeof(line0),
      "%s",
      bjMessage
    );

    snprintf(
      line1,
      sizeof(line1),
      "W:%u L:%u OK",
      bjWins,
      bjLosses
    );

  } else if (bjPhase == BJ_MATCH_WIN) {

    snprintf(
      line0,
      sizeof(line0),
      "BLACKJACK WIN"
    );

    snprintf(
      line1,
      sizeof(line1),
      "W:%u L:%u",
      bjWins,
      bjLosses
    );

  } else {

    snprintf(
      line0,
      sizeof(line0),
      "BLACKJACK OVER"
    );

    snprintf(
      line1,
      sizeof(line1),
      "W:%u L:%u",
      bjWins,
      bjLosses
    );
  }

  printLine(0, line0);
  printLine(1, line1);
}

void updateBlackjack(uint32_t now) {

  if (bjPhase != BJ_DEALER_TURN) {
    return;
  }

  if ((now - bjLastDealerStep) < BJ_DEALER_STEP_MS) {
    return;
  }

  bjLastDealerStep = now;

  if (bjDealer < 17) {

    bjDealer += randomCard();

    renderBlackjack();

    return;
  }

  if (bjDealer > 21) {

    bjEndRound("Dealer Bust", true);

  } else if (bjPlayer > bjDealer) {

    bjEndRound("YOU WIN", true);

  } else if (bjPlayer < bjDealer) {

    bjEndRound("YOU LOSE", false);

  } else {

    strcpy(bjMessage, "PUSH");

    bjPhase = BJ_ROUND_OVER;
  }

  renderBlackjack();
}

void handleBlackjack(uint32_t now) {

  if (consumePressed(btnOk)) {

    if (bjPhase == BJ_ROUND_OVER) {

      bjStartRound();
      renderBlackjack();

    } else {

      gotoMenu();
      renderMenu();

      return;
    }
  }

  if (bjPhase == BJ_PLAYER_TURN) {

    // HIT
    if (consumePressed(btnLeft)) {

      bjPlayer += randomCard();

      if (bjPlayer > 21) {

        bjEndRound(
          "BUST! LOSE",
          false
        );

      } else {

        renderBlackjack();
      }
    }

    // STAND
    if (consumePressed(btnRight)) {

      bjPhase = BJ_DEALER_TURN;

      bjLastDealerStep = now;

      renderBlackjack();
    }
  }

  updateBlackjack(now);
}

// ============================================================
// SNAKE
// ============================================================
//
// A LCD 16x2 tem apenas 32 posições.
// Cada posição é uma célula do Snake.
//
// Índice:
// 0..15  = linha superior
// 16..31 = linha inferior
//
// ============================================================

MiniStatus snakeStatus = STATUS_RUNNING;

uint8_t snakeBody[32];

uint8_t snakeLen = 3;

uint8_t snakeDir = 1;
// 0 = cima
// 1 = direita
// 2 = baixo
// 3 = esquerda

uint8_t snakeFood = 10;

uint32_t snakeLastStep = 0;

byte snakeHeadChar;
byte snakeBodyChar;
byte snakeFoodChar;

bool snakeCharsCreated = false;

void createSnakeCharacters() {

  if (snakeCharsCreated) {
    return;
  }

  byte headChar[8] = {
    B01110,
    B11111,
    B11011,
    B11111,
    B01110,
    B01010,
    B10001,
    B00000
  };

  byte bodyChar[8] = {
    B00110,
    B01111,
    B11111,
    B11111,
    B11111,
    B01111,
    B00110,
    B00000
  };

  byte foodChar[8] = {
    B00100,
    B01110,
    B11111,
    B11111,
    B11111,
    B01110,
    B00100,
    B00000
  };

  lcd.createChar(0, headChar);
  lcd.createChar(1, bodyChar);
  lcd.createChar(2, foodChar);

  snakeCharsCreated = true;
}

bool snakeCellUsed(uint8_t cell) {

  for (uint8_t i = 0; i < snakeLen; i++) {

    if (snakeBody[i] == cell) {
      return true;
    }
  }

  return false;
}

void placeSnakeFood() {

  if (snakeLen >= 32) {

    snakeFood = 0;

    return;
  }

  do {

    snakeFood = random(0, 32);

  } while (snakeCellUsed(snakeFood));
}

void startSnake() {

  appState = STATE_SNAKE;

  snakeStatus = STATUS_RUNNING;

  snakeLen = 3;

  snakeDir = 1;

  snakeBody[0] = 2;
  snakeBody[1] = 1;
  snakeBody[2] = 0;

  placeSnakeFood();

  snakeLastStep = millis();

  lcd.clear();

  createSnakeCharacters();

  renderSnake();
}

uint16_t snakeStepTime() {

  int16_t speed =
    (int16_t)SNAKE_BASE_STEP_MS -
    (int16_t)(snakeLen - 3) * 20;

  if (speed < SNAKE_MIN_STEP_MS) {
    speed = SNAKE_MIN_STEP_MS;
  }

  return (uint16_t)speed;
}

void updateSnake() {

  int8_t headX =
    snakeBody[0] % 16;

  int8_t headY =
    snakeBody[0] / 16;

  if (snakeDir == 0) {
    headY--;
  }

  else if (snakeDir == 1) {
    headX++;
  }

  else if (snakeDir == 2) {
    headY++;
  }

  else {
    headX--;
  }

  // Colisão com parede
  if (
    headX < 0 ||
    headX > 15 ||
    headY < 0 ||
    headY > 1
  ) {

    snakeStatus = STATUS_GAME_OVER;

    return;
  }

  uint8_t newHead =
    (uint8_t)(headY * 16 + headX);

  bool grow =
    newHead == snakeFood;

  uint8_t checkLength = snakeLen;

  if (!grow && checkLength > 0) {
    checkLength--;
  }

  // Colisão com corpo
  for (uint8_t i = 0; i < checkLength; i++) {

    if (snakeBody[i] == newHead) {

      snakeStatus = STATUS_GAME_OVER;

      return;
    }
  }

  // Crescer
  if (grow && snakeLen < 32) {
    snakeLen++;
  }

  // Deslocar corpo
  for (
    int8_t i = snakeLen - 1;
    i > 0;
    i--
  ) {

    snakeBody[i] =
      snakeBody[i - 1];
  }

  snakeBody[0] = newHead;

  if (grow) {

    if (snakeLen >= SNAKE_WIN_LENGTH) {

      snakeStatus = STATUS_WIN;

    } else {

      placeSnakeFood();
    }
  }
}

void renderSnake() {

  if (snakeStatus == STATUS_RUNNING) {

    lcd.clear();

    // Desenhar comida
    uint8_t foodX = snakeFood % 16;
    uint8_t foodY = snakeFood / 16;

    lcd.setCursor(foodX, foodY);
    lcd.write(byte(2));

    // Corpo
    for (uint8_t i = 1; i < snakeLen; i++) {

      uint8_t x =
        snakeBody[i] % 16;

      uint8_t y =
        snakeBody[i] / 16;

      lcd.setCursor(x, y);
      lcd.write(byte(1));
    }

    // Cabeça
    uint8_t headX =
      snakeBody[0] % 16;

    uint8_t headY =
      snakeBody[0] / 16;

    lcd.setCursor(headX, headY);
    lcd.write(byte(0));

  } else {

    lcd.clear();

    if (snakeStatus == STATUS_WIN) {
      printCentered(0, "GANASTE!");
    } else {
      printCentered(0, "GAME OVER");
    }

    char score[17];

    snprintf(
      score,
      sizeof(score),
      "Score: %u",
      snakeLen - 3
    );

    printCentered(1, score);
  }
}

void handleSnake(uint32_t now) {

  if (consumePressed(btnOk)) {

    gotoMenu();
    renderMenu();

    return;
  }

  if (snakeStatus != STATUS_RUNNING) {
    return;
  }

  // Girar para esquerda
  if (consumePressed(btnLeft)) {

    snakeDir =
      (snakeDir + 3) % 4;
  }

  // Girar para direita
  if (consumePressed(btnRight)) {

    snakeDir =
      (snakeDir + 1) % 4;
  }

  uint16_t stepTime =
    snakeStepTime();

  if ((now - snakeLastStep) >= stepTime) {

    snakeLastStep = now;

    updateSnake();

    renderSnake();
  }
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  // Botones
  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);

  // I2C
  Wire.begin();

  // LCD
  lcd.init();
  lcd.backlight();

  // Aleatoriedade
  randomSeed(
    analogRead(A0) ^
    micros()
  );

  // Tela inicial
  lcd.clear();

  printCentered(
    0,
    "TinyPocket"
  );

  printCentered(
    1,
    "Mini Consola"
  );

  delay(1500);

  // Menu
  renderMenu();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  uint32_t now = millis();

  updateButtons(now);

  switch (appState) {

    case STATE_MENU:
      handleMenu();
      break;

    case STATE_PINGPONG:
      handlePingPong(now);
      break;

    case STATE_MASCOTA:
      handleMascota(now);
      break;

    case STATE_BLACKJACK:
      handleBlackjack(now);
      break;

    case STATE_SNAKE:
      handleSnake(now);
      break;
  }
}