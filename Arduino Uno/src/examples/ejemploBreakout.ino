#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - ejemplo de animacion: BREAKOUT
// Version Arduino Uno (ATmega328P).
//
// Muestra los ladrillos, la colision desde las cuatro direcciones y
// la velocidad que va ganando la pelota cada vez que rompe uno.
// La paleta se mueve sola con un seno para poder verlo sin botones.
//
// OLED 128x64 I2C: SDA -> A4, SCL -> A5 (pines I2C del Uno)
//
// La animacion NO es bloqueante: todo con millis(), sin delay().
//
//
// ADAPTACION AL UNO: el ATmega328P solo tiene 2 KB de SRAM, asi que la
// pantalla usa el buffer de UNA PAGINA de U8g2 (el sufijo 1_ en vez de
// F_): 128 bytes en lugar de 1024. El dibujo es exactamente el mismo;
// U8g2 se encarga de volcar la pantalla por paginas. Con el buffer
// completo este ejemplo no entraria en la memoria del Uno.// Compilar con: placa arduino:avr:uno
// ============================================================

U8G2_SH1106_128X64_NONAME_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306 en lugar de SH1106:
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);

const unsigned long FRAME_TIME = 16;

const int COLUMNAS = 8, FILAS = 4;
const float BLOQUE_ANCHO = 13.0, BLOQUE_ALTO = 5.0, BLOQUE_SEP = 2.0;
const float BLOQUES_X = 3.0, BLOQUES_Y = 7.0;
const float RADIO = 2.5;
const float ANCHO_PALETA = 26.0, ALTO_PALETA = 3.0, Y_PALETA = 58.0;
const int LARGO_TRAIL = 7;
const float VELOCIDAD_MAXIMA = 2.8;   // tope para que no se vuelva imposible

struct Punto { float x; float y; };

bool ladrillos[FILAS][COLUMNAS];
int bloquesRestantes = 0;
float paletaX = 64.0, paletaObjetivoX = 64.0;
float pelotaX = 64.0, pelotaY = 48.0;
float pelotaAnteriorX = 64.0, pelotaAnteriorY = 48.0;
float velocidadX = 0.85, velocidadY = -1.25;
float rebote = 0.0;
Punto trail[LARGO_TRAIL];

unsigned long ultimoFrame = 0;

// ============================================================
// ANIMACION
// ============================================================

