#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - ejemplo de animacion: BREAKOUT
// Version ATtiny85.
//
// En un LCD de 16x2 no caben los 32 ladrillos del OLED. Aqui la fila
// de arriba es una muralla de 16 ladrillos (un solo caracter cada uno)
// y la paleta se mueve en la fila de abajo, rebotando la pelota en
// diagonal y rompiendo ladrillos. Es la misma idea del juego del
// OLED, comprimida a lo que cabe en 32 celdas.
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
const char CH_LADRILLO = 2;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
char buffer[33];

const unsigned long FRAME_TIME = 250;
const uint8_t VIDAS = 3;

// La muralla es una mascara de 16 bits: 1 bit por ladrillo. Asi ocupa
// 2 bytes en vez de los 16 de un array de booleanos.
uint16_t muralla = 0xFFFF;
int8_t pelotaX = 8, pelotaY = 1, velX = 1, velY = -1;
int8_t paleta = 8;
uint8_t vidas = VIDAS;
bool terminado = false, ganado = false;
unsigned long ultimoFrame = 0;

void crearCaracteres() {
  uint8_t pelota[8] = {0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00};
  uint8_t paletaChar[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
  uint8_t ladrillo[8] = {0x00, 0x0E, 0x1F, 0x1F, 0x0E, 0x00, 0x00, 0x00};
  lcd.createChar(CH_PELOTA, pelota);
  lcd.createChar(CH_PALETA, paletaChar);
  lcd.createChar(CH_LADRILLO, ladrillo);
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
void reiniciar() {
  muralla = 0xFFFF;
  pelotaX = 8; pelotaY = 1; velX = 1; velY = -1;
  paleta = 8; vidas = VIDAS;
  terminado = false; ganado = false;
}
void actualizar() {
  if (terminado) {
    if (millis() - ultimoFrame > 2000) reiniciar();
    return;
  }
  if (millis() - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = millis();

  pelotaX = (int8_t)(pelotaX + velX);
  if (pelotaX <= 0) { pelotaX = 0; velX = 1; }
  if (pelotaX >= 15) { pelotaX = 15; velX = -1; }

  pelotaY = (int8_t)(pelotaY + velY);
  if (pelotaY <= 0) {
    pelotaY = 0;
    uint16_t bit = (uint16_t)1 << pelotaX;
    if (muralla & bit) {
      muralla &= (uint16_t)~bit;
      velY = 1;
      if (muralla == 0) { terminado = true; ganado = true; }
    } else {
      velY = 1;
    }
  }
  if (pelotaY >= 1) {
    pelotaY = 1;
    if (pelotaX - paleta <= 1 && paleta - pelotaX <= 1) {
      velY = -1;
    } else {
      vidas--;
      if (vidas == 0) { terminado = true; ganado = false; }
      else { pelotaX = paleta; pelotaY = 1; velY = -1; }
    }
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
  actualizar();
  borrarPantalla();
  if (terminado) {
    buffer[0] = ganado ? 'G' : 'F';
    buffer[1] = 'a'; buffer[2] = 'n'; buffer[3] = ganado ? 'o' : 'i';
    buffer[4] = ganado ? 's' : 'n'; buffer[5] = 't'; buffer[6] = 'e';
    buffer[7] = '!';
    for (uint8_t i = 0; i < 16; i++) buffer[16 + i] = (i == 0) ? ' ' : buffer[16 + i];
  } else {
    for (uint8_t i = 0; i < 16; i++)
      if (muralla & ((uint16_t)1 << i)) poner(i, 0, CH_LADRILLO);
    poner((uint8_t)paleta, 1, CH_PALETA);
    poner((uint8_t)pelotaX, (uint8_t)pelotaY, CH_PELOTA);
  }
  char fila[17];
  uint8_t n = 0;
  fila[n++] = 'V'; fila[n++] = 'i'; fila[n++] = 'd'; fila[n++] = 'a';
  fila[n++] = ' '; fila[n++] = (char)('0' + vidas);
  while (n < 16) fila[n++] = ' ';
  fila[n] = '\0';
  for (uint8_t i = 0; i < n; i++) buffer[16 + i] = fila[i];
  pintar();
}