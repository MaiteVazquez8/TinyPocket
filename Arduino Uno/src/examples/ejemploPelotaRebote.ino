#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - ejemplo de animacion: PELOTA REBOTANDO
// Version Arduino Uno (ATmega328P).
//
// Sirve para ver, por separado y sin botones, como se mueve una
// pelota con estela y como crece al rebotar contra las paredes.
//
// OLED 128x64 I2C: SDA -> A4, SCL -> A5 (pines I2C del Uno)
// Alimentacion: switch + bateria al pin VIN
//
// La animacion NO es bloqueante: no hay ni un solo delay() en el
// bucle. Cada paso se comprueba con millis(), asi que este ejemplo
// se puede copiar tal cual dentro de otro programa.
//
//
// ADAPTACION AL UNO: el ATmega328P solo tiene 2 KB de SRAM, asi que la
// pantalla usa el buffer de UNA PAGINA de U8g2 (el sufijo 1_ en vez de
// F_): 128 bytes en lugar de 1024. El dibujo es exactamente el mismo;
// U8g2 se encarga de volcar la pantalla por paginas. Con el buffer
// completo este ejemplo no entraria en la memoria del Uno.// Compilar con: placa arduino:avr:uno
// ============================================================

U8G2_SH1106_128X64_NONAME_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306 en lugar de SH1106, cambia la linea anterior por:
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);

// ============================================================
// PARAMETROS DE LA ANIMACION
// Todos en un solo sitio para poder experimentar con ellos.
// ============================================================

const unsigned long FRAME_TIME = 16;   // ms entre fotogramas (~60 fps)
const int LARGO_TRAIL = 8;             // puntos de la estela
const float VELOCIDAD_X = 0.9;
const float VELOCIDAD_Y = 0.7;
const float MARGEN = 3.0;              // separacion con el borde

struct Punto { float x; float y; };

float pelotaX = 64.0, pelotaY = 32.0;
float velocidadX = VELOCIDAD_X, velocidadY = VELOCIDAD_Y;
float rebote = 0.0;                    // 1.0 al tocar, baja hasta 0
Punto trail[LARGO_TRAIL];

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
void tocarBorde() {
  rebote = 1.0;
}
void actualizarPelota() {
  actualizarTrail();
  pelotaX += velocidadX;
  pelotaY += velocidadY;

  // Rebote en las cuatro paredes. Al tocar, la velocidad se invierte
  // con fabs() para que nunca se pegue al borde.
  if (pelotaX - MARGEN <= 0) { pelotaX = MARGEN; velocidadX = fabs(velocidadX); tocarBorde(); }
  if (pelotaX + MARGEN >= 128) { pelotaX = 128 - MARGEN; velocidadX = -fabs(velocidadX); tocarBorde(); }
  if (pelotaY - MARGEN <= 0) { pelotaY = MARGEN; velocidadY = fabs(velocidadY); tocarBorde(); }
  if (pelotaY + MARGEN >= 64) { pelotaY = 64 - MARGEN; velocidadY = -fabs(velocidadY); tocarBorde(); }

  // La pelota "late" un poco al rebotar y se calma sola.
  if (rebote > 0) {
    rebote -= 0.08;
    if (rebote < 0) rebote = 0;
  }
}

// ============================================================
// DIBUJO
// ============================================================

void dibujar() {
  pantalla.clearBuffer();

  // Marco
  pantalla.drawFrame(0, 0, 128, 64);

  // Estela: cuanto mas reciente el punto, mas grande.
  for (int i = LARGO_TRAIL - 1; i >= 0; i--) {
    int radio = 1;
    if (i < 3) radio = 2;
    pantalla.drawDisc((int)trail[i].x, (int)trail[i].y, radio);
  }

  // Pelota: crece un pixel mientras dura el rebote.
  int radioPelota = 2;
  if (rebote > 0.4) radioPelota = 3;
  pantalla.drawDisc((int)pelotaX, (int)pelotaY, radioPelota);

  pantalla.setFont(u8g2_font_6x10_tf);
  pantalla.drawStr(2, 9, "Pelota rebotando");
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
  if (ahora - ultimoFrame < FRAME_TIME) return;   // no bloquea
  ultimoFrame = ahora;
  actualizarPelota();
  dibujar();
}