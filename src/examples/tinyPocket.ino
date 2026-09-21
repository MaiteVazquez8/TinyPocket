#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// TinyPocket
//
// LCD 16x2 + OLED 128x64 + 3 botones
//
// LCD:
//   Menu
//   PingPong
//   Blackjack
//   Snake
//
// OLED:
//   Mascota animada
//
// I2C:
//   LCD  -> 0x27
//   OLED -> 0x3C
//
// BOTONES:
//   PB3 -> LEFT
//   PB4 -> RIGHT
//   PB1 -> OK
//
// Botones entre pin y GND.
// Se utiliza INPUT_PULLUP.
//
// IMPORTANTE:
// Las animaciones utilizan millis().
// No se utilizan delays durante las partidas.
// ============================================================


// ============================================================
// LCD
// ============================================================

#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

LiquidCrystal_I2C lcd(
  LCD_ADDRESS,
  LCD_COLS,
  LCD_ROWS
);


// ============================================================
// OLED
// ============================================================

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 pantalla(
  OLED_WIDTH,
  OLED_HEIGHT,
  &Wire,
  -1
);

bool oledDisponible = false;


// ============================================================
// PINES
// ============================================================

#define PIN_BTN_LEFT  PB3
#define PIN_BTN_RIGHT PB4
#define PIN_BTN_OK    PB1


// ============================================================
// TIEMPOS GENERALES
// ============================================================

#define DEBOUNCE_MS 35UL


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


void updateButton(
  Button &button,
  uint32_t now
) {

  bool currentRead =
    digitalRead(button.pin);


  if (
    currentRead !=
    button.lastRead
  ) {

    button.lastRead =
      currentRead;

    button.lastChange =
      now;
  }


  if (
    now - button.lastChange
    >= DEBOUNCE_MS
  ) {

    if (
      button.stableState !=
      currentRead
    ) {

      button.stableState =
        currentRead;


      if (
        button.stableState ==
        LOW
      ) {

        button.pressed =
          true;
      }
    }
  }
}


void updateButtons(
  uint32_t now
) {

  updateButton(
    btnLeft,
    now
  );

  updateButton(
    btnRight,
    now
  );

  updateButton(
    btnOk,
    now
  );
}


bool consumePressed(
  Button &button
) {

  if (button.pressed) {

    button.pressed =
      false;

    return true;
  }

  return false;
}


// ============================================================
// ESTADOS PRINCIPALES
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


AppState appState =
  STATE_MENU;


// ============================================================
// FUNCIONES LCD
// ============================================================

void clearLine(
  uint8_t row
) {

  lcd.setCursor(
    0,
    row
  );

  lcd.print(
    "                "
  );
}


void printLine(
  uint8_t row,
  const char *text
) {

  clearLine(row);

  lcd.setCursor(
    0,
    row
  );

  lcd.print(text);
}


void printCentered(
  uint8_t row,
  const char *text
) {

  clearLine(row);

  uint8_t length =
    strlen(text);


  if (length > 16) {
    length = 16;
  }


  uint8_t start =
    (16 - length) / 2;


  lcd.setCursor(
    start,
    row
  );


  for (
    uint8_t i = 0;
    i < length;
    i++
  ) {

    lcd.print(
      text[i]
    );
  }
}


// ============================================================
// UTILIDAD: TRANSICIÓN LCD
// ============================================================

bool lcdTransitionActive = false;

uint32_t lcdTransitionStart = 0;

uint8_t lcdTransitionStep = 0;


void startLCDTransition() {

  lcdTransitionActive = true;

  lcdTransitionStart =
    millis();

  lcdTransitionStep = 0;

  lcd.clear();
}


