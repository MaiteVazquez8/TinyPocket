#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - ejemplo de animacion: CARTAS DE BLACKJACK
// Version Arduino Uno (ATmega328P).
//
// Muestra el reparto: las cartas entran volando desde los laterales,
// se colocan solas en el centro y la banca se revela al final.
//
// OLED 128x64 I2C: SDA -> A4, SCL -> A5 (pines I2C del Uno)
//
// La animacion NO es bloqueante: el reparto se hace con millis(),
// no con delay(). En tinyPocket.ino el mismo reparto se controla con
// botones: izquierda = pedir carta, derecha = plantarse.
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

// ============================================================
// PARAMETROS
// ============================================================

const unsigned long FRAME_TIME = 16;
const unsigned long TIEMPO_ENTRE_CARTAS = 800;   // una carta cada 800 ms
const float VELOCIDAD_ANIMACION = 0.18;          // ease-out suave
const int MAX_CARTAS = 10;
const int ANCHO_CARTA = 17, ALTO_CARTA = 19, ESPACIO = 3;
const int Y_CARTAS_CASA = 9, Y_SEPARADOR = 30, Y_CARTAS_JUGADOR = 39;

struct Carta { byte palo; byte valor; };
struct CartaAnimada {
  Carta carta;
  float x, y, destinoX, destinoY;
  bool jugador, oculta;
};

CartaAnimada cartas[MAX_CARTAS];
int cantidadCartas = 0;
int cartasJugador = 0, cartasCasa = 0;
unsigned long ultimaCarta = 0;
unsigned long ultimoFrame = 0;
int paso = 0;                      // 0..3 reparte, 4 terminado

// ============================================================
// ANIMACION
// ============================================================

int anchoGrupo(int cantidad) {
  if (cantidad <= 0) return 0;
  return cantidad * ANCHO_CARTA + (cantidad - 1) * ESPACIO;
}
void actualizarPosiciones() {
  int anchoJ = anchoGrupo(cartasJugador);
  int anchoC = anchoGrupo(cartasCasa);
  int inicioJ = (128 - anchoJ) / 2;
  int inicioC = (128 - anchoC) / 2;
  int idxJ = 0, idxC = 0;
  for (int i = 0; i < cantidadCartas; i++) {
    if (cartas[i].jugador) {
      cartas[i].destinoX = inicioJ + idxJ * (ANCHO_CARTA + ESPACIO);
      cartas[i].destinoY = Y_CARTAS_JUGADOR;
      idxJ++;
    } else {
      cartas[i].destinoX = inicioC + idxC * (ANCHO_CARTA + ESPACIO);
      cartas[i].destinoY = Y_CARTAS_CASA;
      idxC++;
    }
  }
}
void agregarCarta(bool jugador, bool oculta) {
  if (cantidadCartas >= MAX_CARTAS) return;
  CartaAnimada &c = cartas[cantidadCartas];
  c.carta.palo = (byte)random(0, 4);
  c.carta.valor = (byte)random(1, 13);
  c.jugador = jugador;
  c.oculta = oculta;
  // Alterna el lado por el que entra cada carta para que el reparto
  // no salga siempre por el mismo.
  if ((jugador && cartasJugador % 2 == 0) || (!jugador && cartasCasa % 2 == 0)) c.x = -20;
  else c.x = 148;
  c.y = jugador ? Y_CARTAS_JUGADOR : Y_CARTAS_CASA;
  c.destinoX = c.x;
  c.destinoY = c.y;
  cantidadCartas++;
  if (jugador) cartasJugador++; else cartasCasa++;
  actualizarPosiciones();
}
void actualizarAnimaciones() {
  for (int i = 0; i < cantidadCartas; i++) {
    float dx = cartas[i].destinoX - cartas[i].x;
    float dy = cartas[i].destinoY - cartas[i].y;
    cartas[i].x += dx * VELOCIDAD_ANIMACION;
    cartas[i].y += dy * VELOCIDAD_ANIMACION;
    if (fabs(dx) < 0.1) cartas[i].x = cartas[i].destinoX;
    if (fabs(dy) < 0.1) cartas[i].y = cartas[i].destinoY;
  }
}
void nuevaPartida() {
  cantidadCartas = 0; cartasJugador = 0; cartasCasa = 0; paso = 0;
  ultimaCarta = millis();
}
void repartir() {
  if (paso >= 4) return;                         // ya se repartieron las cuatro
  if (millis() - ultimaCarta < TIEMPO_ENTRE_CARTAS) return;
  switch (paso) {
    case 0: agregarCarta(true, false);  break;
    case 1: agregarCarta(false, false); break;
    case 2: agregarCarta(true, false);  break;
    case 3: agregarCarta(false, true);  break;
  }
  paso++;
  ultimaCarta = millis();
}

