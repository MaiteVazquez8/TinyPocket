#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - ejemplo de animacion: CARTAS DE BLACKJACK
// Version ATtiny85.
//
// En el OLED las cartas son rectangulos que entran volando. En el LCD
// de 16x2 no hay pixeles libres, asi que el reparto se ve como las
// cartas apareciendo de una en una. La animacion conserva lo
// importante: el orden del reparto, la carta oculta de la banca y la
// revelacion final.
//
// Pantalla : LCD 16x2 I2C, direccion 0x27
// I2C      : SDA -> PB0, SCL -> PB2
// Botones  : PB3 (izq), PB4 (der), PB1 (OK) - no usados aqui
//
// Sin delay(): el reparto se temporiza con millis().
//
// Compilar con:
//   placa  : ATtiny25/45/85 (No bootloader), ATTinyCore, 8 MHz interno
// ============================================================

#define LCD_ADDRESS 0x27

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
char buffer[33];

const int MAX_CARTAS = 5;
const unsigned long TIEMPO_CARTA = 700;
const uint8_t META_VICTORIAS = 3, META_DERROTAS = 3;

uint8_t jugador[MAX_CARTAS], casa[MAX_CARTAS];
uint8_t cartasJugador = 0, cartasCasa = 0;
uint8_t victoria = 0, derrota = 0;
uint8_t totalJugador = 0, totalCasa = 0;
uint8_t paso = 0;              // 0..3 reparte, 4 cerrada, 5 repartir otra vez
bool oculta = true;
unsigned long ultimaCarta = 0;

void borrarPantalla() {
  for (uint8_t i = 0; i < 32; i++) buffer[i] = ' ';
  buffer[32] = '\0';
}
void pintar() {
  buffer[16] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(buffer + 16);
}
uint8_t valor(uint8_t v) { return v == 1 ? 11 : v; }
uint8_t total(uint8_t *cartas, uint8_t cantidad, bool incluirOculta) {
  uint8_t suma = 0, ases = 0;
  for (uint8_t i = 0; i < cantidad; i++) {
    suma += valor(cartas[i]);
    if (cartas[i] == 1) ases++;
  }
  while (suma > 21 && ases > 0) { suma -= 10; ases--; }
  return suma;
}
void nuevaRonda() {
  cartasJugador = 0; cartasCasa = 0;
  victoria = 0; derrota = 0;
  paso = 0; oculta = true;
  ultimaCarta = millis();
}
void repartir() {
  if (paso >= 4) return;
  if (millis() - ultimaCarta < TIEMPO_CARTA) return;
  uint8_t v = (uint8_t)random(1, 11);
  if (paso % 2 == 0 && cartasJugador < MAX_CARTAS) jugador[cartasJugador++] = v;
  else if (cartasCasa < MAX_CARTAS) casa[cartasCasa++] = v;
  paso++;
  ultimaCarta = millis();
}
void jugar() {
  oculta = false;
  totalJugador = total(jugador, cartasJugador, true);
  totalCasa = total(casa, cartasCasa, true);
  if (totalJugador > 21 || totalCasa > totalJugador) derrota++;
  else if (totalCasa < totalJugador || totalCasa > 21) victoria++;
  paso = 4;
}
void setup() {
  randomSeed(analogRead(A0) + micros());
  Wire.begin();
  Wire.setClock(100000);
  lcd.init();
  lcd.backlight();
  nuevaRonda();
}
void loop() {
  unsigned long ahora = millis();
  if (paso < 4) {
    repartir();
  } else if (ahora - ultimaCarta > 2500) {
    // Cerrada la mano: juega la banca hasta 17 y se reabre la ronda.
    while (total(casa, cartasCasa, true) < 17 && cartasCasa < MAX_CARTAS)
      casa[cartasCasa++] = (uint8_t)random(1, 11);
    jugar();
    ultimaCarta = ahora;
    paso = 5;
  } else if (paso == 5 && ahora - ultimaCarta > 3000) {
    if (victoria >= META_VICTORIAS || derrota >= META_DERROTAS) nuevaRonda();
    else {
      jugador[0] = (uint8_t)random(1, 11); jugador[1] = (uint8_t)random(1, 11);
      cartasJugador = 2;
      casa[0] = (uint8_t)random(1, 11); casa[1] = (uint8_t)random(1, 11);
      cartasCasa = 2;
      oculta = true;
      paso = 0;
      ultimaCarta = ahora;
    }
  }

  borrarPantalla();
  char fila[17];
  uint8_t n = 0;
  fila[n++] = 'C'; fila[n++] = 'a'; fila[n++] = 's';
  for (uint8_t i = 0; i < cartasCasa; i++) {
    fila[n++] = ' ';
    if (i == 1 && oculta && paso < 4) fila[n++] = '?';
    else fila[n++] = (char)('0' + (valor(casa[i]) % 10));
  }
  fila[n] = '\0';
  for (uint8_t i = 0; i < n; i++) buffer[i] = fila[i];

  n = 0;
  fila[n++] = 'T'; fila[n++] = 'u';
  for (uint8_t i = 0; i < cartasJugador; i++) {
    fila[n++] = ' ';
    fila[n++] = (char)('0' + (valor(jugador[i]) % 10));
  }
  while (n < 12) fila[n++] = ' ';
  fila[n++] = (char)('0' + ((total(jugador, cartasJugador, true) / 10) % 10));
  fila[n++] = (char)('0' + (total(jugador, cartasJugador, true) % 10));
  fila[n++] = ' ';
  fila[n++] = (char)('0' + victoria);
  fila[n++] = '-';
  fila[n++] = (char)('0' + derrota);
  fila[n] = '\0';
  for (uint8_t i = 0; i < n; i++) buffer[16 + i] = fila[i];
  pintar();
}