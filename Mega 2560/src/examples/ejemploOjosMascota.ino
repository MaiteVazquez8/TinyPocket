#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - ejemplo de animacion: OJOS DE LA MASCOTA
// Version Arduino Mega 2560.
//
// Toda la expresion de la mascota convertida en fases con duracion,
// de forma que el bucle NO tiene ni un solo delay(). Cada fase es un
// trozo de la expresion y se encadena sola con millis().
//
// Este archivo es la referencia de las fases: el mismo listado, con
// los mismos tiempos, es el que usa tinyPocket.ino en su seccion
// MASCOTA.
//
// OLED 128x64 I2C: SDA -> D20, SCL -> D21 (pines I2C del Mega)
//
// Compilar con: placa arduino:avr:mega
// ============================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306 en lugar de SH1106:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);

// ============================================================
// GEOMETRIA DE LOS OJOS
// ============================================================

const float CENTRO_IZQ = 43.0;
const float CENTRO_DER = 85.0;
const float CENTRO_Y = 32.0;
const float OJO_ANCHO = 25.0;
const float OJO_ALTO = 31.0;

// ============================================================
// FASES
// Los valores son la duracion en milisegundos de cada fase. Los
// nombres describen la expresion: mirar, volver al centro, parpadear,
// feliz, sorpresa, guino, dormir y despertar.
// ============================================================

enum Fase {
  NORMAL, MIRA_IZQ, PAUSA_IZQ, VUELVE_CENTRO_1, PAUSA_CENTRO_1,
  MIRA_DER, PAUSA_DER, VUELVE_CENTRO_2, PAUSA_CENTRO_2,
  PARPADEO_CIERRA, PARPADEO_ESPERA, PARPADEO_ABRE, PARPADEO_PAUSA,
  FELIZ, FELIZ_ESPERA, FELIZ_NORMAL, FELIZ_PAUSA,
  SORPRESA, SORPRESA_REBOTE, SORPRESA_ESPERA, SORPRESA_NORMAL, SORPRESA_PAUSA,
  GUINO_CIERRA, GUINO_ESPERA, GUINO_ABRE, GUINO_PAUSA,
  MIRA_POCO_IZQ, MIRA_POCO_PAUSA, MIRA_POCO_CENTRO, MIRA_POCO_PAUSA_2,
  DORMIR_CIERRA, DORMIR, DESPERTAR, DESPERTAR_PAUSA,
  DESPERTAR_PARPADEO_CIERRA, DESPERTAR_PARPADEO_ESPERA, DESPERTAR_PARPADEO_ABRE
};

const unsigned long duracionFase[] PROGMEM = {
    800, 600, 500, 550, 300, 650, 500, 550, 500,
    240,  90, 240, 600,
    240, 1000, 500, 400,
     300, 210, 700, 450, 400,
     200, 150, 200, 500,
     400, 300, 400, 500,
     360, 1200, 360, 200, 240, 90, 240
};

Fase fase = NORMAL;
unsigned long inicioFase = 0;

// ============================================================
// AYUDAS
// ============================================================

// Interpolacion suave (smoothstep): el movimiento arranca y acaba
// despacio en vez de a tirones, que es lo que hace que los ojos
// parezcan "vivos" y no un servo.
float suave(float t) {
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  return t * t * (3.0 - 2.0 * t);
}
unsigned long duracionActual() {
  return pgm_read_dword(&duracionFase[fase]);
}
void siguienteFase() {
  fase = (Fase)((fase + 1) % (DESPERTAR_PARPADEO_ABRE + 1));
  inicioFase = millis();
}

// ============================================================
// DIBUJO DE LOS OJOS
// ============================================================

void dibujarOjo(float x, float y, float ancho, float alto) {
  pantalla.drawRBox((int)(x - ancho / 2), (int)(y - alto / 2), (int)ancho, (int)alto, 8);
}
void dibujarOjoFeliz(float x, float y) {
  pantalla.drawLine((int)x - 11, (int)y + 2, (int)x - 6, (int)y - 3);
  pantalla.drawLine((int)x - 6, (int)y - 3, (int)x, (int)y - 5);
  pantalla.drawLine((int)x, (int)y - 5, (int)x + 6, (int)y - 3);
  pantalla.drawLine((int)x + 6, (int)y - 3, (int)x + 11, (int)y + 2);
}
void dibujarDosOjos(float desplazamiento, float ancho, float alto, float y) {
  pantalla.clearBuffer();
  dibujarOjo(CENTRO_IZQ + desplazamiento, y, ancho, alto);
  dibujarOjo(CENTRO_DER + desplazamiento, y, ancho, alto);
}
void ojosCerrados() {
  pantalla.drawLine((int)CENTRO_IZQ - 10, (int)CENTRO_Y, (int)CENTRO_IZQ + 10, (int)CENTRO_Y);
  pantalla.drawLine((int)CENTRO_DER - 10, (int)CENTRO_Y, (int)CENTRO_DER + 10, (int)CENTRO_Y);
}
void dibujarGuino(float altoIzquierdo) {
  pantalla.clearBuffer();
  dibujarOjo(CENTRO_IZQ, CENTRO_Y, OJO_ANCHO, altoIzquierdo);
  dibujarOjo(CENTRO_DER, CENTRO_Y, OJO_ANCHO, OJO_ALTO);
}