// ============================================================
// DIBUJO
// ============================================================

// Triangulo relleno, porque U8g2 solo trae el de contorno.
void llenarTriangulo(int x0, int y0, int x1, int y1, int x2, int y2) {
  int xs[3] = {x0, x1, x2};
  int ys[3] = {y0, y1, y2};
  for (int y = min(min(y0, y1), y2); y <= max(max(y0, y1), y2); y++) {
    int xMin = 128, xMax = -1;
    for (int i = 0; i < 3; i++) {
      int j = (i + 1) % 3;
      int a = ys[i], b = ys[j];
      if ((a <= y && y < b) || (b <= y && y < a)) {
        int xi = xs[i] + (int)((long)(y - a) * (xs[j] - xs[i]) / (b - a));
        if (xi < xMin) xMin = xi;
        if (xi > xMax) xMax = xi;
      }
    }
    if (xMax >= xMin) {
      if (xMin < 0) xMin = 0;
      if (xMax > 127) xMax = 127;
      pantalla.drawHLine(xMin, y, xMax - xMin + 1);
    }
  }
}
void dibujarPalo(int x, int y, byte palo) {
  if (palo == 0) {                                  // corazon
    pantalla.drawDisc(x + 4, y + 4, 3);
    pantalla.drawDisc(x + 9, y + 4, 3);
    llenarTriangulo(x + 1, y + 5, x + 12, y + 5, x + 6, y + 12);
  } else if (palo == 1) {                           // diamante
    llenarTriangulo(x + 6, y, x + 12, y + 6, x + 6, y + 12);
    llenarTriangulo(x + 6, y, x, y + 6, x + 6, y + 12);
  } else if (palo == 2) {                           // trebol
    pantalla.drawDisc(x + 3, y + 4, 3);
    pantalla.drawDisc(x + 9, y + 4, 3);
    pantalla.drawDisc(x + 6, y + 8, 3);
    llenarTriangulo(x + 6, y + 7, x + 3, y + 13, x + 9, y + 13);
  } else {                                          // picas
    llenarTriangulo(x + 6, y, x, y + 8, x + 12, y + 8);
    pantalla.drawDisc(x + 3, y + 8, 3);
    pantalla.drawDisc(x + 9, y + 8, 3);
    pantalla.drawBox(x + 5, y + 7, 3, 6);
  }
}
void dibujarCarta(CartaAnimada &c) {
  int x = (int)c.x, y = (int)c.y;
  if (c.oculta) {
    pantalla.drawRBox(x, y, ANCHO_CARTA, ALTO_CARTA, 2);
    return;
  }
  pantalla.drawRBox(x, y, ANCHO_CARTA, ALTO_CARTA, 2);
  pantalla.setDrawColor(0);
  pantalla.setFont(u8g2_font_4x6_tf);
  // El valor se escribe a mano: no se usa String ni print() para no
  // meter memoria dinamica ni formateo de numeros en un ejemplo.
  char v[3];
  v[0] = '\0';
  if (c.carta.valor == 1) { v[0] = 'A'; v[1] = '\0'; }
  else if (c.carta.valor == 11) { v[0] = 'J'; v[1] = '\0'; }
  else if (c.carta.valor == 12) { v[0] = 'Q'; v[1] = '\0'; }
  else if (c.carta.valor == 13) { v[0] = 'K'; v[1] = '\0'; }
  else if (c.carta.valor == 10) { v[0] = '1'; v[1] = '0'; v[2] = '\0'; }
  else { v[0] = (char)('0' + c.carta.valor); v[1] = '\0'; }
  pantalla.drawStr(x + 2, y + 5, v);
  dibujarPalo(x + 2, y + 6, c.carta.palo);
  pantalla.setDrawColor(1);
}
void dibujar() {
  pantalla.clearBuffer();
  pantalla.setFont(u8g2_font_4x6_tf);
  pantalla.drawStr(2, 7, "BLACKJACK");
  pantalla.drawHLine(0, Y_SEPARADOR, 128);
  pantalla.drawStr(2, 37, "REPARTO");
  for (int i = 0; i < cantidadCartas; i++) dibujarCarta(cartas[i]);
  pantalla.sendBuffer();
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  pantalla.begin();
  pantalla.setContrast(180);
  randomSeed(analogRead(A0) + micros());
  nuevaPartida();
}

void loop() {
  unsigned long ahora = millis();
  if (ahora - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = ahora;
  repartir();
  actualizarAnimaciones();
  dibujar();
}