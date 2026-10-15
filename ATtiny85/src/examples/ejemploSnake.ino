#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - ejemplo de animacion: SNAKE CON IA
// Version ATtiny85.
//
// IMPORTANTE: este ejemplo NO es el juego jugable. La serpiente se
// mueve sola con una IA que busca la comida, porque lo que se quiere
// ver aqui es el movimiento, la cabeza con ojos y la comida que late.
// El Snake CONTROLADO POR EL JUGADOR (con botones, sin IA y sin giros
// de 180 grados) esta en tinyPocket.ino.
//
// El tablero son las 32 celdas del LCD (16 columnas x 2 filas).
//
// Pantalla : LCD 16x2 I2C, direccion 0x27
// I2C      : SDA -> PB0, SCL -> PB2
// Botones  : PB3 (izq), PB4 (der), PB1 (OK) - no usados aqui
//
// Sin delay(): todo con millis().
//
// Compilar con:
//   placa  : ATtiny25/45/85 (No bootloader), ATTinyCore, 8 MHz interno
// ============================================================

#define LCD_ADDRESS 0x27
const char CH_CABEZA = 0;
const char CH_CUERPO = 1;
const char CH_COMIDA = 2;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
char buffer[33];

const int MAX_SEGMENTOS = 24;
uint8_t cuerpo[MAX_SEGMENTOS];
uint8_t longitud = 3;
uint8_t direccion = 1;          // 0 arriba, 1 derecha, 2 abajo, 3 izquierda
uint8_t comida = 0;
unsigned long ultimoFrame = 0;

void crearCaracteres() {
  uint8_t cabeza[8] = {0x00, 0x1F, 0x15, 0x1F, 0x1F, 0x1F, 0x00, 0x00};
  uint8_t cuerpoChar[8] = {0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00};
  uint8_t comidaChar[8] = {0x00, 0x06, 0x0F, 0x1F, 0x0F, 0x00, 0x00, 0x00};
  lcd.createChar(CH_CABEZA, cabeza);
  lcd.createChar(CH_CUERPO, cuerpoChar);
  lcd.createChar(CH_COMIDA, comidaChar);
}
void borrarPantalla() {
  for (uint8_t i = 0; i < 32; i++) buffer[i] = ' ';
  buffer[32] = '\0';
}
void poner(uint8_t col, uint8_t fila, char c) {
  if (col < 16) buffer[fila * 16 + col] = c;
}
void pintar() {
  buffer[16] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(buffer + 16);
}
uint8_t celda(uint8_t col, uint8_t fila) { return (uint8_t)(fila * 16 + col); }
void colocarComida() {
  bool ocupada;
  do {
    comida = (uint8_t)random(0, 32);
    ocupada = false;
    for (uint8_t i = 0; i < longitud; i++) if (cuerpo[i] == comida) ocupada = true;
  } while (ocupada && longitud < 32);
}
void reiniciar() {
  longitud = 3;
  cuerpo[0] = celda(8, 1);
  cuerpo[1] = celda(7, 1);
  cuerpo[2] = celda(6, 1);
  direccion = 1;
  colocarComida();
}
// IA sencilla: prueba las 4 direcciones y elige la que acerca mas a
// la comida sin golpear con el borde. En un tablero de 2 filas esto
// es suficiente y cabe de sobra en la memoria del ATtiny85.
void pensar() {
  int8_t mejor = -1;
  int16_t mejorDistancia = 32000;
  for (uint8_t i = 0; i < 4; i++) {
    if (i == (uint8_t)((direccion + 2) % 4)) continue;   // no media vuelta
    int8_t dx = (i == 1) ? 1 : (i == 3) ? -1 : 0;
    int8_t dy = (i == 0) ? -1 : (i == 2) ? 1 : 0;
    int8_t nx = (int8_t)((cuerpo[0] % 16) + dx);
    int8_t ny = (int8_t)((cuerpo[0] / 16) + dy);
    if (nx < 0 || nx > 15 || ny < 0 || ny > 1) continue;  // borde
    uint8_t destino = celda((uint8_t)nx, (uint8_t)ny);
    bool chocaria = false;
    for (uint8_t s = 1; s < longitud - 1; s++) if (cuerpo[s] == destino) chocaria = true;
    if (chocaria) continue;
    int16_t dx2 = (int16_t)nx - (int16_t)(comida % 16);
    int16_t dy2 = (int16_t)ny - (int16_t)(comida / 16);
    int16_t d = (int16_t)(dx2 * dx2 + dy2 * dy2);
    if (d < mejorDistancia) { mejorDistancia = d; mejor = (int8_t)i; }
  }
  if (mejor >= 0) direccion = (uint8_t)mejor;
}
void actualizar() {
  if (millis() - ultimoFrame < (longitud > 8 ? 150 : 250)) return;
  ultimoFrame = millis();
  for (uint8_t i = longitud - 1; i > 0; i--) cuerpo[i] = cuerpo[i - 1];
  int8_t nx = (int8_t)(cuerpo[0] % 16);
  int8_t ny = (int8_t)(cuerpo[0] / 16);
  if (direccion == 0) ny--;
  if (direccion == 1) nx++;
  if (direccion == 2) ny++;
  if (direccion == 3) nx--;
  if (nx < 0 || nx > 15 || ny < 0 || ny > 1) { reiniciar(); return; }
  cuerpo[0] = celda((uint8_t)nx, (uint8_t)ny);
  if (cuerpo[0] == comida) {
    if (longitud < MAX_SEGMENTOS) {
      cuerpo[longitud] = cuerpo[longitud - 1];
      longitud++;
    }
    colocarComida();
  }
}
void setup() {
  randomSeed(analogRead(A0) + micros());
  Wire.begin();
  Wire.setClock(100000);
  lcd.init();
  lcd.backlight();
  crearCaracteres();
  reiniciar();
}
void loop() {
  actualizar();
  pensar();
  borrarPantalla();
  poner(comida % 16, comida / 16, CH_COMIDA);
  for (uint8_t i = (uint8_t)(longitud - 1); i > 0; i--) poner(cuerpo[i] % 16, cuerpo[i] / 16, CH_CUERPO);
  poner(cuerpo[0] % 16, cuerpo[0] / 16, CH_CABEZA);
  char fila[17];
  uint8_t n = 0;
  fila[n++] = 'L'; fila[n++] = 'e'; fila[n++] = 'n'; fila[n++] = ' ';
  fila[n++] = (char)('0' + ((longitud / 10) % 10));
  fila[n++] = (char)('0' + (longitud % 10));
  while (n < 16) fila[n++] = ' ';
  fila[n] = '\0';
  for (uint8_t i = 0; i < n; i++) buffer[16 + i] = fila[i];
  pintar();
}