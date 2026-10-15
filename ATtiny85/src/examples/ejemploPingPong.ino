#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - ejemplo de animacion: PING PONG
// Version ATtiny85.
//
// El mismo rebote del juego final, llevado al LCD de 16x2: la
// paleta del jugador va abajo y la de la maquina arriba, y la pelota
// rebota en diagonal entre las dos filas. Aqui las paletas se mueven
// solas para ver la animacion; en tinyPocket.ino las mueve el jugador
// con los botones.
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
const char CH_PELOTA = 0;
const char CH_PALETA = 1;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
char buffer[33];

const unsigned long FRAME_TIME = 220;
const int PUNTOS_GANAR = 5;

int8_t pelotaX = 8, pelotaY = 0, velX = 1, velY = 1;
int8_t paletaJugador = 8, paletaMaquina = 8;
uint8_t puntosJugador = 0, puntosMaquina = 0;
unsigned long ultimoFrame = 0;

void crearCaracteres() {
  uint8_t pelota[8] = {0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00};
  uint8_t paleta[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
  lcd.createChar(CH_PELOTA, pelota);
  lcd.createChar(CH_PALETA, paleta);
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
void reiniciarPelota() {
  pelotaX = paletaJugador;
  pelotaY = 1;
  velX = (random(0, 2) ? 1 : -1);
  velY = 1;
}
void actualizar() {
  // La maquina persigue la pelota mientras esta arriba.
  if (pelotaY == 0) {
    if (pelotaX > paletaMaquina) paletaMaquina++;
    else if (pelotaX < paletaMaquina) paletaMaquina--;
  }
  pelotaX = (int8_t)(pelotaX + velX);
  if (pelotaX <= 0) { pelotaX = 0; velX = 1; }
  if (pelotaX >= 15) { pelotaX = 15; velX = -1; }
  pelotaY = (int8_t)(pelotaY + velY);
  if (pelotaY <= 0) {
    pelotaY = 0;
    if (pelotaX - paletaMaquina <= 1 && paletaMaquina - pelotaX <= 1) velY = 1;
    else { puntosJugador++; if (puntosJugador >= PUNTOS_GANAR) puntosJugador = 0; reiniciarPelota(); }
  }
  if (pelotaY >= 1) {
    pelotaY = 1;
    if (pelotaX - paletaJugador <= 1 && paletaJugador - pelotaX <= 1) velY = -1;
    else { puntosMaquina++; if (puntosMaquina >= PUNTOS_GANAR) puntosMaquina = 0; reiniciarPelota(); }
  }
}
void setup() {
  randomSeed(analogRead(A0) + micros());
  Wire.begin();
  Wire.setClock(100000);
  lcd.init();
  lcd.backlight();
  crearCaracteres();
}
void loop() {
  if (millis() - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = millis();
  // Sin botones: ambas paletas las mueve el programa.
  if (pelotaX > paletaJugador && paletaJugador < 15) paletaJugador++;
  else if (pelotaX < paletaJugador && paletaJugador > 0) paletaJugador--;
  actualizar();
  borrarPantalla();
  poner((uint8_t)paletaMaquina, 0, CH_PALETA);
  poner((uint8_t)paletaJugador, 1, CH_PALETA);
  poner((uint8_t)pelotaX, (uint8_t)pelotaY, CH_PELOTA);
  char fila[17];
  uint8_t n = 0;
  fila[n++] = 'T'; fila[n++] = 'u'; fila[n++] = 's';
  fila[n++] = (char)('0' + puntosJugador);
  fila[n++] = ' ';
  fila[n++] = 'M'; fila[n++] = 'a'; fila[n++] = 'q';
  fila[n++] = (char)('0' + puntosMaquina);
  fila[n] = '\0';
  for (uint8_t i = 0; i < n; i++) buffer[16 + i] = fila[i];
  pintar();
}