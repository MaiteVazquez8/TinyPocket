#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - ejemplo de animacion: PING PONG
// Version Arduino Uno (ATmega328P).
//
// Muestra el juego de la version final sin botones: la paleta se
// mueve sola con un seno, para poder ver el rebote, la estela y el
// crecimiento de la pelota en cada impacto.
//
// OLED 128x64 I2C: SDA -> A4, SCL -> A5 (pines I2C del Uno)
//
// El bucle NO es bloqueante: no hay delay(), todo se sincroniza con
// millis(). En la version final de tinyPocket.ino esta misma funcion
// actualizarPelota() se llama igual, pero la paleta la mueven los
// botones en lugar del seno.
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
const int LARGO_TRAIL = 8;
const float ANCHO_PALETA = 28.0;
const float ALTO_PALETA = 3.0;
const float Y_PALETA = 58.0;
const float RADIO_PELOTA = 2.5;

struct Punto { float x; float y; };

float paletaX = 64.0, paletaObjetivoX = 64.0;
float pelotaX = 64.0, pelotaY = 45.0;
float velocidadX = 0.85, velocidadY = -1.25;
float rebote = 0.0;
Punto trail[LARGO_TRAIL];
byte impactos = 0;

unsigned long ultimoFrame = 0;

// ============================================================
// ANIMACION
// ============================================================

void inicializarTrail() {
  for (int i = 0; i < LARGO_TRAIL; i++) { trail[i].x = pelotaX; trail[i].y = pelotaY; }
}
void actualizarTrail() {
  for (int i = LARGO_TRAIL - 1; i > 0; i--) trail[i] = trail[i - 1];
  trail[0].x = pelotaX;
  trail[0].y = pelotaY;
}
// Movimiento automatico de la paleta: recorre la pantalla con un seno.
// En la version final esto lo sustituyen los botones.
void actualizarPaleta() {
  float t = millis() / 1000.0;
  paletaObjetivoX = 64.0 + sin(t * 1.35) * 42.0;
  paletaX += (paletaObjetivoX - paletaX) * 0.15;
}
void comprobarPaleta() {
  float izquierda = paletaX - ANCHO_PALETA / 2;
  float derecha = paletaX + ANCHO_PALETA / 2;
  float arriba = Y_PALETA;
  float abajo = Y_PALETA + ALTO_PALETA;

  bool tocaPaleta = pelotaX + RADIO_PELOTA >= izquierda &&
                    pelotaX - RADIO_PELOTA <= derecha &&
                    pelotaY + RADIO_PELOTA >= arriba &&
                    pelotaY - RADIO_PELOTA <= abajo;

  // Solo rebota cuando la pelota viene hacia abajo.
  if (tocaPaleta && velocidadY > 0) {
    pelotaY = arriba - RADIO_PELOTA - 0.5;
    velocidadY = -fabs(velocidadY);
    // La direccion horizontal depende de donde golpee: en el borde
    // sale mas paralela, en el centro sube casi vertical.
    velocidadX = (pelotaX - paletaX) * 0.08;
    if (fabs(velocidadX) < 0.35) {
      velocidadX = (velocidadX >= 0) ? 0.35 : -0.35;
    }
    rebote = 1.0;
    impactos++;
  }
}
void actualizarPelota() {
  actualizarTrail();
  pelotaX += velocidadX;
  pelotaY += velocidadY;

  if (pelotaX - RADIO_PELOTA <= 1) { pelotaX = 1 + RADIO_PELOTA; velocidadX = fabs(velocidadX); rebote = 1.0; }
  if (pelotaX + RADIO_PELOTA >= 126) { pelotaX = 126 - RADIO_PELOTA; velocidadX = -fabs(velocidadX); rebote = 1.0; }
  if (pelotaY - RADIO_PELOTA <= 1) { pelotaY = 1 + RADIO_PELOTA; velocidadY = fabs(velocidadY); rebote = 1.0; }

  comprobarPaleta();

  if (rebote > 0) {
    rebote -= 0.08;
    if (rebote < 0) rebote = 0;
  }

  // Si se le escapa por abajo, vuelve a salir desde la paleta.
  if (pelotaY > 69) {
    pelotaX = paletaX;
    pelotaY = Y_PALETA - 7;
    velocidadX = (random(0, 2) ? 0.85 : -0.85);
    velocidadY = -1.25;
    inicializarTrail();
  }
}

// ============================================================
// DIBUJO
// ============================================================

void dibujar() {
  pantalla.clearBuffer();

  pantalla.setFont(u8g2_font_6x10_tf);
  pantalla.drawStr(4, 9, "PING PONG");
  pantalla.setCursor(96, 1);
  pantalla.print(impactos);

  pantalla.drawVLine(0, 0, 64);
  pantalla.drawVLine(127, 0, 64);
  pantalla.drawHLine(0, 0, 128);

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