void actualizarYRedibujar() {
  unsigned long transcurrido = millis() - inicioFase;
  unsigned long dur = duracionActual();
  float p = (float)transcurrido / (float)dur;
  if (p > 1) p = 1;
  float s = suave(p);

  float desplazamiento = 0, ancho = OJO_ANCHO, alto = OJO_ALTO, y = CENTRO_Y;

  // --- Miradas ---
  if (fase == MIRA_IZQ) desplazamiento = -15 * s;
  else if (fase == VUELVE_CENTRO_1) desplazamiento = -15 + 15 * s;
  else if (fase == MIRA_DER) desplazamiento = 15 * s;
  else if (fase == VUELVE_CENTRO_2) desplazamiento = 15 - 15 * s;
  else if (fase == MIRA_POCO_IZQ) desplazamiento = -8 * s;
  else if (fase == MIRA_POCO_CENTRO) desplazamiento = -8 + 8 * s;

  // --- Parpadeo: los ojos se aplastan hasta desaparecer ---
  if (fase == PARPADEO_CIERRA || fase == DESPERTAR_PARPADEO_CIERRA) {
    alto = OJO_ALTO * (1.0 - s);
    if (alto < 2) alto = 2;
  } else if (fase == PARPADEO_ABRE || fase == DESPERTAR_PARPADEO_ABRE) {
    alto = 2 + (OJO_ALTO - 2) * s;
  } else if (fase == PARPADEO_ESPERA || fase == DESPERTAR_PARPADEO_ESPERA) {
    pantalla.clearBuffer();
    ojosCerrados();
    pantalla.sendBuffer();
    return;
  }

  // --- Feliz: se achican, bajan y quedan de "^" ---
  if (fase == FELIZ) {
    alto = OJO_ALTO - (OJO_ALTO - 6) * s;
    y = CENTRO_Y + 3 * s;
  } else if (fase == FELIZ_ESPERA) {
    pantalla.clearBuffer();
    dibujarOjoFeliz(CENTRO_IZQ, CENTRO_Y);
    dibujarOjoFeliz(CENTRO_DER, CENTRO_Y);
    pantalla.sendBuffer();
    return;
  } else if (fase == FELIZ_NORMAL) {
    alto = 6 + (OJO_ALTO - 6) * s;
    y = CENTRO_Y + 3 - 3 * s;
  }

  // --- Sorpresa: crecen y dan un bote ---
  if (fase == SORPRESA) {
    ancho = OJO_ANCHO + 7 * s;
    alto = OJO_ALTO + 8 * s;
  } else if (fase == SORPRESA_REBOTE) {
    ancho = OJO_ANCHO + 7;
    alto = OJO_ALTO + 8;
    y = CENTRO_Y + sin(p * PI * 3.0) * (1.0 - p) * 2.0;
  } else if (fase == SORPRESA_NORMAL) {
    ancho = OJO_ANCHO + 7 - 7 * s;
    alto = OJO_ALTO + 8 - 8 * s;
  }

  // --- Guino: solo se cierra el ojo izquierdo ---
  if (fase == GUINO_CIERRA) {
    float h = OJO_ALTO - (OJO_ALTO - 3) * s;
    if (h < 3) h = 3;
    dibujarGuino(h);
    pantalla.sendBuffer();
    return;
  } else if (fase == GUINO_ESPERA) {
    dibujarGuino(3);
    pantalla.sendBuffer();
    return;
  } else if (fase == GUINO_ABRE) {
    dibujarGuino(3 + (OJO_ALTO - 3) * s);
    pantalla.sendBuffer();
    return;
  }

  // --- Dormir y despertar ---
  if (fase == DORMIR_CIERRA) {
    alto = OJO_ALTO * (1.0 - s);
    if (alto < 2) alto = 2;
  } else if (fase == DORMIR) {
    pantalla.clearBuffer();
    ojosCerrados();
    pantalla.sendBuffer();
    return;
  } else if (fase == DESPERTAR) {
    alto = 2 + (OJO_ALTO - 2) * s;
  }

  dibujarDosOjos(desplazamiento, ancho, alto, y);
  pantalla.sendBuffer();
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  pantalla.begin();
  pantalla.setContrast(180);
  fase = NORMAL;
  inicioFase = millis();
}

void loop() {
  if (millis() - inicioFase >= duracionActual()) siguienteFase();
  actualizarYRedibujar();
}