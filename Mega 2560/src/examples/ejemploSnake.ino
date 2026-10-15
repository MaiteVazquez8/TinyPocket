#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - ejemplo de animacion: SNAKE CON IA
// Version Arduino Mega 2560.
//
// IMPORTANTE: este ejemplo NO es el juego jugable. Aqui la serpiente
// se mueve sola con una IA que persigue la comida, porque el objetivo
// es ver el movimiento suave, los ojos de la cabeza y la comida que
// late. El Snake CONTROLADO POR EL JUGADOR (con botones, sin IA y
// sin giros de 180 grados) esta en tinyPocket.ino.
//
// OLED 128x64 I2C: SDA -> D20, SCL -> D21 (pines I2C del Mega)
//
// La animacion NO es bloqueante: todo con millis(), sin delay().
//
// Compilar con: placa arduino:avr:mega
// ============================================================

U8G2_SH1106_128X64_NONAME_F_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306 en lugar de SH1106:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);

const unsigned long FRAME_TIME = 20;   // 50 fps: el ojo humano no necesita mas
const int MAX_SEGMENTOS = 40;
const float VELOCIDAD = 1.55;
const float DISTANCIA_COMIDA = 8.0;

float snakeX[MAX_SEGMENTOS], snakeY[MAX_SEGMENTOS];
int longitud = 8;
int dirX = 0, dirY = -1;               // arranca hacia arriba
float comidaX = 32, comidaY = 20;
unsigned long ultimoFrame = 0;

// ============================================================
// IA
// ============================================================

float distancia(float x1, float y1, float x2, float y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}
bool posicionPeligrosa(float x, float y) {
  if (x <= 4 || x >= 124 || y <= 4 || y >= 60) return true;
  // No se miran los 2 primeros segmentos porque van pegados a la cabeza.
  for (int i = 4; i < longitud - 2; i++)
    if (distancia(x, y, snakeX[i], snakeY[i]) < 5.5) return true;
  return false;
}
bool direccionSegura(int ndx, int ndy) {
  // Mira 5, 10 y 15 pixeles por delante: si hay pared o cuerpo, no.
  for (int d = 5; d <= 15; d += 5) {
    if (posicionPeligrosa(snakeX[0] + ndx * d, snakeY[0] + ndy * d)) return false;
  }
  return true;
}
int evaluarDireccion(int ndx, int ndy) {
  float futuroX = snakeX[0] + ndx * 25;
  float futuroY = snakeY[0] + ndy * 25;
  float distanciaActual = distancia(snakeX[0], snakeY[0], comidaX, comidaY);
  float distanciaFutura = distancia(futuroX, futuroY, comidaX, comidaY);

  int puntaje = 0;
  // Cuanto mas cerca quede la comida, mejor.
  puntaje += (int)(distanciaFutura * 2.5);
  if (distanciaFutura < distanciaActual) puntaje -= 45;
  else puntaje += 25;

  // Seguridad: es una mala opcion pero no imposible.
  if (!direccionSegura(ndx, ndy)) puntaje += 1000;
  // Media vuelta prohibida.
  if (ndx == -dirX && ndy == -dirY) puntaje += 500;
  // Un poco de azar para que no se quede en un ciclo cerrado.
  puntaje += random(0, 6);
  return puntaje;
}
void pensarMovimiento() {
  int posiblesX[4] = {1, -1, 0, 0};
  int posiblesY[4] = {0, 0, 1, -1};
  int mejor = -1;
  int mejorPuntaje = 999999;
  for (int i = 0; i < 4; i++) {
    if (posiblesX[i] == -dirX && posiblesY[i] == -dirY) continue;
    int p = evaluarDireccion(posiblesX[i], posiblesY[i]);
    if (p < mejorPuntaje) { mejorPuntaje = p; mejor = i; }
  }
  if (mejor >= 0) { dirX = posiblesX[mejor]; dirY = posiblesY[mejor]; }
}

// ============================================================
// ANIMACION
// ============================================================

void generarComida() {
  for (int intento = 0; intento < 50; intento++) {
    float x = random(12, 116);
    float y = random(8, 56);
    x = ((int)x / 4) * 4;
    y = ((int)y / 4) * 4;
    bool libre = true;
    for (int i = 0; i < longitud; i++)
      if (distancia(x, y, snakeX[i], snakeY[i]) < DISTANCIA_COMIDA) { libre = false; break; }
    if (libre) { comidaX = x; comidaY = y; return; }
  }
}
void reiniciar() {
  longitud = 8;
  for (int i = 0; i < longitud; i++) { snakeX[i] = 64 - i * 4; snakeY[i] = 40; }
  dirX = 0; dirY = -1;
  generarComida();
}
void actualizarSerpiente() {
  for (int i = longitud - 1; i > 0; i--) { snakeX[i] = snakeX[i - 1]; snakeY[i] = snakeY[i - 1]; }
  snakeX[0] += dirX * VELOCIDAD;
  snakeY[0] += dirY * VELOCIDAD;

  if (snakeX[0] <= 1 || snakeX[0] >= 127 || snakeY[0] <= 1 || snakeY[0] >= 63) { reiniciar(); return; }
  for (int i = 4; i < longitud; i++)
    if (distancia(snakeX[0], snakeY[0], snakeX[i], snakeY[i]) < 4.0) { reiniciar(); return; }

  if (distancia(snakeX[0], snakeY[0], comidaX, comidaY) < DISTANCIA_COMIDA) {
    if (longitud < MAX_SEGMENTOS) {
      snakeX[longitud] = snakeX[longitud - 1];
      snakeY[longitud] = snakeY[longitud - 1];
      longitud++;
    }
    generarComida();
  }
}

// ============================================================
// DIBUJO
// ============================================================

void dibujarSerpiente() {
  // De la cola a la cabeza, para que la cabeza quede encima.
  for (int i = longitud - 1; i >= 0; i--) {
    int x = (int)snakeX[i];
    int y = (int)snakeY[i];
    if (i == 0) {
      pantalla.drawDisc(x, y, 3);
      // Los ojos miran hacia donde avanza.
      int ex1 = x, ey1 = y, ex2 = x, ey2 = y;
      if (dirX > 0) { ex1++; ey1--; ex2++; ey2++; }
      else if (dirX < 0) { ex1--; ey1--; ex2--; ey2++; }
      else if (dirY < 0) { ex1--; ey1--; ex2++; ey2--; }
      else { ex1--; ey1++; ex2++; ey2++; }
      pantalla.setDrawColor(0);
      pantalla.drawPixel(ex1, ey1);
      pantalla.drawPixel(ex2, ey2);
      pantalla.setDrawColor(1);
    } else {
      pantalla.drawDisc(x, y, 2);
    }
  }
}
void dibujar() {
  pantalla.clearBuffer();
  pantalla.drawFrame(0, 0, 128, 64);

  // Comida: late cada 180 ms y lleva un brillo encima.
  int radio = 2;
  if (((millis() / 180) % 2) == 0) radio = 3;
  pantalla.drawDisc((int)comidaX, (int)comidaY, radio);
  pantalla.drawPixel((int)comidaX, (int)comidaY - radio - 1);

  dibujarSerpiente();
  pantalla.sendBuffer();
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  pantalla.begin();
  pantalla.setContrast(180);
  randomSeed(analogRead(A0) + micros());
  reiniciar();
}

void loop() {
  unsigned long ahora = millis();
  if (ahora - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = ahora;
  pensarMovimiento();
  actualizarSerpiente();
  dibujar();
}