bool updateLCDTransition(
  uint32_t now
) {

  if (!lcdTransitionActive) {
    return true;
  }


  if (
    now - lcdTransitionStart
    < 80
  ) {

    return false;
  }


  lcdTransitionStart =
    now;


  lcdTransitionStep++;


  if (
    lcdTransitionStep >= 3
  ) {

    lcdTransitionActive =
      false;

    return true;
  }


  return false;
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

uint32_t menuAnimationTime = 0;

bool menuAnimation = false;


void renderMenu() {

  lcd.clear();


  lcd.setCursor(
    0,
    0
  );

  lcd.print(">");


  lcd.print(
    menuItems[menuIndex]
  );


  uint8_t next =
    (menuIndex + 1) % 4;


  lcd.setCursor(
    0,
    1
  );

  lcd.print(" ");


  lcd.print(
    menuItems[next]
  );
}


void animateMenuSelection(
  uint32_t now
) {

  if (!menuAnimation) {
    return;
  }


  if (
    now - menuAnimationTime
    >= 100
  ) {

    menuAnimationTime =
      now;

    menuAnimation =
      false;

    renderMenu();
  }
}


void gotoMenu() {

  appState =
    STATE_MENU;


  lcdTransitionActive =
    false;


  menuAnimation =
    false;


  if (oledDisponible) {

    pantalla.clearDisplay();

    pantalla.display();
  }


  renderMenu();
}


void handleMenu(
  uint32_t now
) {

  animateMenuSelection(now);


  if (menuAnimation) {
    return;
  }


  if (
    consumePressed(btnLeft)
  ) {

    if (menuIndex == 0) {

      menuIndex = 3;

    } else {

      menuIndex--;
    }


    menuAnimation =
      true;

    menuAnimationTime =
      now;

    lcd.clear();


    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      ">"
    );


    lcd.print(
      menuItems[menuIndex]
    );


    return;
  }


  if (
    consumePressed(btnRight)
  ) {

    menuIndex++;


    if (
      menuIndex >= 4
    ) {

      menuIndex = 0;
    }


    menuAnimation =
      true;

    menuAnimationTime =
      now;


    lcd.clear();


    lcd.setCursor(
      0,
      0
    );

    lcd.print(
      ">"
    );


    lcd.print(
      menuItems[menuIndex]
    );


    return;
  }


  if (
    consumePressed(btnOk)
  ) {

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

#define PP_WIN_SCORE 5

#define PP_BASE_STEP 170UL
#define PP_FAST_STEP 90UL

MiniStatus ppStatus =
  STATUS_RUNNING;


int8_t ppBallX = 7;

int8_t ppVX = 1;

uint8_t ppBallY = 0;

int8_t ppVY = 1;

uint8_t ppPlayerY = 0;

uint8_t ppAiY = 0;

uint8_t ppPlayerScore = 0;

uint8_t ppAiScore = 0;

uint32_t ppLastStep = 0;

uint32_t ppPointAnimationStart = 0;

bool ppPointAnimation = false;

bool ppFlash = false;


void resetPingPongBall(
  int8_t direction
) {

  ppBallX = 7;

  ppBallY =
    random(0, 2);

  ppVX =
    direction;

  ppVY =
    random(0, 2) == 0
    ? -1
    : 1;
}


void startPingPong() {

  appState =
    STATE_PINGPONG;


  ppStatus =
    STATUS_RUNNING;


  ppPlayerScore = 0;

  ppAiScore = 0;


  ppPlayerY = 0;

  ppAiY = 0;


  ppPointAnimation =
    false;


  ppFlash =
    false;


  ppLastStep =
    millis();


  resetPingPongBall(
    random(0, 2) == 0
    ? -1
    : 1
  );


  lcd.clear();

  renderPingPong();
}


uint32_t getPingPongSpeed() {

  uint8_t totalScore =
    ppPlayerScore +
    ppAiScore;


  uint32_t speed =
    PP_BASE_STEP -
    (totalScore * 10UL);


  if (
    speed < PP_FAST_STEP
  ) {

    speed =
      PP_FAST_STEP;
  }


  return speed;
}


void renderPingPong() {

  char line0[17];

  char line1[17];


  for (
    uint8_t i = 0;
    i < 16;
    i++
  ) {

    line0[i] = ' ';

    line1[i] = ' ';
  }


  line0[16] =
    '\0';

  line1[16] =
    '\0';


  char score[17];


  snprintf(
    score,
    sizeof(score),
    "P:%u AI:%u",
    ppPlayerScore,
    ppAiScore
  );


  for (
    uint8_t i = 0;
    i < strlen(score) &&
    i < 16;
    i++
  ) {

    line0[i] =
      score[i];
  }


  uint8_t ball =
    ppBallX;


  if (
    ball > 15
  ) {

    ball = 15;
  }


  if (!ppFlash) {

    line1[ball] =
      'o';
  }


  line1[0] =
    '|';

  line1[15] =
    '|';


  lcd.setCursor(
    0,
    0
  );

  lcd.print(line0);


  lcd.setCursor(
    0,
    1
  );

  lcd.print(line1);
}


void renderPingPongEnd() {

  if (
    ppStatus ==
    STATUS_WIN
  ) {

    printCentered(
      0,
      "YOU WIN!"
    );

  } else {

    printCentered(
      0,
      "GAME OVER"
    );
  }


  char score[17];


  snprintf(
    score,
    sizeof(score),
    "P:%u AI:%u",
    ppPlayerScore,
    ppAiScore
  );


  printCentered(
    1,
    score
  );
}


void updatePingPong(
  uint32_t now
) {

  if (ppPointAnimation) {

    if (
      now -
      ppPointAnimationStart
      < 350
    ) {

      return;
    }


    ppPointAnimation =
      false;

    ppFlash =
      false;


    resetPingPongBall(
      ppVX
    );


    ppLastStep =
      now;


    renderPingPong();

    return;
  }


  if (
    now - ppLastStep
    < getPingPongSpeed()
  ) {

    return;
  }


  ppLastStep =
    now;


  ppBallX +=
    ppVX;


  ppBallY +=
    ppVY;


  if (
    ppBallY < 0
  ) {

    ppBallY = 0;

    ppVY = 1;
  }


  if (
    ppBallY > 1
  ) {

    ppBallY = 1;

    ppVY = -1;
  }


  // IA suave

  if (
    random(0, 100)
    < 85
  ) {

    ppAiY =
      ppBallY;
  }


  // Jugador

  if (
    ppBallX <= 0
  ) {

    if (
      ppBallY ==
      ppPlayerY
    ) {

      ppBallX = 0;

      ppVX = 1;


      ppVY =
        random(0, 2) == 0
        ? -1
        : 1;

    } else {

      ppAiScore++;


      ppPointAnimation =
        true;


      ppPointAnimationStart =
        now;


      ppFlash =
        true;


      renderPingPong();
    }
  }


  // IA

  else if (
    ppBallX >= 15
  ) {

    if (
      ppBallY ==
      ppAiY
    ) {

      ppBallX = 15;

      ppVX = -1;


      ppVY =
        random(0, 2) == 0
        ? -1
        : 1;

    } else {

      ppPlayerScore++;


      ppPointAnimation =
        true;


      ppPointAnimationStart =
        now;


      ppFlash =
        true;


      renderPingPong();
    }
  }


  if (
    ppPlayerScore >=
    PP_WIN_SCORE
  ) {

    ppStatus =
      STATUS_WIN;

    renderPingPongEnd();

    return;
  }


  if (
    ppAiScore >=
    PP_WIN_SCORE
  ) {

    ppStatus =
      STATUS_GAME_OVER;

    renderPingPongEnd();

    return;
  }


  renderPingPong();
}


void handlePingPong(
  uint32_t now
) {

  if (
    ppStatus !=
    STATUS_RUNNING
  ) {

    if (
      consumePressed(btnOk)
    ) {

      gotoMenu();
    }

    return;
  }


  if (
    consumePressed(btnOk)
  ) {

    gotoMenu();

    return;
  }


  if (
    consumePressed(btnLeft)
  ) {

    ppPlayerY = 0;

    renderPingPong();
  }


  if (
    consumePressed(btnRight)
  ) {

    ppPlayerY = 1;

    renderPingPong();
  }


  updatePingPong(now);
}


// ============================================================
// MASCOTA - ESTADO
// ============================================================

MiniStatus petStatus =
  STATUS_RUNNING;


uint8_t petHunger = 9;

uint8_t petHappy = 9;


uint32_t petStart = 0;

uint32_t petLastHunger = 0;

uint32_t petLastHappy = 0;

uint32_t petLastLCD = 0;


// ============================================================
// MASCOTA - OJOS
// ============================================================

const float CENTRO_IZQ = 43;

const float CENTRO_DER = 85;

const float CENTRO_Y = 32;

const float OJO_ANCHO = 25;

const float OJO_ALTO = 31;


// ============================================================
// ANIMACION MASCOTA
// ============================================================

enum PetAnimation : uint8_t {

  PET_ANIM_NORMAL,

  PET_ANIM_LOOK_LEFT,

  PET_ANIM_LOOK_RIGHT,

  PET_ANIM_BLINK,

  PET_ANIM_HAPPY,

  PET_ANIM_SURPRISE,

  PET_ANIM_WINK,

  PET_ANIM_SLEEP,

  PET_ANIM_WAKE
};


PetAnimation petAnimation =
  PET_ANIM_NORMAL;


uint32_t petAnimationStart = 0;

uint32_t petNextAnimation = 0;

float petEyeOffset = 0;

float petEyeTarget = 0;

float petEyeY = CENTRO_Y;

float petEyeHeight = OJO_ALTO;

float petEyeWidth = OJO_ANCHO;

bool petBlinkClosed = false;

bool petOLEDDirty = true;


// ============================================================
// INTERPOLACION
// ============================================================

float smoothStep(
  float value
) {

  if (value < 0) {
    value = 0;
  }

  if (value > 1) {
    value = 1;
  }


  return value *
         value *
         (3.0 - 2.0 * value);
}


// ============================================================
// DIBUJAR OJO
// ============================================================

void dibujarOjo(
  float x,
  float y,
  float ancho,
  float alto
) {

  if (alto < 2) {
    alto = 2;
  }


  pantalla.fillRoundRect(
    (int)(x - ancho / 2),
    (int)(y - alto / 2),
    (int)ancho,
    (int)alto,
    8,
    SSD1306_WHITE
  );
}


// ============================================================
// DIBUJAR OJOS
// ============================================================

void dibujarDosOjos(
  float offset,
  float ancho,
  float alto,
  float y
) {

  if (!oledDisponible) {
    return;
  }


  pantalla.clearDisplay();


  dibujarOjo(
    CENTRO_IZQ + offset,
    y,
    ancho,
    alto
  );


  dibujarOjo(
    CENTRO_DER + offset,
    y,
    ancho,
    alto
  );


  pantalla.display();
}


// ============================================================
// OJOS FELICES
// ============================================================

void dibujarOjoFeliz(
  int x
) {

  pantalla.drawLine(
    x - 11,
    34,
    x - 6,
    29,
    SSD1306_WHITE
  );


  pantalla.drawLine(
    x - 6,
    29,
    x,
    27,
    SSD1306_WHITE
  );


  pantalla.drawLine(
    x,
    27,
    x + 6,
    29,
    SSD1306_WHITE
  );


  pantalla.drawLine(
    x + 6,
    29,
    x + 11,
    34,
    SSD1306_WHITE
  );
}


// ============================================================
// OJOS NORMALES
// ============================================================

void dibujarOjosNormales() {

  dibujarDosOjos(
    petEyeOffset,
    petEyeWidth,
    petEyeHeight,
    petEyeY
  );
}


// ============================================================
// OJOS CERRADOS
// ============================================================

void dibujarOjosCerrados() {

  pantalla.clearDisplay();


  pantalla.drawLine(
    CENTRO_IZQ - 10,
    32,
    CENTRO_IZQ + 10,
    32,
    SSD1306_WHITE
  );


  pantalla.drawLine(
    CENTRO_DER - 10,
    32,
    CENTRO_DER + 10,
    32,
    SSD1306_WHITE
  );


  pantalla.display();
}


// ============================================================
// INICIAR ANIMACION
// ============================================================

void startPetAnimation(
  PetAnimation animation,
  uint32_t now
) {

  petAnimation =
    animation;


  petAnimationStart =
    now;


  petBlinkClosed =
    false;


  switch (animation) {

    case PET_ANIM_LOOK_LEFT:

      petEyeTarget = -15;

      break;


    case PET_ANIM_LOOK_RIGHT:

      petEyeTarget = 15;

      break;


    case PET_ANIM_BLINK:

      petEyeTarget =
        petEyeOffset;

      break;


    case PET_ANIM_HAPPY:

      petEyeTarget = 0;

      break;


    case PET_ANIM_SURPRISE:

      petEyeTarget = 0;

      break;


    case PET_ANIM_WINK:

      petEyeTarget = 0;

      break;


    case PET_ANIM_SLEEP:

      petEyeTarget = 0;

      break;


    case PET_ANIM_WAKE:

      petEyeTarget = 0;

      break;


    default:

      petEyeTarget = 0;

      break;
  }
}


// ============================================================
// ACTUALIZAR ANIMACION MASCOTA
// ============================================================

void updatePetAnimation(
  uint32_t now
) {

  if (!oledDisponible) {
    return;
  }


  uint32_t elapsed =
    now - petAnimationStart;


  // ==========================================================
  // NORMAL
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_NORMAL
  ) {

    petEyeOffset +=
      (petEyeTarget -
       petEyeOffset) *
      0.08;


    petEyeY +=
      (CENTRO_Y -
       petEyeY) *
      0.08;


    petEyeHeight +=
      (OJO_ALTO -
       petEyeHeight) *
      0.10;


    petEyeWidth +=
      (OJO_ANCHO -
       petEyeWidth) *
      0.10;


    dibujarOjosNormales();


    if (
      now >=
      petNextAnimation
    ) {

      uint8_t action =
        random(0, 100);


      if (
        action < 35
      ) {

        startPetAnimation(
          PET_ANIM_LOOK_LEFT,
          now
        );

      } else if (
        action < 70
      ) {

        startPetAnimation(
          PET_ANIM_LOOK_RIGHT,
          now
        );

      } else {

        startPetAnimation(
          PET_ANIM_BLINK,
          now
        );
      }
    }


    return;
  }


  // ==========================================================
  // MIRAR IZQUIERDA
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_LOOK_LEFT
  ) {

    float progress =
      (float)elapsed /
      450.0;


    if (progress > 1) {
      progress = 1;
    }


    float suave =
      smoothStep(progress);


    float offset =
      petEyeOffset +
      (-15 - petEyeOffset)
      * suave;


    dibujarDosOjos(
      offset,
      OJO_ANCHO,
      OJO_ALTO,
      CENTRO_Y
    );


    if (
      elapsed >= 450
    ) {

      petEyeOffset = -15;

      petAnimationStart =
        now;

      petAnimation =
        PET_ANIM_NORMAL;

      petNextAnimation =
        now + random(500, 1600);
    }


    return;
  }


  // ==========================================================
  // MIRAR DERECHA
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_LOOK_RIGHT
  ) {

    float progress =
      (float)elapsed /
      450.0;


    if (progress > 1) {
      progress = 1;
    }


    float suave =
      smoothStep(progress);


    float offset =
      petEyeOffset +
      (15 - petEyeOffset)
      * suave;


    dibujarDosOjos(
      offset,
      OJO_ANCHO,
      OJO_ALTO,
      CENTRO_Y
    );


    if (
      elapsed >= 450
    ) {

      petEyeOffset = 15;

      petAnimationStart =
        now;

      petAnimation =
        PET_ANIM_NORMAL;

      petNextAnimation =
        now + random(500, 1600);
    }


    return;
  }


  // ==========================================================
  // PARPADEO
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_BLINK
  ) {

    if (
      elapsed < 120
    ) {

      float progress =
        (float)elapsed /
        120.0;


      float suave =
        smoothStep(progress);


      float height =
        OJO_ALTO -
        (OJO_ALTO - 2)
        * suave;


      dibujarDosOjos(
        petEyeOffset,
        OJO_ANCHO,
        height,
        CENTRO_Y
      );


    } else if (
      elapsed < 220
    ) {

      dibujarOjosCerrados();


    } else if (
      elapsed < 340
    ) {

      float progress =
        (float)(
          elapsed - 220
        ) / 120.0;


      float suave =
        smoothStep(progress);


      float height =
        2 +
        (OJO_ALTO - 2)
        * suave;


      dibujarDosOjos(
        petEyeOffset,
        OJO_ANCHO,
        height,
        CENTRO_Y
      );


    } else {

      petAnimation =
        PET_ANIM_NORMAL;


      petAnimationStart =
        now;


      petNextAnimation =
        now + random(700, 2000);
    }


    return;
  }


  // ==========================================================
  // FELIZ
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_HAPPY
  ) {

    if (
      elapsed < 350
    ) {

      float progress =
        (float)elapsed /
        350.0;


      float suave =
        smoothStep(progress);


      float height =
        OJO_ALTO -
        (OJO_ALTO - 8)
        * suave;


      float y =
        CENTRO_Y +
        3 * suave;


      dibujarDosOjos(
        0,
        OJO_ANCHO,
        height,
        y
      );


    } else if (
      elapsed < 1000
    ) {

      pantalla.clearDisplay();


      dibujarOjoFeliz(
        CENTRO_IZQ
      );


      dibujarOjoFeliz(
        CENTRO_DER
      );


      pantalla.display();


    } else {

      petEyeOffset = 0;

      petAnimation =
        PET_ANIM_NORMAL;

      petAnimationStart =
        now;

      petNextAnimation =
        now + 1200;
    }


    return;
  }


  // ==========================================================
  // SORPRESA
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_SURPRISE
  ) {

    if (
      elapsed < 350
    ) {

      float progress =
        (float)elapsed /
        350.0;


      float suave =
        smoothStep(progress);


      float width =
        OJO_ANCHO +
        7 * suave;


      float height =
        OJO_ALTO +
        8 * suave;


      dibujarDosOjos(
        0,
        width,
        height,
        CENTRO_Y
      );


    } else if (
      elapsed < 650
    ) {

      dibujarDosOjos(
        0,
        OJO_ANCHO + 7,
        OJO_ALTO + 8,
        CENTRO_Y
      );


    } else {

      petAnimation =
        PET_ANIM_NORMAL;

      petAnimationStart =
        now;

      petNextAnimation =
        now + 1000;
    }


    return;
  }


  // ==========================================================
  // GUIÑO
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_WINK
  ) {

    pantalla.clearDisplay();


    if (
      elapsed < 200
    ) {

      float progress =
        (float)elapsed /
        200.0;


      float suave =
        smoothStep(progress);


      float height =
        OJO_ALTO -
        (OJO_ALTO - 3)
        * suave;


      dibujarOjo(
        CENTRO_IZQ,
        CENTRO_Y,
        OJO_ANCHO,
        height
      );


      dibujarOjo(
        CENTRO_DER,
        CENTRO_Y,
        OJO_ANCHO,
        OJO_ALTO
      );


    } else if (
      elapsed < 450
    ) {

      dibujarOjo(
        CENTRO_IZQ,
        CENTRO_Y,
        OJO_ANCHO,
        3
      );


      dibujarOjo(
        CENTRO_DER,
        CENTRO_Y,
        OJO_ANCHO,
        OJO_ALTO
      );


    } else if (
      elapsed < 650
    ) {

      float progress =
        (float)(
          elapsed - 450
        ) / 200.0;


      float suave =
        smoothStep(progress);


      float height =
        3 +
        (OJO_ALTO - 3)
        * suave;


      dibujarOjo(
        CENTRO_IZQ,
        CENTRO_Y,
        OJO_ANCHO,
        height
      );


      dibujarOjo(
        CENTRO_DER,
        CENTRO_Y,
        OJO_ANCHO,
        OJO_ALTO
      );


    } else {

      petAnimation =
        PET_ANIM_NORMAL;

      petAnimationStart =
        now;

      petNextAnimation =
        now + 1000;
    }


    pantalla.display();

    return;
  }


  // ==========================================================
  // DORMIR
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_SLEEP
  ) {

    if (
      elapsed < 600
    ) {

      float progress =
        (float)elapsed /
        600.0;


      float suave =
        smoothStep(progress);


      float height =
        OJO_ALTO -
        (OJO_ALTO - 2)
        * suave;


      dibujarDosOjos(
        0,
        OJO_ANCHO,
        height,
        CENTRO_Y
      );


    } else {

      dibujarOjosCerrados();
    }


    return;
  }


  // ==========================================================
  // DESPERTAR
  // ==========================================================

  if (
    petAnimation ==
    PET_ANIM_WAKE
  ) {

    if (
      elapsed < 700
    ) {

      float progress =
        (float)elapsed /
        700.0;


      float suave =
        smoothStep(progress);


      float height =
        2 +
        (OJO_ALTO - 2)
        * suave;


      dibujarDosOjos(
        0,
        OJO_ANCHO,
        height,
        CENTRO_Y
      );


    } else {

      petAnimation =
        PET_ANIM_BLINK;

      petAnimationStart =
        now;
    }
  }
}


