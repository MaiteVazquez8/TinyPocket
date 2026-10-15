#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// BREAKOUT - OLED 128x64
// 1 paleta abajo
// Pelota rebota contra paredes, bloques y paleta
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

const float PADDLE_WIDTH = 26.0;
const float PADDLE_HEIGHT = 3.0;
const float PADDLE_Y = 58.0;

float paletaX = 64.0;
float paletaObjetivoX = 64.0;

// ============================================================
// PELOTA
// ============================================================

const float BALL_RADIUS = 2.5;

float pelotaX = 64.0;
float pelotaY = 48.0;

float pelotaAnteriorX = 64.0;
float pelotaAnteriorY = 48.0;

float velocidadX = 0.85;
float velocidadY = -1.25;

// ============================================================
// TRAIL DE LA PELOTA
// ============================================================

const int TRAIL_LENGTH = 7;

struct Punto {
  float x;
  float y;
};

Punto trail[TRAIL_LENGTH];

// ============================================================
// BLOQUES
// ============================================================

const int COLUMNAS_BLOQUES = 8;
const int FILAS_BLOQUES = 4;

const float BLOQUE_ANCHO = 13.0;
const float BLOQUE_ALTO = 5.0;
const float BLOQUE_SEPARACION = 2.0;

const float BLOQUES_X = 3.0;
const float BLOQUES_Y = 7.0;

bool bloques[FILAS_BLOQUES][COLUMNAS_BLOQUES];

int bloquesRestantes = 0;

// ============================================================
// ANIMACION DE REBOTE
// ============================================================

float reboteAnimacion = 0.0;

// ============================================================
// INICIALIZAR BLOQUES
// ============================================================

void inicializarBloques() {

  bloquesRestantes = 0;

  for (int fila = 0; fila < FILAS_BLOQUES; fila++) {

    for (int columna = 0;
         columna < COLUMNAS_BLOQUES;
         columna++) {

      bloques[fila][columna] = true;

      bloquesRestantes++;
    }
  }
}

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

  for (int i = TRAIL_LENGTH - 1;
       i > 0;
       i--) {

    trail[i] = trail[i - 1];
  }

  trail[0].x = pelotaX;
  trail[0].y = pelotaY;
}

// ============================================================
// ACTUALIZAR PALETA
// ============================================================