void inicializarBloques() {
  bloquesRestantes = 0;
  for (int f = 0; f < FILAS; f++)
    for (int c = 0; c < COLUMNAS; c++) { ladrillos[f][c] = true; bloquesRestantes++; }
}
void inicializarTrail() {
  for (int i = 0; i < LARGO_TRAIL; i++) { trail[i].x = pelotaX; trail[i].y = pelotaY; }
}
void actualizarTrail() {
  for (int i = LARGO_TRAIL - 1; i > 0; i--) trail[i] = trail[i - 1];
  trail[0].x = pelotaX;
  trail[0].y = pelotaY;
}
void actualizarPaleta() {
  float t = millis() / 1000.0;
  paletaObjetivoX = 64.0 + sin(t * 1.15) * 43.0;
  paletaX += (paletaObjetivoX - paletaX) * 0.15;
  paletaX = constrain(paletaX, ANCHO_PALETA / 2, 128 - ANCHO_PALETA / 2);
}
void comprobarPaleta() {
  float izquierda = paletaX - ANCHO_PALETA / 2;
  float derecha = paletaX + ANCHO_PALETA / 2;
  float arriba = Y_PALETA;
  float abajo = Y_PALETA + ALTO_PALETA;
  bool tocaPaleta = pelotaX + RADIO >= izquierda &&
                    pelotaX - RADIO <= derecha &&
                    pelotaY + RADIO >= arriba &&
                    pelotaY - RADIO <= abajo;
  if (tocaPaleta && velocidadY > 0) {
    pelotaY = arriba - RADIO - 0.5;
    velocidadY = -fabs(velocidadY);
    velocidadX = (pelotaX - paletaX) * 0.07;
    if (fabs(velocidadX) < 0.35) velocidadX = (velocidadX < 0) ? -0.35 : 0.35;
    rebote = 1.0;
  }
}
void comprobarBloques() {
  for (int f = 0; f < FILAS; f++) {
    for (int c = 0; c < COLUMNAS; c++) {
      if (!ladrillos[f][c]) continue;

      float bx = BLOQUES_X + c * (BLOQUE_ANCHO + BLOQUE_SEP);
      float by = BLOQUES_Y + f * (BLOQUE_ALTO + BLOQUE_SEP);
      float derecha = bx + BLOQUE_ANCHO;
      float abajo = by + BLOQUE_ALTO;

      bool tocaX = pelotaX + RADIO >= bx && pelotaX - RADIO <= derecha;
      bool tocaY = pelotaY + RADIO >= by && pelotaY - RADIO <= abajo;
      if (!tocaX || !tocaY) continue;

      ladrillos[f][c] = false;
      bloquesRestantes--;

      // La direccion de entrada se deduce de DONDE ESTABA la pelota en
      // el fotograma anterior. Asi el rebote sale por la cara correcta
      // y no se "pega" al ladillo.
      bool desdeArriba = pelotaAnteriorY + RADIO <= by;
      bool desdeAbajo = pelotaAnteriorY - RADIO >= abajo;
      bool desdeIzquierda = pelotaAnteriorX + RADIO <= bx;
      bool desdeDerecha = pelotaAnteriorX - RADIO >= derecha;

      if (desdeArriba) {
        pelotaY = by - RADIO - 0.5;
        velocidadY = -fabs(velocidadY);
      } else if (desdeAbajo) {
        pelotaY = abajo + RADIO + 0.5;
        velocidadY = fabs(velocidadY);
      } else if (desdeIzquierda) {
        pelotaX = bx - RADIO - 0.5;
        velocidadX = -fabs(velocidadX);
      } else if (desdeDerecha) {
        pelotaX = derecha + RADIO + 0.5;
        velocidadX = fabs(velocidadX);
      } else {
        // Entrada por una esquina: rebota por la cara mas cercana.
        float dVertical = min(fabs(pelotaY - by), fabs(pelotaY - abajo));
        float dHorizontal = min(fabs(pelotaX - bx), fabs(pelotaX - derecha));
        if (dVertical <= dHorizontal) velocidadY = -velocidadY;
        else velocidadX = -velocidadX;
      }

      // Cada ladrillo roto deja la pelota un poco mas rapida, pero
      // nunca por encima del tope.
      velocidadX *= 1.015;
      velocidadY *= 1.015;
      if (fabs(velocidadX) > VELOCIDAD_MAXIMA) velocidadX = (velocidadX > 0) ? VELOCIDAD_MAXIMA : -VELOCIDAD_MAXIMA;
      if (fabs(velocidadY) > VELOCIDAD_MAXIMA) velocidadY = (velocidadY > 0) ? VELOCIDAD_MAXIMA : -VELOCIDAD_MAXIMA;

      rebote = 1.0;
      return;   // sale tras el primer golpe: evita romper varios de un golpe
    }
  }
}
void actualizarPelota() {
  pelotaAnteriorX = pelotaX;
  pelotaAnteriorY = pelotaY;
  actualizarTrail();
  pelotaX += velocidadX;
  pelotaY += velocidadY;

  if (pelotaX <= 2) { pelotaX = 2; velocidadX = fabs(velocidadX); rebote = 1.0; }
  if (pelotaX >= 125) { pelotaX = 125; velocidadX = -fabs(velocidadX); rebote = 1.0; }
  if (pelotaY <= 14) { pelotaY = 14; velocidadY = fabs(velocidadY); rebote = 1.0; }

  comprobarBloques();
  comprobarPaleta();

  if (rebote > 0) {
    rebote -= 0.08;
    if (rebote < 0) rebote = 0;
  }

  if (pelotaY > 69) {
    pelotaX = paletaX;
    pelotaY = Y_PALETA - 7;
    velocidadX = (random(0, 2) ? 0.85 : -0.85);
    velocidadY = -1.25;
    inicializarTrail();
  }
  if (bloquesRestantes <= 0) inicializarBloques();
}

// ============================================================
// DIBUJO
// ============================================================

void dibujar() {
  pantalla.clearBuffer();

  pantalla.setFont(u8g2_font_6x10_tf);
  pantalla.drawStr(4, 9, "BREAKOUT");
  pantalla.setCursor(110, 1);
  pantalla.print(bloquesRestantes);

  pantalla.drawVLine(0, 0, 64);
  pantalla.drawVLine(127, 0, 64);
  pantalla.drawHLine(0, 0, 128);

  for (int f = 0; f < FILAS; f++) {
    for (int c = 0; c < COLUMNAS; c++) {
      if (!ladrillos[f][c]) continue;
      int x = (int)(BLOQUES_X + c * (BLOQUE_ANCHO + BLOQUE_SEP));
      int y = (int)(BLOQUES_Y + f * (BLOQUE_ALTO + BLOQUE_SEP));
      pantalla.drawBox(x, y, (int)BLOQUE_ANCHO, (int)BLOQUE_ALTO);
    }
  }

  for (int i = LARGO_TRAIL - 1; i >= 0; i--) {
    int radio = 1;
    if (i < 2) radio = 2;
    pantalla.drawDisc((int)trail[i].x, (int)trail[i].y, radio);
  }

  pantalla.drawRBox((int)(paletaX - ANCHO_PALETA / 2), (int)Y_PALETA,
                    (int)ANCHO_PALETA, (int)ALTO_PALETA, 1);
  int radioPelota = 2;
  if (rebote > 0.5) radioPelota = 3;
  pantalla.drawDisc((int)pelotaX, (int)pelotaY, radioPelota);

  pantalla.sendBuffer();
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  pantalla.begin();
  pantalla.setContrast(180);
  randomSeed(analogRead(A0) + micros());
  inicializarBloques();
  inicializarTrail();
}

void loop() {
  unsigned long ahora = millis();
  if (ahora - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = ahora;
  actualizarPaleta();
  actualizarPelota();
  dibujar();
}