// ============================================================
// MASCOTA - LCD
// ============================================================

void renderMascotaLCD(
  uint32_t now
) {

  char line0[17];

  char line1[17];


  uint16_t seconds =
    (uint16_t)(
      (now - petStart)
      / 1000UL
    );


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


  printLine(
    0,
    line0
  );


  printLine(
    1,
    line1
  );
}


// ============================================================
// MASCOTA - FINAL
// ============================================================

void renderMascotaEnd(
  uint32_t now
) {

  uint16_t score =
    (uint16_t)(
      (now - petStart)
      / 1000UL
    );


  if (
    petStatus ==
    STATUS_WIN
  ) {

    printCentered(
      0,
      "PET WIN!"
    );

  } else {

    printCentered(
      0,
      "PET GAME OVER"
    );
  }


  char line[17];


  snprintf(
    line,
    sizeof(line),
    "Score: %us",
    score
  );


  printCentered(
    1,
    line
  );
}


// ============================================================
// INICIAR MASCOTA
// ============================================================

void startMascota() {

  appState =
    STATE_MASCOTA;


  petStatus =
    STATUS_RUNNING;


  petHunger = 9;

  petHappy = 9;


  petStart =
    millis();


  petLastHunger =
    petStart;


  petLastHappy =
    petStart;


  petLastLCD =
    0;


  petEyeOffset =
    0;

  petEyeTarget =
    0;

  petEyeY =
    CENTRO_Y;

  petEyeHeight =
    OJO_ALTO;

  petEyeWidth =
    OJO_ANCHO;


  petAnimation =
    PET_ANIM_NORMAL;


  petAnimationStart =
    millis();


  petNextAnimation =
    millis() + 1200;


  lcd.clear();


  renderMascotaLCD(
    petStart
  );


  if (oledDisponible) {

    pantalla.clearDisplay();

    pantalla.display();
  }
}


