#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// PING PONG - OLED 128x64
// 1 sola paleta abajo
// La pelota rebota arriba y contra los laterales
// Animaciones no bloqueantes
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 pantalla(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ============================================================
// TIEMPO
// ============================================================

const unsigned long FRAME_TIME = 16;
unsigned long ultimoFrame = 0;

// ============================================================
// PALETA
// ============================================================

const float PADDLE_WIDTH = 28.0;
const float PADDLE_HEIGHT = 3.0;
const float PADDLE_Y = 58.0;

float paletaX = 64.0;
float paletaObjetivoX = 64.0;

// ============================================================
// PELOTA
// ============================================================

const float BALL_RADIUS = 2.5;

float pelotaX = 64.0;
float pelotaY = 45.0;

float pelotaAnteriorX = 64.0;
float pelotaAnteriorY = 45.0;

float velocidadX = 0.85;
float velocidadY = -1.25;

// ============================================================
// TRAIL
// ============================================================

const int TRAIL_LENGTH = 8;

struct Punto {
  float x;
  float y;
};

Punto trail[TRAIL_LENGTH];

// ============================================================
// ANIMACION DE REBOTE
// ============================================================

float reboteAnimacion = 0.0;

// ============================================================
// INICIALIZAR TRAIL
// ============================================================

void inicializarTrail() {

  for (int i = 0; i < TRAIL_LENGTH; i++) {

    trail[i].x = pelotaX;
    trail[i].y = pelotaY;
  }
}

// ============================================================
// ACTUALIZAR TRAIL
// ============================================================

void actualizarTrail() {

  for (int i = TRAIL_LENGTH - 1; i > 0; i--) {

    trail[i] = trail[i - 1];
  }

  trail[0].x = pelotaX;
  trail[0].y = pelotaY;
}

// ============================================================
// ACTUALIZAR PALETA
// ============================================================

void actualizarPaleta() {

  // Movimiento automatico para probar el juego.
  // Luego podemos cambiarlo por botones.

  float tiempo =
    millis() / 1000.0;

  paletaObjetivoX =
    64.0 +
    sin(tiempo * 1.35) * 42.0;

  paletaX +=
    (paletaObjetivoX - paletaX) * 0.15;

  if (paletaX < PADDLE_WIDTH / 2) {

    paletaX =
      PADDLE_WIDTH / 2;
  }

  if (paletaX >
      SCREEN_WIDTH - PADDLE_WIDTH / 2) {

    paletaX =
      SCREEN_WIDTH - PADDLE_WIDTH / 2;
  }
}

// ============================================================
// REBOTE LATERAL
// ============================================================

void comprobarParedesLaterales() {

  if (pelotaX - BALL_RADIUS <= 1) {

    pelotaX =
      1 + BALL_RADIUS;

    velocidadX =
      fabs(velocidadX);

    reboteAnimacion = 1.0;
  }

  if (pelotaX + BALL_RADIUS >=
      SCREEN_WIDTH - 1) {

    pelotaX =
      SCREEN_WIDTH - 1 - BALL_RADIUS;

    velocidadX =
      -fabs(velocidadX);

    reboteAnimacion = 1.0;
  }
}

// ============================================================
// REBOTE ARRIBA
// ============================================================

void comprobarParedSuperior() {

  if (pelotaY - BALL_RADIUS <= 1) {

    pelotaY =
      1 + BALL_RADIUS;

    velocidadY =
      fabs(velocidadY);

    reboteAnimacion = 1.0;
  }
}

// ============================================================
// REBOTE PALETA
// ============================================================

void comprobarPaleta() {

  float izquierda =
    paletaX - PADDLE_WIDTH / 2;

  float derecha =
    paletaX + PADDLE_WIDTH / 2;

  float arriba =
    PADDLE_Y;

  float abajo =
    PADDLE_Y + PADDLE_HEIGHT;

  bool tocaPaleta =
    pelotaX + BALL_RADIUS >= izquierda &&
    pelotaX - BALL_RADIUS <= derecha &&
    pelotaY + BALL_RADIUS >= arriba &&
    pelotaY - BALL_RADIUS <= abajo;

  // Solo rebotar cuando la pelota viene hacia abajo.
  if (tocaPaleta && velocidadY > 0) {

    pelotaY =
      arriba - BALL_RADIUS - 0.5;

    // Rebote hacia arriba.
    velocidadY =
      -fabs(velocidadY);

    // Cambiar direccion horizontal dependiendo
    // de donde golpeo la pelota.
    float diferencia =
      pelotaX - paletaX;

    velocidadX =
      diferencia * 0.08;

    // Evitar trayectoria demasiado vertical.
    if (fabs(velocidadX) < 0.35) {

      if (velocidadX >= 0) {

        velocidadX = 0.35;

      } else {

        velocidadX = -0.35;
      }
    }

    reboteAnimacion = 1.0;
  }
}

// ============================================================
// REINICIAR PELOTA
// ============================================================

void reiniciarPelota() {

  pelotaX = paletaX;

  pelotaY =
    PADDLE_Y - 7;

  pelotaAnteriorX =
    pelotaX;

  pelotaAnteriorY =
    pelotaY;

  velocidadX = 0.85;

  velocidadY = -1.25;

  inicializarTrail();
}

// ============================================================
// ACTUALIZAR PELOTA
// ============================================================

void actualizarPelota() {

  // Guardar posicion anterior.
  pelotaAnteriorX =
    pelotaX;

  pelotaAnteriorY =
    pelotaY;

  // Actualizar trail.
  actualizarTrail();

  // Mover pelota.
  pelotaX +=
    velocidadX;

  pelotaY +=
    velocidadY;

  // Rebotes.
  comprobarParedesLaterales();

  comprobarParedSuperior();

  comprobarPaleta();

  // Animacion.
  if (reboteAnimacion > 0) {

    reboteAnimacion -= 0.08;

    if (reboteAnimacion < 0) {

      reboteAnimacion = 0;
    }
  }

  // Si la pelota pasa la paleta,
  // reiniciar el juego.
  if (pelotaY >
      SCREEN_HEIGHT + 5) {

    reiniciarPelota();
  }
}

// ============================================================
// DIBUJAR TRAIL
// ============================================================

void dibujarTrail() {

  for (int i = TRAIL_LENGTH - 1;
       i >= 0;
       i--) {

    int radio = 1;

    if (i < 2) {
      radio = 2;
    }

    pantalla.fillCircle(
      trail[i].x,
      trail[i].y,
      radio,
      SSD1306_WHITE
    );
  }
}

// ============================================================
// DIBUJAR PALETA
// ============================================================

void dibujarPaleta() {

  int izquierda =
    paletaX -
    PADDLE_WIDTH / 2;

  pantalla.fillRoundRect(
    izquierda,
    PADDLE_Y,
    PADDLE_WIDTH,
    PADDLE_HEIGHT,
    1,
    SSD1306_WHITE
  );
}

// ============================================================
// DIBUJAR PELOTA
// ============================================================

void dibujarPelota() {

  int radio =
    BALL_RADIUS;

  if (reboteAnimacion > 0.5) {

    radio = 3;
  }

  pantalla.fillCircle(
    pelotaX,
    pelotaY,
    radio,
    SSD1306_WHITE
  );
}

// ============================================================
// DIBUJAR JUEGO
// ============================================================

void dibujarJuego() {

  pantalla.clearDisplay();

  // Pared izquierda.
  pantalla.drawFastVLine(
    0,
    0,
    SCREEN_HEIGHT,
    SSD1306_WHITE
  );

  // Pared derecha.
  pantalla.drawFastVLine(
    SCREEN_WIDTH - 1,
    0,
    SCREEN_HEIGHT,
    SSD1306_WHITE
  );

  // Pared superior.
  pantalla.drawFastHLine(
    0,
    0,
    SCREEN_WIDTH,
    SSD1306_WHITE
  );

  dibujarTrail();

  dibujarPaleta();

  dibujarPelota();

  pantalla.display();
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Wire.begin();

  Wire.setClock(400000);

  if (!pantalla.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
      )) {

    while (true) {
    }
  }

  pantalla.clearDisplay();

  pantalla.display();

  inicializarTrail();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long ahora =
    millis();

  if (ahora - ultimoFrame >= FRAME_TIME) {

    ultimoFrame = ahora;

    actualizarPaleta();

    actualizarPelota();

    dibujarJuego();
  }
}