void actualizarPaleta() {

  // Movimiento automatico solamente para probar.
  // Luego lo reemplazamos por los botones.

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
// REBOTE CONTRA LATERALES
// ============================================================

void comprobarParedesLaterales() {

  if (pelotaX - BALL_RADIUS <= 1) {

    pelotaX =
      1 + BALL_RADIUS;

    velocidadX =
      fabs(velocidadX);

    reboteAnimacion = 1.0;
  }

  if (pelotaX + BALL_RADIUS >= SCREEN_WIDTH - 1) {

    pelotaX =
      SCREEN_WIDTH - 1 - BALL_RADIUS;

    velocidadX =
      -fabs(velocidadX);

    reboteAnimacion = 1.0;
  }
}

// ============================================================
// REBOTE CONTRA ARRIBA
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
// REBOTE CONTRA PALETA
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

  if (tocaPaleta && velocidadY > 0) {

    pelotaY =
      arriba - BALL_RADIUS - 0.5;

    velocidadY =
      -fabs(velocidadY);

    // Modificar direccion horizontal
    // dependiendo de donde golpee.

    float diferencia =
      pelotaX - paletaX;

    velocidadX =
      diferencia * 0.08;

    // Evitar que la pelota vaya
    // casi completamente vertical.

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
// COMPROBAR BLOQUES
// ============================================================

void comprobarBloques() {

  for (int fila = 0;
       fila < FILAS_BLOQUES;
       fila++) {

    for (int columna = 0;
         columna < COLUMNAS_BLOQUES;
         columna++) {

      // Si el bloque ya fue destruido,
      // pasar al siguiente.

      if (!bloques[fila][columna]) {
        continue;
      }

      // Posicion del bloque.

      float bloqueX =
        BLOQUES_X +
        columna *
        (BLOQUE_ANCHO + BLOQUE_SEPARACION);

      float bloqueY =
        BLOQUES_Y +
        fila *
        (BLOQUE_ALTO + BLOQUE_SEPARACION);

      float bloqueDerecha =
        bloqueX + BLOQUE_ANCHO;

      float bloqueAbajo =
        bloqueY + BLOQUE_ALTO;

      // Comprobar colision.

      bool tocaHorizontal =
        pelotaX + BALL_RADIUS >= bloqueX &&
        pelotaX - BALL_RADIUS <= bloqueDerecha;

      bool tocaVertical =
        pelotaY + BALL_RADIUS >= bloqueY &&
        pelotaY - BALL_RADIUS <= bloqueAbajo;

      if (tocaHorizontal && tocaVertical) {

        // Destruir bloque.

        bloques[fila][columna] = false;

        bloquesRestantes--;

        // Determinar desde donde
        // llego la pelota.

        bool desdeArriba =
          pelotaAnteriorY + BALL_RADIUS <= bloqueY;

        bool desdeAbajo =
          pelotaAnteriorY - BALL_RADIUS >= bloqueAbajo;

        bool desdeIzquierda =
          pelotaAnteriorX + BALL_RADIUS <= bloqueX;

        bool desdeDerecha =
          pelotaAnteriorX - BALL_RADIUS >= bloqueDerecha;

        // Rebote vertical.

        if (desdeArriba) {

          pelotaY =
            bloqueY - BALL_RADIUS - 0.5;

          velocidadY =
            -fabs(velocidadY);
        }

        else if (desdeAbajo) {

          pelotaY =
            bloqueAbajo + BALL_RADIUS + 0.5;

          velocidadY =
            fabs(velocidadY);
        }

        // Rebote horizontal.

        else if (desdeIzquierda) {

          pelotaX =
            bloqueX - BALL_RADIUS - 0.5;

          velocidadX =
            -fabs(velocidadX);
        }

        else if (desdeDerecha) {

          pelotaX =
            bloqueDerecha + BALL_RADIUS + 0.5;

          velocidadX =
            fabs(velocidadX);
        }

        // Caso especial:
        // si entra por una esquina.

        else {

          float distanciaVertical =
            min(
              fabs(pelotaY - bloqueY),
              fabs(pelotaY - bloqueAbajo)
            );

          float distanciaHorizontal =
            min(
              fabs(pelotaX - bloqueX),
              fabs(pelotaX - bloqueDerecha)
            );

          if (distanciaVertical <= distanciaHorizontal) {

            velocidadY =
              -velocidadY;

          } else {

            velocidadX =
              -velocidadX;
          }
        }

        // Aumentar ligeramente la velocidad
        // cada vez que rompe un bloque.

        velocidadX *= 1.015;
        velocidadY *= 1.015;

        // Limite de velocidad horizontal.

        if (fabs(velocidadX) > 2.8) {

          if (velocidadX > 0) {

            velocidadX = 2.8;

          } else {

            velocidadX = -2.8;
          }
        }

        // Limite de velocidad vertical.

        if (fabs(velocidadY) > 2.8) {

          if (velocidadY > 0) {

            velocidadY = 2.8;

          } else {

            velocidadY = -2.8;
          }
        }

        reboteAnimacion = 1.0;

        // Importante:
        // salir despues de destruir un bloque.
        // Evita destruir varios en el mismo frame.

        return;
      }
    }
  }
}

// ============================================================
// REINICIAR PELOTA
// ============================================================

void reiniciarPelota() {

  pelotaX =
    paletaX;

  pelotaY =
    PADDLE_Y - 7;

  pelotaAnteriorX =
    pelotaX;

  pelotaAnteriorY =
    pelotaY;

  velocidadX =
    0.85;

  velocidadY =
    -1.25;

  inicializarTrail();
}

// ============================================================
// REINICIAR BREAKOUT
// ============================================================

void reiniciarBreakout() {

  inicializarBloques();

  pelotaX =
    paletaX;

  pelotaY =
    PADDLE_Y - 7;

  pelotaAnteriorX =
    pelotaX;

  pelotaAnteriorY =
    pelotaY;

  velocidadX =
    0.85;

  velocidadY =
    -1.25;

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

  // Paredes.

  comprobarParedesLaterales();

  comprobarParedSuperior();

  // Bloques.

  comprobarBloques();

  // Paleta.

  comprobarPaleta();

  // Animacion de rebote.

  if (reboteAnimacion > 0) {

    reboteAnimacion -= 0.08;

    if (reboteAnimacion < 0) {

      reboteAnimacion = 0;
    }
  }

  // Si la pelota se fue por abajo,
  // reiniciar solamente la pelota.

  if (pelotaY >
      SCREEN_HEIGHT + 5) {

    reiniciarPelota();
  }

  // Si se destruyeron todos los bloques,
  // comenzar otra partida.

  if (bloquesRestantes <= 0) {

    reiniciarBreakout();
  }
}

// ============================================================
// DIBUJAR BLOQUES
// ============================================================

void dibujarBloques() {

  for (int fila = 0;
       fila < FILAS_BLOQUES;
       fila++) {

    for (int columna = 0;
         columna < COLUMNAS_BLOQUES;
         columna++) {

      if (!bloques[fila][columna]) {
        continue;
      }

      // Convertimos las medidas float
      // a enteros para Adafruit_GFX.

      int x =
        (int)(
          BLOQUES_X +
          columna *
          (BLOQUE_ANCHO + BLOQUE_SEPARACION)
        );

      int y =
        (int)(
          BLOQUES_Y +
          fila *
          (BLOQUE_ALTO + BLOQUE_SEPARACION)
        );

      int ancho =
        (int)BLOQUE_ANCHO;

      int alto =
        (int)BLOQUE_ALTO;

      // fillRect necesita:
      // x, y, ancho, alto, color

      pantalla.fillRect(
        x,
        y,
        ancho,
        alto,
        SSD1306_WHITE
      );
    }
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

    // Convertir float a int.

    int x =
      (int)trail[i].x;

    int y =
      (int)trail[i].y;

    // fillCircle necesita:
    // x, y, radio, color

    pantalla.fillCircle(
      x,
      y,
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
    (int)(
      paletaX -
      PADDLE_WIDTH / 2
    );

  int ancho =
    (int)PADDLE_WIDTH;

  int alto =
    (int)PADDLE_HEIGHT;

  int y =
    (int)PADDLE_Y;

  pantalla.fillRoundRect(
    izquierda,
    y,
    ancho,
    alto,
    1,
    SSD1306_WHITE
  );
}

// ============================================================
// DIBUJAR PELOTA
// ============================================================

void dibujarPelota() {

  int x =
    (int)pelotaX;

  int y =
    (int)pelotaY;

  int radio =
    (int)BALL_RADIUS;

  if (reboteAnimacion > 0.5) {

    radio = 3;
  }

  pantalla.fillCircle(
    x,
    y,
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

  // Dibujar bloques.

  dibujarBloques();

  // Dibujar rastro.

  dibujarTrail();

  // Dibujar paleta.

  dibujarPaleta();

  // Dibujar pelota.

  dibujarPelota();

  // Mostrar pantalla.

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

  inicializarBloques();

  inicializarTrail();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long ahora =
    millis();

  if (ahora - ultimoFrame >= FRAME_TIME) {

    ultimoFrame =
      ahora;

    actualizarPaleta();

    actualizarPelota();

    dibujarJuego();
  }
}