// ============================================================
// MASCOTA - LOOP
// ============================================================

void handleMascota(
  uint32_t now
) {

  // ----------------------------------------------------------
  // OK
  // ----------------------------------------------------------

  if (
    consumePressed(btnOk)
  ) {

    gotoMenu();

    return;
  }


  // ----------------------------------------------------------
  // FIN
  // ----------------------------------------------------------

  if (
    petStatus !=
    STATUS_RUNNING
  ) {

    updatePetAnimation(now);

    return;
  }


  // ----------------------------------------------------------
  // ALIMENTAR
  // ----------------------------------------------------------

  if (
    consumePressed(btnLeft)
  ) {

    if (
      petHunger <= 7
    ) {

      petHunger += 2;

    } else {

      petHunger = 9;
    }


    if (oledDisponible) {

      startPetAnimation(
        PET_ANIM_HAPPY,
        now
      );
    }
  }


  // ----------------------------------------------------------
  // JUGAR
  // ----------------------------------------------------------

  if (
    consumePressed(btnRight)
  ) {

    if (
      petHappy <= 7
    ) {

      petHappy += 2;

    } else {

      petHappy = 9;
    }


    if (oledDisponible) {

      startPetAnimation(
        PET_ANIM_LOOK_LEFT,
        now
      );
    }
  }


  // ----------------------------------------------------------
  // HAMBRE
  // ----------------------------------------------------------

  if (
    now - petLastHunger
    >= PET_HUNGER_DECAY_MS
  ) {

    petLastHunger +=
      PET_HUNGER_DECAY_MS;


    if (
      petHunger > 0
    ) {

      petHunger--;
    }
  }


  // ----------------------------------------------------------
  // FELICIDAD
  // ----------------------------------------------------------

  if (
    now - petLastHappy
    >= PET_HAPPY_DECAY_MS
  ) {

    petLastHappy +=
      PET_HAPPY_DECAY_MS;


    if (
      petHappy > 0
    ) {

      petHappy--;
    }
  }


  // ----------------------------------------------------------
  // GAME OVER
  // ----------------------------------------------------------

  if (
    petHunger == 0 ||
    petHappy == 0
  ) {

    petStatus =
      STATUS_GAME_OVER;


    renderMascotaEnd(now);


    if (oledDisponible) {

      startPetAnimation(
        PET_ANIM_SLEEP,
        now
      );
    }


    return;
  }


  // ----------------------------------------------------------
  // GANAR
  // ----------------------------------------------------------

  if (
    now - petStart
    >= PET_SURVIVE_WIN_MS
  ) {

    petStatus =
      STATUS_WIN;


    renderMascotaEnd(now);


    if (oledDisponible) {

      startPetAnimation(
        PET_ANIM_HAPPY,
        now
      );
    }


    return;
  }


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  updatePetAnimation(now);


  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  if (
    now - petLastLCD
    >= 250
  ) {

    petLastLCD =
      now;


    renderMascotaLCD(now);
  }
}


// ============================================================
// BLACKJACK
// ============================================================

#define BJ_TARGET_WINS 3
#define BJ_TARGET_LOSSES 3

#define BJ_DEAL_STEP 550UL


enum BJPhase : uint8_t {

  BJ_DEALING,

  BJ_PLAYER_TURN,

  BJ_DEALER_TURN,

  BJ_ROUND_OVER,

  BJ_MATCH_WIN,

  BJ_MATCH_LOSE
};


BJPhase bjPhase =
  BJ_PLAYER_TURN;


uint8_t bjPlayer = 0;

uint8_t bjDealer = 0;

uint8_t bjDealerShown = 0;

uint8_t bjWins = 0;

uint8_t bjLosses = 0;


uint8_t bjDealStep = 0;

uint32_t bjLastDeal = 0;

uint32_t bjDealerStep = 0;

char bjMessage[17];


uint8_t randomCard() {

  return random(
    1,
    11
  );
}


// ============================================================
// NUEVA RONDA
// ============================================================

void bjStartRound() {

  bjPlayer = 0;

  bjDealer = 0;

  bjDealerShown = 0;


  bjDealStep = 0;


  bjLastDeal =
    millis();


  bjPhase =
    BJ_DEALING;


  strcpy(
    bjMessage,
    "Dealing..."
  );


  lcd.clear();


  renderBlackjack();
}


// ============================================================
// START BLACKJACK
// ============================================================

void startBlackjack() {

  appState =
    STATE_BLACKJACK;


  bjWins = 0;

  bjLosses = 0;


  bjStartRound();
}


// ============================================================
// FINAL DE RONDA
// ============================================================

void bjEndRound(
  const char *message,
  bool playerWon
) {

  strncpy(
    bjMessage,
    message,
    16
  );


  bjMessage[16] =
    '\0';


  if (playerWon) {

    bjWins++;

  } else {

    bjLosses++;
  }


  if (
    bjWins >=
    BJ_TARGET_WINS
  ) {

    bjPhase =
      BJ_MATCH_WIN;

  } else if (
    bjLosses >=
    BJ_TARGET_LOSSES
  ) {

    bjPhase =
      BJ_MATCH_LOSE;

  } else {

    bjPhase =
      BJ_ROUND_OVER;
  }


  renderBlackjack();
}


// ============================================================
// RENDER BLACKJACK
// ============================================================

void renderBlackjack() {

  char line0[17];

  char line1[17];


  if (
    bjPhase ==
    BJ_DEALING
  ) {

    snprintf(
      line0,
      sizeof(line0),
      "P:%u D:%s",
      bjPlayer,
      bjDealerShown > 0
      ? "?"
      : "?"
    );


    snprintf(
      line1,
      sizeof(line1),
      "Dealing..."
    );


  } else if (
    bjPhase ==
    BJ_PLAYER_TURN
  ) {

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


  } else if (
    bjPhase ==
    BJ_DEALER_TURN
  ) {

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


  } else if (
    bjPhase ==
    BJ_ROUND_OVER
  ) {

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


  } else if (
    bjPhase ==
    BJ_MATCH_WIN
  ) {

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


  printLine(
    0,
    line0
  );


  printLine(
    1,
    line1
  );
}


// ============================================================
// REPARTIR CARTAS
// ============================================================

void updateBlackjackDealing(
  uint32_t now
) {

  if (
    now - bjLastDeal
    < BJ_DEAL_STEP
  ) {

    return;
  }


  bjLastDeal =
    now;


  if (
    bjDealStep == 0
  ) {

    bjPlayer =
      randomCard();

    bjDealStep++;

    renderBlackjack();

    return;
  }


  if (
    bjDealStep == 1
  ) {

    bjDealerShown =
      randomCard();

    bjDealer =
      bjDealerShown;

    bjDealStep++;

    renderBlackjack();

    return;
  }


  if (
    bjDealStep == 2
  ) {

    bjPlayer +=
      randomCard();

    bjDealStep++;

    renderBlackjack();

    return;
  }


  if (
    bjDealStep == 3
  ) {

    bjDealer +=
      randomCard();

    bjDealStep++;


    bjPhase =
      BJ_PLAYER_TURN;


    renderBlackjack();

    return;
  }
}


// ============================================================
// DEALER
// ============================================================

void updateBlackjackDealer(
  uint32_t now
) {

  if (
    bjPhase !=
    BJ_DEALER_TURN
  ) {

    return;
  }


  if (
    now - bjDealerStep
    < BJ_DEAL_STEP
  ) {

    return;
  }


  bjDealerStep =
    now;


  if (
    bjDealer < 17
  ) {

    bjDealer +=
      randomCard();


    renderBlackjack();

    return;
  }


  if (
    bjDealer > 21
  ) {

    bjEndRound(
      "Dealer Bust",
      true
    );


  } else if (
    bjPlayer > bjDealer
  ) {

    bjEndRound(
      "YOU WIN",
      true
    );


  } else if (
    bjPlayer < bjDealer
  ) {

    bjEndRound(
      "YOU LOSE",
      false
    );


  } else {

    strcpy(
      bjMessage,
      "PUSH"
    );


    bjPhase =
      BJ_ROUND_OVER;


    renderBlackjack();
  }
}


// ============================================================
// HANDLE BLACKJACK
// ============================================================

void handleBlackjack(
  uint32_t now
) {

  // ----------------------------------------------------------
  // RONDA EN REPARTO
  // ----------------------------------------------------------

  if (
    bjPhase ==
    BJ_DEALING
  ) {

    if (
      consumePressed(btnOk)
    ) {

      gotoMenu();

      return;
    }


    updateBlackjackDealing(
      now
    );


    return;
  }


  // ----------------------------------------------------------
  // RESULTADO
  // ----------------------------------------------------------

  if (
    bjPhase ==
    BJ_ROUND_OVER
  ) {

    if (
      consumePressed(btnOk)
    ) {

      bjStartRound();
    }


    return;
  }


  // ----------------------------------------------------------
  // MATCH TERMINADO
  // ----------------------------------------------------------

  if (
    bjPhase ==
    BJ_MATCH_WIN ||
    bjPhase ==
    BJ_MATCH_LOSE
  ) {

    if (
      consumePressed(btnOk)
    ) {

      gotoMenu();
    }


    return;
  }


  // ----------------------------------------------------------
  // SALIR
  // ----------------------------------------------------------

  if (
    consumePressed(btnOk)
  ) {

    gotoMenu();

    return;
  }


  // ----------------------------------------------------------
  // TURNO DEL JUGADOR
  // ----------------------------------------------------------

  if (
    bjPhase ==
    BJ_PLAYER_TURN
  ) {

    // HIT

    if (
      consumePressed(btnLeft)
    ) {

      bjPlayer +=
        randomCard();


      if (
        bjPlayer > 21
      ) {

        bjEndRound(
          "BUST! LOSE",
          false
        );

      } else {

        renderBlackjack();
      }
    }


    // STAND

    if (
      consumePressed(btnRight)
    ) {

      bjPhase =
        BJ_DEALER_TURN;


      bjDealerStep =
        now;


      renderBlackjack();
    }
  }


  updateBlackjackDealer(
    now
  );
}


// ============================================================
// SNAKE
// ============================================================

#define SNAKE_WIN_LENGTH 12

#define SNAKE_BASE_STEP 650UL

#define SNAKE_MIN_STEP 230UL


MiniStatus snakeStatus =
  STATUS_RUNNING;


uint8_t snakeBody[32];

uint8_t snakeLen = 3;

uint8_t snakeDir = 1;

uint8_t snakeFood = 10;


uint32_t snakeLastStep = 0;

uint32_t snakeFoodAnimation = 0;

bool snakeFoodVisible = true;


bool snakeCharsCreated =
  false;


// ============================================================
// CHARACTERS
// ============================================================

void createSnakeCharacters() {

  if (
    snakeCharsCreated
  ) {

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


  lcd.createChar(
    0,
    headChar
  );


  lcd.createChar(
    1,
    bodyChar
  );


  lcd.createChar(
    2,
    foodChar
  );


  snakeCharsCreated =
    true;
}


// ============================================================
// FOOD
// ============================================================

bool snakeCellUsed(
  uint8_t cell
) {

  for (
    uint8_t i = 0;
    i < snakeLen;
    i++
  ) {

    if (
      snakeBody[i] ==
      cell
    ) {

      return true;
    }
  }


  return false;
}


void placeSnakeFood() {

  if (
    snakeLen >= 32
  ) {

    snakeFood = 0;

    return;
  }


  do {

    snakeFood =
      random(0, 32);

  } while (
    snakeCellUsed(
      snakeFood
    )
  );
}


// ============================================================
// START SNAKE
// ============================================================

void startSnake() {

  appState =
    STATE_SNAKE;


  snakeStatus =
    STATUS_RUNNING;


  snakeLen = 3;

  snakeDir = 1;


  snakeBody[0] = 2;

  snakeBody[1] = 1;

  snakeBody[2] = 0;


  placeSnakeFood();


  snakeLastStep =
    millis();


  snakeFoodAnimation =
    millis();


  snakeFoodVisible =
    true;


  lcd.clear();


  createSnakeCharacters();


  renderSnake();
}


// ============================================================
// VELOCIDAD
// ============================================================

uint16_t snakeStepTime() {

  int16_t speed =
    (int16_t)
    SNAKE_BASE_STEP -
    (int16_t)
    (snakeLen - 3)
    * 35;


  if (
    speed <
    SNAKE_MIN_STEP
  ) {

    speed =
      SNAKE_MIN_STEP;
  }


  return (
    uint16_t
  )speed;
}


// ============================================================
// UPDATE SNAKE
// ============================================================

void updateSnake() {

  int8_t headX =
    snakeBody[0] % 16;


  int8_t headY =
    snakeBody[0] / 16;


  if (
    snakeDir == 0
  ) {

    headY--;

  } else if (
    snakeDir == 1
  ) {

    headX++;

  } else if (
    snakeDir == 2
  ) {

    headY++;

  } else {

    headX--;
  }


  // Pared

  if (
    headX < 0 ||
    headX > 15 ||
    headY < 0 ||
    headY > 1
  ) {

    snakeStatus =
      STATUS_GAME_OVER;

    return;
  }


  uint8_t newHead =
    (uint8_t)
    (headY * 16 + headX);


  bool grow =
    newHead ==
    snakeFood;


  uint8_t checkLength =
    snakeLen;


  if (
    !grow &&
    checkLength > 0
  ) {

    checkLength--;
  }


  // Cuerpo

  for (
    uint8_t i = 0;
    i < checkLength;
    i++
  ) {

    if (
      snakeBody[i] ==
      newHead
    ) {

      snakeStatus =
        STATUS_GAME_OVER;

      return;
    }
  }


  // Crecer

  if (
    grow &&
    snakeLen < 32
  ) {

    snakeLen++;
  }


  // Mover cuerpo

  for (
    int8_t i =
      snakeLen - 1;
    i > 0;
    i--
  ) {

    snakeBody[i] =
      snakeBody[i - 1];
  }


  snakeBody[0] =
    newHead;


  // Comida

  if (grow) {

    if (
      snakeLen >=
      SNAKE_WIN_LENGTH
    ) {

      snakeStatus =
        STATUS_WIN;

    } else {

      placeSnakeFood();
    }
  }
}


// ============================================================
// RENDER SNAKE
// ============================================================

void renderSnake() {

  if (
    snakeStatus !=
    STATUS_RUNNING
  ) {

    lcd.clear();


    if (
      snakeStatus ==
      STATUS_WIN
    ) {

      printCentered(
        0,
        "GANASTE!"
      );

    } else {

      printCentered(
        0,
        "GAME OVER"
      );
    }


    char score[17];


    snprintf(
      score,
      sizeof(score),
      "Score: %u",
      snakeLen - 3
    );


    printCentered(
      1,
      score
    );


    return;
  }


  lcd.clear();


  // Comida

  if (
    snakeFoodVisible
  ) {

    uint8_t foodX =
      snakeFood % 16;


    uint8_t foodY =
      snakeFood / 16;


    lcd.setCursor(
      foodX,
      foodY
    );


    lcd.write(
      byte(2)
    );
  }


  // Cuerpo

  for (
    uint8_t i = 1;
    i < snakeLen;
    i++
  ) {

    uint8_t x =
      snakeBody[i] % 16;


    uint8_t y =
      snakeBody[i] / 16;


    lcd.setCursor(
      x,
      y
    );


    lcd.write(
      byte(1)
    );
  }


  // Cabeza

  uint8_t headX =
    snakeBody[0] % 16;


  uint8_t headY =
    snakeBody[0] / 16;


  lcd.setCursor(
    headX,
    headY
  );


  lcd.write(
    byte(0)
  );
}


// ============================================================
// HANDLE SNAKE
// ============================================================

void handleSnake(
  uint32_t now
) {

  // ----------------------------------------------------------
  // FIN
  // ----------------------------------------------------------

  if (
    snakeStatus !=
    STATUS_RUNNING
  ) {

    if (
      consumePressed(btnOk)
    ) {

      gotoMenu();
    }


    return;
  }


  // ----------------------------------------------------------
  // SALIR
  // ----------------------------------------------------------

  if (
    consumePressed(btnOk)
  ) {

    gotoMenu();

    return;
  }


  // ----------------------------------------------------------
  // GIRAR IZQUIERDA
  // ----------------------------------------------------------

  if (
    consumePressed(btnLeft)
  ) {

    snakeDir =
      (snakeDir + 3) % 4;
  }


  // ----------------------------------------------------------
  // GIRAR DERECHA
  // ----------------------------------------------------------

  if (
    consumePressed(btnRight)
  ) {

    snakeDir =
      (snakeDir + 1) % 4;
  }


  // ----------------------------------------------------------
  // COMIDA PARPADEANDO
  // ----------------------------------------------------------

  if (
    now - snakeFoodAnimation
    >= 250
  ) {

    snakeFoodAnimation =
      now;


    snakeFoodVisible =
      !snakeFoodVisible;


    renderSnake();
  }


  // ----------------------------------------------------------
  // MOVIMIENTO
  // ----------------------------------------------------------

  if (
    now - snakeLastStep
    >= snakeStepTime()
  ) {

    snakeLastStep =
      now;


    updateSnake();


    if (
      snakeStatus ==
      STATUS_RUNNING
    ) {

      snakeFoodVisible =
        true;


      renderSnake();

    } else {

      renderSnake();
    }
  }
}


// ============================================================
// SETUP
// ============================================================

void setup() {

  // ----------------------------------------------------------
  // BOTONES
  // ----------------------------------------------------------

  pinMode(
    PIN_BTN_LEFT,
    INPUT_PULLUP
  );


  pinMode(
    PIN_BTN_RIGHT,
    INPUT_PULLUP
  );


  pinMode(
    PIN_BTN_OK,
    INPUT_PULLUP
  );


  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin();


  // ----------------------------------------------------------
  // LCD
  // ----------------------------------------------------------

  lcd.init();

  lcd.backlight();


  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (
    pantalla.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    oledDisponible =
      true;


    pantalla.clearDisplay();

    pantalla.display();


    Wire.setClock(
      400000
    );
  }


  // ----------------------------------------------------------
  // RANDOM
  // ----------------------------------------------------------

  randomSeed(
    analogRead(A0) ^
    micros()
  );


  // ----------------------------------------------------------
  // INICIO LCD
  // ----------------------------------------------------------

  lcd.clear();


  printCentered(
    0,
    "TinyPocket"
  );


  printCentered(
    1,
    "Mini Consola"
  );


  // ----------------------------------------------------------
  // INICIO OLED
  // ----------------------------------------------------------

  if (oledDisponible) {

    pantalla.clearDisplay();


    dibujarDosOjos(
      0,
      OJO_ANCHO,
      OJO_ALTO,
      CENTRO_Y
    );
  }


  // No usamos delay.
  // Esperamos desde millis().


  uint32_t start =
    millis();


  while (
    millis() - start
    < 1000
  ) {

    uint32_t now =
      millis();


    updateButtons(now);
  }


  // ----------------------------------------------------------
  // MENU
  // ----------------------------------------------------------

  renderMenu();
}


// ============================================================
// LOOP PRINCIPAL
// ============================================================

void loop() {

  uint32_t now =
    millis();


  // ----------------------------------------------------------
  // BOTONES
  // ----------------------------------------------------------

  updateButtons(now);


  // ----------------------------------------------------------
  // ESTADO ACTUAL
  // ----------------------------------------------------------

  switch (appState) {

    case STATE_MENU:

      handleMenu(now);

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