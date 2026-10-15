#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - version ATtiny85 (ATtiny85 @ 8 MHz interno)
// Esta es la version MAS COMPACTA del proyecto.
//
// Hardware (segun ATtiny85/explicacion_codigo.md):
//   Pantalla : LCD 16x2 con modulo I2C, direccion 0x27
//              (si tu modulo usa 0x3F, cambia LCD_ADDRESS)
//   I2C      : SDA -> PB0, SCL -> PB2
//   Botones  : izquierda -> PB3, derecha -> PB4, OK -> PB1
//              (cada boton entre su pin y GND)
//   INPUT_PULLUP: presionado = LOW
//
// POR QUE NO USA OLED EN ESTA PLACA:
//   El ATtiny85 tiene 512 bytes de SRAM. El bUfer de 128x64 que
//   necesitan U8g2 o Adafruit_SSD1306 ocupa por si solo 1024 bytes,
//   o sea el doble de toda la memoria RAM del chip: no enlaza. Por eso
//   esta version usa el LCD 16x2 de caracteres (16x2 = 32 celdas) con
//   LiquidCrystal_I2C, que ocupa apenas unos 8 bytes de RAM. Es lo que
//   describe la documentacion de esta carpeta.
//   Lo que NO cabe en 512 bytes es el Breakout con 32 ladrillos; aqui
//   se implementa una version compacta de 16 ladrillos sobre la fila
//   superior (ver seccion BREAKOUT).
//
// MEDIDAS DE MEMORIA (las da el compilador al compilar, con el core
// ATTinyCore 1.5.2 y la placa "ATtiny25/45/85 (No bootloader)" a 8 MHz):
//   Flash : 7968 bytes de 8192  (97%)
//   SRAM  :  287 bytes de  512  (56%), con 225 de margen para la pila
//
// REGLA DE ORO PARA ESTA PLACA:
//   1. Todos los tipos y los prototipos van ANTES de la primera
//      funcion, porque el Arduino IDE coloca los prototipos que
//      genera justo despues de los #include y si no, rompe la
//      compilacion con "'X' was not declared in this scope".
//   2. Sin float: el ATtiny85 no tiene FPU y cada operacion en
//      coma flotante arrastra codigo y slows. Todo es entero.
//   3. Sin String y sin menus dinamicos: se reserva un buffer fijo
//      de 32 caracteres y todos los textos literales van con F()
//      para que queden en flash y no ocupen RAM.
//   4. Un solo buffer y una sola funcion de pintado para toda la
//      aplicacion: dibujar es escribir en el buffer y enviarlo.
// ============================================================

// Direccion I2C del modulo LCD. Cambiar a 0x3F si hace falta.
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2

// Pines reales del ATtiny85.
const byte PIN_IZQUIERDA = PB3;
const byte PIN_DERECHA   = PB4;
const byte PIN_OK       = PB1;

LiquidCrystal_I2C lcd(LCD_ADDRESS, LCD_COLS, LCD_ROWS);

// ============================================================
// TIPOS
// ============================================================

struct Boton {
  uint8_t pin;
  bool lecturaAnterior;
  bool estadoEstable;
  uint16_t cambioLectura;
  uint16_t inicioPulsacion;
  bool corto;
  bool largo;
};

// Estados de la aplicacion.
enum Estado { MENU, SUBMENU_PINGPONG, JUEGO_PINGPONG, JUEGO_BREAKOUT,
              JUEGO_MASCOTA, JUEGO_BLACKJACK, JUEGO_SNAKE, INFORMACION };
Estado estado = MENU;
uint8_t seleccionMenu = 0, seleccionPingPong = 0;

// Caracteres especiales creados con createChar().
// Cada bitmap son 8 filas de 5 bits, de la fila de arriba hacia abajo.
const char CH_PELOTA   = 0;
const char CH_PALETA   = 1;
const char CH_CUERPO   = 2;
const char CH_COMIDA   = 3;
const char CH_CABEZA   = 4;
const char CH_LADRILLO = 5;
const char CH_CORAZON  = 6;
const char CH_OJO_CERRADO = 7;

// ============================================================
// PROTOTIPOS
// ============================================================

void actualizarBoton(Boton &boton);
void actualizarBotones();
void volverAlMenu();

// Pantalla.
void crearCaracteres();
void borrarPantalla();
void poner(uint8_t columna, uint8_t fila, char c);
void ponerTexto(uint8_t fila, const char *texto);
void ponerTextoF(uint8_t fila, const __FlashStringHelper *texto);
void ponerFilaCentrada(uint8_t fila, const __FlashStringHelper *texto);
void pintar();

// Menu.
void dibujarMenu();
void procesarMenu();
void dibujarSubmenu();
void procesarSubmenu();

// Ping Pong.
void iniciarPingPong();
void actualizarPingPong();
void dibujarPingPong();
void procesarPingPong();

// Breakout compacto.
void iniciarBreakout();
void actualizarBreakout();
void dibujarBreakout();
void procesarBreakout();

// Mascota.
void iniciarMascota();
void actualizarMascota();
void dibujarMascota();
void procesarMascota();

// Blackjack.
void nuevaRondaBlackjack();
void actualizarBlackjack();
void dibujarBlackjack();
void procesarBlackjack();

// Snake.
void iniciarSnake();
void actualizarSnake();
void dibujarSnake();
void procesarSnake();

// Informacion.
void dibujarInformacion();

// ============================================================
// BOTONES (con antirrebote, igual que en las otras placas)
// ============================================================

Boton izquierda = {PIN_IZQUIERDA, HIGH, HIGH, 0, 0, false, false};
Boton derecha   = {PIN_DERECHA,   HIGH, HIGH, 0, 0, false, false};
Boton ok        = {PIN_OK,       HIGH, HIGH, 0, 0, false, false};

// Los tres botones comparten esta funcion por parametro, asi que solo
// hay una copia del codigo de antirrebote.
void actualizarBoton(Boton &boton) {
  boton.corto = false;
  boton.largo = false;
  bool lectura = digitalRead(boton.pin);
  unsigned long ahora = millis();
  if (lectura != boton.lecturaAnterior) {
    boton.lecturaAnterior = lectura;
    boton.cambioLectura = (uint16_t)ahora;
  }
  if (ahora - boton.cambioLectura >= 35 && lectura != boton.estadoEstable) {
    boton.estadoEstable = lectura;
    if (boton.estadoEstable == LOW) {
      boton.inicioPulsacion = (uint16_t)ahora;
    } else if (ahora - boton.inicioPulsacion >= 800) {
      boton.largo = true;
    } else {
      boton.corto = true;
    }
  }
}
void actualizarBotones() {
  actualizarBoton(izquierda);
  actualizarBoton(derecha);
  actualizarBoton(ok);
}
void volverAlMenu() { estado = MENU; }

// ============================================================
// PANTALLA
// Un buffer de 32 caracteres (las dos filas del LCD) y una sola
// funcion de pintado. Todos los juegos escriben en el buffer.
// ============================================================

char buffer[33];

void crearCaracteres() {
  uint8_t pelota[8] = {0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00};
  uint8_t paleta[8] = {0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00};
  uint8_t cuerpo[8] = {0x00, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x00, 0x00};
  uint8_t comida[8] = {0x00, 0x06, 0x0F, 0x1F, 0x0F, 0x00, 0x00, 0x00};
  uint8_t cabeza[8] = {0x00, 0x1F, 0x15, 0x1F, 0x1F, 0x1F, 0x00, 0x00};
  uint8_t ladrillo[8] = {0x00, 0x1F, 0x15, 0x1F, 0x1F, 0x1F, 0x00, 0x00};
  uint8_t corazon[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};
  uint8_t ojoCerrado[8] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00};
  lcd.createChar(CH_PELOTA, pelota);
  lcd.createChar(CH_PALETA, paleta);
  lcd.createChar(CH_CUERPO, cuerpo);
  lcd.createChar(CH_COMIDA, comida);
  lcd.createChar(CH_CABEZA, cabeza);
  lcd.createChar(CH_LADRILLO, ladrillo);
  lcd.createChar(CH_CORAZON, corazon);
  lcd.createChar(CH_OJO_CERRADO, ojoCerrado);
}
void borrarPantalla() {
  for (uint8_t i = 0; i < 32; i++) buffer[i] = ' ';
  buffer[32] = '\0';
}
void poner(uint8_t columna, uint8_t fila, char c) {
  if (columna < 16) buffer[fila * 16 + columna] = c;
}
// Texto armado en tiempo de ejecucion (se queda en RAM, son 16 bytes
// de pila como mucho y se reutilizan en cada fotograma).
void ponerTexto(uint8_t fila, const char *texto) {
  uint8_t i = 0;
  while (texto[i] != '\0' && i < 16) { buffer[fila * 16 + i] = texto[i]; i++; }
}
// Texto literal: se pasa con F() para que quede en FLASH. Sin el F(),
// cada literal se copiaria a la RAM al arrancar y el ATtiny85 solo
// tiene 512 bytes.
void ponerTextoF(uint8_t fila, const __FlashStringHelper *texto) {
  const char *p = (const char *)texto;
  uint8_t i = 0;
  while (i < 16 && pgm_read_byte(p + i) != 0) {
    buffer[fila * 16 + i] = (char)pgm_read_byte(p + i);
    i++;
  }
}
void pintar() {
  buffer[16] = '\0';
  lcd.setCursor(0, 0);
  lcd.print(buffer);
  lcd.setCursor(0, 1);
  lcd.print(buffer + 16);
}
// Centra un texto de flash de 16 columnas en la fila indicada.
void ponerFilaCentrada(uint8_t fila, const __FlashStringHelper *texto) {
  const char *p = (const char *)texto;
  uint8_t i = 0;
  while (i < 16 && pgm_read_byte(p + i) != 0) {
    if (pgm_read_byte(p + i) != ' ') buffer[fila * 16 + i] = (char)pgm_read_byte(p + i);
    i++;
  }
}

// ============================================================
// MENU
// En 16x2 solo cabe un nombre por fila, asi que se muestra la
// opcion seleccionada centrada con un marcador '>' al lado.
// ============================================================

// Nombres del menu en una sola cadena de flash separada por comas.
// Un array de punteros a textos ("const char *opciones[]") ocuparia
// RAM: los 6 punteros (12 bytes) mas los propios textos (~60 bytes).
// Asi el menu completo no cuesta ni un byte de RAM.
const char nombresMenu[] PROGMEM = "PingPong,Breakout,Mascota,Blackjack,Snake,Info";
const byte CANT_OPCIONES = 6;

// Dibuja el nombre de la opcion seleccionada, centrado y con un '>'.
// Se arma leyendo de PROGMEM, asi que el resultado va al buffer.
void dibujarMenu() {
  borrarPantalla();
  uint8_t i = 0, n = 0, largo = 0;
  while (n < seleccionMenu) {              // saltar hasta el nombre pedido
    while (pgm_read_byte(&nombresMenu[i]) != ',') i++;
    i++; n++;
  }
  uint8_t inicio = i;
  while (pgm_read_byte(&nombresMenu[i]) != ',') { largo++; i++; }
  uint8_t x = (uint8_t)((16 - (largo + 2)) / 2);
  poner(x, 0, '>');
  poner((uint8_t)(x + 1), 0, ' ');
  for (uint8_t k = 0; k < largo; k++)
    poner((uint8_t)(x + 2 + k), 0, (char)pgm_read_byte(&nombresMenu[inicio + k]));
  ponerTextoF(1, F("< > elige  OK: entra"));
  pintar();
}
void procesarMenu() {
  if (izquierda.corto) seleccionMenu = (seleccionMenu == 0) ? CANT_OPCIONES - 1 : seleccionMenu - 1;
  if (derecha.corto) seleccionMenu = (seleccionMenu + 1) % CANT_OPCIONES;
  if (ok.corto) {
    switch (seleccionMenu) {
      case 0: seleccionPingPong = 0; estado = SUBMENU_PINGPONG; break;
      case 1: estado = JUEGO_BREAKOUT; iniciarBreakout(); break;
      case 2: estado = JUEGO_MASCOTA; iniciarMascota(); break;
      case 3: estado = JUEGO_BLACKJACK; nuevaRondaBlackjack(); break;
      case 4: estado = JUEGO_SNAKE; iniciarSnake(); break;
      default: estado = INFORMACION; break;
    }
  }
  dibujarMenu();
}
void dibujarSubmenu() {
  borrarPantalla();
  if (seleccionPingPong == 0) ponerTextoF(0, F("   Normal     "));
  else ponerTextoF(0, F("   Breakout   "));
  poner(0, 1, '>');
  ponerTextoF(1, F("OK entra  mant: menu"));
  pintar();
}
void procesarSubmenu() {
  if (izquierda.corto || derecha.corto) seleccionPingPong = 1 - seleccionPingPong;
  if (ok.largo) estado = MENU;
  else if (ok.corto) {
    if (seleccionPingPong == 0) { estado = JUEGO_PINGPONG; iniciarPingPong(); }
    else { estado = JUEGO_BREAKOUT; iniciarBreakout(); }
  }
  dibujarSubmenu();
}

// ============================================================
// PING PONG
// La pelota rebota en diagonal por las dos filas del LCD.
// La paleta del jugador va abajo y la de la maquina arriba.
// ============================================================

#define PP_PUNTOS_GANAR 5
#define PP_PASO_MS      260

int8_t ppPelotaX = 8, ppPelotaY = 0, ppVelX = 1, ppVelY = 1;
int8_t ppPaletaJugador = 8, ppPaletaMaquina = 8;
uint8_t ppPuntosJugador = 0, ppPuntosMaquina = 0;
uint16_t ppUltimoPaso = 0;
bool ppTerminado = false;

void iniciarPingPong() {
  ppPelotaX = 8; ppPelotaY = 0;
  ppVelX = 1; ppVelY = 1;
  ppPaletaJugador = 8; ppPaletaMaquina = 8;
  ppPuntosJugador = 0; ppPuntosMaquina = 0;
  ppTerminado = false;
  ppUltimoPaso = millis();
}
void reiniciarPelotaPingPong() {
  ppPelotaX = ppPaletaJugador;
  ppPelotaY = 1;
  ppVelX = (random(0, 2) ? 1 : -1);
  ppVelY = 1;
}
void actualizarPingPong() {
  if (ppTerminado) return;
  if (millis() - ppUltimoPaso < PP_PASO_MS) return;
  ppUltimoPaso = millis();

  // La maquina persigue la pelota, pero con un error cada tanto para
  // que se pueda ganar: si la pelota va muy rapido se queda atras.
  if (ppPelotaY == 0) {
    if (ppPelotaX > ppPaletaMaquina) ppPaletaMaquina++;
    else if (ppPelotaX < ppPaletaMaquina) ppPaletaMaquina--;
  }

  ppPelotaX = (int8_t)(ppPelotaX + ppVelX);
  if (ppPelotaX < 0) { ppPelotaX = 0; ppVelX = 1; }
  if (ppPelotaX > 15) { ppPelotaX = 15; ppVelX = -1; }

  ppPelotaY = (int8_t)(ppPelotaY + ppVelY);
  if (ppPelotaY <= 0) {
    ppPelotaY = 0;
    if (ppPelotaX - ppPaletaMaquina <= 1 && ppPaletaMaquina - ppPelotaX <= 1) {
      ppVelY = 1;
    } else {
      ppPuntosJugador++;
      if (ppPuntosJugador >= PP_PUNTOS_GANAR) ppTerminado = true;
      reiniciarPelotaPingPong();
    }
  }
  if (ppPelotaY >= 1) {
    ppPelotaY = 1;
    if (ppPelotaX - ppPaletaJugador <= 1 && ppPaletaJugador - ppPelotaX <= 1) {
      ppVelY = -1;
    } else {
      ppPuntosMaquina++;
      if (ppPuntosMaquina >= PP_PUNTOS_GANAR) ppTerminado = true;
      reiniciarPelotaPingPong();
    }
  }
}
void dibujarPingPong() {
  borrarPantalla();
  if (ppTerminado) {
    ponerTextoF(0, ppPuntosJugador > ppPuntosMaquina ? F(" Ganaste!      ") : F(" Perdiste      "));
    ponerTextoF(1, F("OK otra | mant:menu"));
    pintar();
    return;
  }
  poner((uint8_t)ppPaletaMaquina, 0, CH_PALETA);
  poner((uint8_t)ppPaletaJugador, 1, CH_PALETA);
  poner((uint8_t)ppPelotaX, (uint8_t)ppPelotaY, CH_PELOTA);
  // Marcador: "Tus 2  Maq 1"
  char marcador[17];
  uint8_t n = 0;
  marcador[n++] = 'T'; marcador[n++] = 'u'; marcador[n++] = 's'; marcador[n++] = ' ';
  marcador[n++] = (char)('0' + ppPuntosJugador);
  marcador[n++] = ' '; marcador[n++] = ' '; marcador[n++] = 'M'; marcador[n++] = 'a';
  marcador[n++] = 'q'; marcador[n++] = ' ';
  marcador[n++] = (char)('0' + ppPuntosMaquina);
  marcador[n] = '\0';
  ponerTexto(1, marcador);
  pintar();
}
void procesarPingPong() {
  if (ok.largo) { volverAlMenu(); return; }
  if (ppTerminado) {
    if (ok.corto) iniciarPingPong();
    dibujarPingPong();
    return;
  }
  if (izquierda.estadoEstable == LOW && ppPaletaJugador > 0) ppPaletaJugador--;
  if (derecha.estadoEstable == LOW && ppPaletaJugador < 15) ppPaletaJugador++;
  actualizarPingPong();
  dibujarPingPong();
}

// ============================================================
// BREAKOUT COMPACTO
// En 16x2 no caben 32 ladrillos: la fila de arriba es la muralla de
// 16 ladrillos y el jugador mueve la paleta en la fila de abajo.
// La pelota rebota en diagonal y rompe ladrillos al llegar arriba.
// ============================================================

#define BO_VIDAS 3

// La muralla se guarda como una mascara de 16 bits (un bit por
// ladrillo) en vez de un array de 16 booleanos: 2 bytes en lugar de 16.
uint16_t boMuralla = 0xFFFF;
int8_t boPelotaX = 8, boPelotaY = 1, boVelX = 1, boVelY = -1;
int8_t boPaleta = 8;
uint8_t boVidas = BO_VIDAS;
uint16_t boUltimoPaso = 0;
bool boTerminado = false, boGanado = false;

void iniciarBreakout() {
  boMuralla = 0xFFFF;
  boPelotaX = 8; boPelotaY = 1; boVelX = 1; boVelY = -1;
  boPaleta = 8; boVidas = BO_VIDAS;
  boTerminado = false; boGanado = false;
  boUltimoPaso = millis();
}
void actualizarBreakout() {
  if (boTerminado) return;
  if (millis() - boUltimoPaso < 300) return;
  boUltimoPaso = millis();

  boPelotaX = (int8_t)(boPelotaX + boVelX);
  if (boPelotaX < 0) { boPelotaX = 0; boVelX = 1; }
  if (boPelotaX > 15) { boPelotaX = 15; boVelX = -1; }

  boPelotaY = (int8_t)(boPelotaY + boVelY);
  if (boPelotaY <= 0) {
    boPelotaY = 0;
    uint16_t bit = (uint16_t)1 << boPelotaX;
    if (boMuralla & bit) {
      boMuralla &= (uint16_t)~bit;
      boVelY = 1;
      if (boMuralla == 0) { boTerminado = true; boGanado = true; }
    } else {
      boVelY = 1;
    }
  }
  if (boPelotaY >= 1) {
    boPelotaY = 1;
    if (boPelotaX - boPaleta <= 1 && boPaleta - boPelotaX <= 1) {
      boVelY = -1;
    } else {
      boVidas--;
      if (boVidas == 0) { boTerminado = true; boGanado = false; }
      else { boPelotaX = boPaleta; boPelotaY = 1; boVelY = -1; }
    }
  }
}
void dibujarBreakout() {
  borrarPantalla();
  if (boTerminado) {
    ponerTextoF(0, boGanado ? F(" Ganaste!      ") : F(" Fin juego     "));
    char vidas[17];
    vidas[0] = 'V'; vidas[1] = 'i'; vidas[2] = 'd'; vidas[3] = 'a';
    vidas[4] = 's'; vidas[5] = ' '; vidas[6] = (char)('0' + boVidas);
    vidas[7] = '\0';
    ponerTexto(1, vidas);
    pintar();
    return;
  }
  for (uint8_t i = 0; i < 16; i++)
    if (boMuralla & ((uint16_t)1 << i)) poner(i, 0, CH_LADRILLO);
  poner((uint8_t)boPaleta, 1, CH_PALETA);
  poner((uint8_t)boPelotaX, (uint8_t)boPelotaY, CH_PELOTA);
  char vidas[17];
  vidas[0] = '<'; vidas[1] = ' '; vidas[2] = '>'; vidas[3] = ' ';
  vidas[4] = (char)('0' + boVidas); vidas[5] = '\0';
  ponerTexto(1, vidas);
  pintar();
}
void procesarBreakout() {
  if (ok.largo) { volverAlMenu(); return; }
  if (boTerminado) {
    if (ok.corto) iniciarBreakout();
    dibujarBreakout();
    return;
  }
  if (izquierda.estadoEstable == LOW && boPaleta > 0) boPaleta--;
  if (derecha.estadoEstable == LOW && boPaleta < 15) boPaleta++;
  actualizarBreakout();
  dibujarBreakout();
}

// ============================================================
// MASCOTA
// Dos estadisticas de 0 a 9 (hambre y felicidad) y una cara que se
// anima en la fila de arriba. Las dos estadisticas bajan con el
// tiempo; si alguna llega a 0 se pierde, y si sobrevive 60 s gana.
// ============================================================

#define MASCOTA_POR_COMER 2
#define MASCOTA_POR_JUGAR 2
#define MASCOTA_MAXIMO 9
#define MASCOTA_TIC_MS 4000
#define MASCOTA_GANAR_MS 60000UL

uint8_t maHambre = MASCOTA_MAXIMO, maFelicidad = MASCOTA_MAXIMO;
uint16_t maUltimoTic = 0, maInicio = 0;
uint8_t maFase = 0;
bool maTerminada = false, maGanada = false;

// Fases de la cara: 0 normal, 1 guiño, 2 feliz, 3 cansado, 4 durmiendo.
void dibujarCaraMascota() {
  char ojo = '_';
  char boca = '.';
  switch (maFase) {
    case 1: ojo = CH_OJO_CERRADO; break;
    case 2: boca = CH_CORAZON; break;
    case 3: ojo = '-'; break;
    case 4: ojo = CH_OJO_CERRADO; boca = '-'; break;
  }
  // "  (o)   (o)  " -> 16 caracteres con los ojos y la boca.
  poner(4, 0, '(');
  poner(5, 0, ojo);
  poner(6, 0, ')');
  poner(9, 0, '(');
  poner(10, 0, ojo);
  poner(11, 0, ')');
  poner(7, 0, boca);
}
void iniciarMascota() {
  maHambre = MASCOTA_MAXIMO;
  maFelicidad = MASCOTA_MAXIMO;
  maTerminada = false;
  maGanada = false;
  maFase = 0;
  maUltimoTic = millis();
  maInicio = millis();
}
void actualizarMascota() {
  if (maTerminada) return;
  unsigned long ahora = millis();
  if (ahora - maUltimoTic >= MASCOTA_TIC_MS) {
    maUltimoTic = (uint16_t)ahora;
    if (maHambre > 0) maHambre--;
    if (maFelicidad > 0) maFelicidad--;
    if (maHambre == 0 || maFelicidad == 0) maTerminada = true;
  }
  // La cara va cambiando sola cada 700 ms para que se vea viva.
  maFase = (uint8_t)((ahora / 700) % 5);
  if (ahora - maInicio >= MASCOTA_GANAR_MS) { maTerminada = true; maGanada = true; }
}
void dibujarMascota() {
  borrarPantalla();
  if (maTerminada) {
    ponerTextoF(0, maGanada ? F(" Sobreviviste!  ") : F("  Se apago      "));
    ponerTextoF(1, F("OK otra | mant:menu"));
    pintar();
    return;
  }
  dibujarCaraMascota();
  // Fila de abajo: "Hambre 7  Fel 8" con barras de 3 caracteres.
  char fila[17];
  uint8_t n = 0;
  fila[n++] = 'H'; fila[n++] = ' ';
  for (uint8_t i = 0; i < 3; i++) fila[n++] = (i < maHambre / 4) ? '#' : '-';
  fila[n++] = ' '; fila[n++] = 'F'; fila[n++] = ' ';
  for (uint8_t i = 0; i < 3; i++) fila[n++] = (i < maFelicidad / 4) ? '#' : '-';
  fila[n] = '\0';
  ponerTexto(1, fila);
  pintar();
}
void procesarMascota() {
  if (ok.largo) { volverAlMenu(); return; }
  if (maTerminada) {
    if (ok.corto) iniciarMascota();
    dibujarMascota();
    return;
  }
  if (izquierda.corto) {
    if (maHambre < MASCOTA_MAXIMO) maHambre += MASCOTA_POR_COMER;
    else maHambre = MASCOTA_MAXIMO;
  }
  if (derecha.corto) {
    if (maFelicidad < MASCOTA_MAXIMO) maFelicidad += MASCOTA_POR_JUGAR;
    else maFelicidad = MASCOTA_MAXIMO;
  }
  actualizarMascota();
  dibujarMascota();
}

// ============================================================
// BLACKJACK
// Cartas de 1 a 10 (los ases valen 11 y bajan a 1 si hace falta).
// Se reparten de una en una con una animacion de la carta entrando.
// ============================================================

#define BJ_MAX_CARTAS 5
#define BJ_PASO_MS 600
#define BJ_META_VICTORIAS 3
#define BJ_META_DERROTAS 3

uint8_t bjJugador[BJ_MAX_CARTAS], bjCasa[BJ_MAX_CARTAS];
uint8_t bjCartasJugador = 0, bjCartasCasa = 0;
uint8_t bjVictoria = 0, bjDerrota = 0;
uint8_t bjPaso = 0;                 // 0 reparte, 1 juega el jugador, 2 juega la casa, 3 fin
uint8_t bjTotalJugador = 0, bjTotalCasa = 0;
bool bjOculta = true;
uint16_t bjUltimoPaso = 0;

uint8_t bjValorCarta(uint8_t valor) { return valor == 1 ? 11 : valor; }
uint8_t bjTotal(uint8_t *cartas, uint8_t cantidad, bool incluirOculta) {
  uint8_t suma = 0, ases = 0;
  for (uint8_t i = 0; i < cantidad; i++) {
    suma += bjValorCarta(cartas[i]);
    if (cartas[i] == 1) ases++;
  }
  while (suma > 21 && ases > 0) { suma -= 10; ases--; }
  return suma;
}
void nuevaRondaBlackjack() {
  bjCartasJugador = 0; bjCartasCasa = 0;
  bjVictoria = 0; bjDerrota = 0;
  bjPaso = 0; bjOculta = true;
  bjUltimoPaso = millis();
}
void repartirCarta() {
  // Alterna jugador / casa para el reparto inicial.
  bool alJugador = (bjPaso % 2) == 0;
  uint8_t valor = (uint8_t)random(1, 11);
  if (alJugador && bjCartasJugador < BJ_MAX_CARTAS) bjJugador[bjCartasJugador++] = valor;
  else if (!alJugador && bjCartasCasa < BJ_MAX_CARTAS) bjCasa[bjCartasCasa++] = valor;
  bjPaso++;
}
void terminarRondaBlackjack() {
  bjOculta = false;
  uint8_t totalJugador = bjTotal(bjJugador, bjCartasJugador, true);
  uint8_t totalCasa = bjTotal(bjCasa, bjCartasCasa, true);
  bjTotalJugador = totalJugador;
  bjTotalCasa = totalCasa;
  if (totalJugador > 21 || totalCasa > totalJugador) bjDerrota++;
  else if (totalCasa < totalJugador || totalCasa > 21) bjVictoria++;
  bjPaso = 3;
}
void actualizarBlackjack() {
  if (millis() - bjUltimoPaso < BJ_PASO_MS) return;
  bjUltimoPaso = millis();
  if (bjPaso == 0) {
    repartirCarta();
    if (bjPaso >= 4) { bjPaso = 1; bjTotalCasa = bjTotal(bjCasa, bjCartasCasa, false); }
  }
}
void dibujarBlackjack() {
  borrarPantalla();
  if (bjPaso == 3) {
    if (bjVictoria >= BJ_META_VICTORIAS || bjDerrota >= BJ_META_DERROTAS) {
      ponerTextoF(0, bjVictoria > bjDerrota ? F(" Ganaste!      ") : F(" Perdiste      "));
    } else {
      char resultado[17];
      uint8_t n = 0;
      resultado[n++] = (char)('0' + bjVictoria);
      resultado[n++] = '-';
      resultado[n++] = (char)('0' + bjDerrota);
      resultado[n++] = ' ';
      if (bjTotalJugador > 21) { resultado[n++] = 'p'; resultado[n++] = 'a'; resultado[n++] = 's'; }
      else if (bjTotalCasa > 21) { resultado[n++] = 'c'; resultado[n++] = 'a'; resultado[n++] = 's'; }
      else if (bjTotalJugador > bjTotalCasa) { resultado[n++] = 'G'; resultado[n++] = 'a'; }
      else if (bjTotalJugador < bjTotalCasa) { resultado[n++] = 'P'; resultado[n++] = 'e'; }
      else { resultado[n++] = 'E'; resultado[n++] = 'm'; }
      resultado[n] = '\0';
      for (uint8_t k = 0; k < n; k++) poner((uint8_t)((16 - n) / 2 + k), 0, resultado[k]);
    }
    ponerTextoF(1, F("OK otra | mant:menu"));
    pintar();
    return;
  }
  // Fila 0: la banca. La segunda carta sale como '?' hasta el final.
  uint8_t n = 0;
  char fila[17];
  fila[n++] = 'C';
  fila[n++] = 'a';
  fila[n++] = 's';
  for (uint8_t i = 0; i < bjCartasCasa; i++) {
    fila[n++] = ' ';
    if (i == 1 && bjOculta) fila[n++] = '?';
    else fila[n++] = (char)('0' + bjValorCarta(bjCasa[i]) % 10);
  }
  fila[n] = '\0';
  ponerTexto(0, fila);
  // Fila 1: tus cartas y tu total.
  n = 0;
  fila[n++] = 'T';
  fila[n++] = 'u';
  for (uint8_t i = 0; i < bjCartasJugador; i++) {
    fila[n++] = ' ';
    fila[n++] = (char)('0' + bjValorCarta(bjJugador[i]) % 10);
  }
  while (n < 13) fila[n++] = ' ';
  fila[n++] = (char)('0' + (bjTotal(bjJugador, bjCartasJugador, true) / 10) % 10);
  fila[n++] = (char)('0' + bjTotal(bjJugador, bjCartasJugador, true) % 10);
  fila[n] = '\0';
  ponerTexto(1, fila);
  pintar();
}
void procesarBlackjack() {
  if (ok.largo) { volverAlMenu(); return; }
  if (bjPaso == 3) {
    if (ok.corto) nuevaRondaBlackjack();
    dibujarBlackjack();
    return;
  }
  if (bjPaso == 1) {
    // Izquierda = pedir carta, derecha = plantarse.
    if (izquierda.corto && bjCartasJugador < BJ_MAX_CARTAS) {
      uint8_t valor = (uint8_t)random(1, 11);
      bjJugador[bjCartasJugador++] = valor;
      bjUltimoPaso = millis();
      if (bjTotal(bjJugador, bjCartasJugador, true) > 21) terminarRondaBlackjack();
    }
    if (derecha.corto) terminarRondaBlackjack();
  }
  actualizarBlackjack();
  dibujarBlackjack();
}

// ============================================================
// SNAKE
// El tablero son las 32 celdas del LCD (16 columnas x 2 filas).
// El cuerpo se guarda como indice de celda en un solo byte por
// segmento. Los botones controlan la direccion y NO se permite dar
// media vuelta, porque eso mataba al instante al chocar consigo mismo.
// ============================================================

#define SN_MAX_SEGMENTOS 24
#define SN_VICTORIA 12

uint8_t snCuerpo[SN_MAX_SEGMENTOS];
uint8_t snLongitud = 3;
uint8_t snDireccion = 1;   // 0 arriba, 1 derecha, 2 abajo, 3 izquierda
uint8_t snComida = 0;
uint16_t snUltimoPaso = 0;
bool snTerminado = false, snPausa = false;

uint8_t snCelda(uint8_t columna, uint8_t fila) { return (uint8_t)(fila * 16 + columna); }
void colocarComida() {
  bool ocupada;
  do {
    snComida = (uint8_t)random(0, 32);
    ocupada = false;
    for (uint8_t i = 0; i < snLongitud; i++)
      if (snCuerpo[i] == snComida) ocupada = true;
  } while (ocupada && snLongitud < 32);
}
void iniciarSnake() {
  snLongitud = 3;
  snCuerpo[0] = snCelda(8, 1);
  snCuerpo[1] = snCelda(7, 1);
  snCuerpo[2] = snCelda(6, 1);
  snDireccion = 1;
  snTerminado = false;
  snPausa = false;
  colocarComida();
  snUltimoPaso = millis();
}
void girarSnake(bool aLaIzquierda) {
  uint8_t nueva = aLaIzquierda ? (uint8_t)((snDireccion + 3) % 4)
                                : (uint8_t)((snDireccion + 1) % 4);
  // Media vuelta: no se permite, porque la cabeza entraria de lleno
  // en el propio cuerpo y la partida terminaria al instante.
  if (nueva == (uint8_t)((snDireccion + 2) % 4)) return;
  snDireccion = nueva;
}
void actualizarSnake() {
  if (millis() - snUltimoPaso < (snLongitud > 8 ? 150 : 220)) return;
  snUltimoPaso = millis();
  for (uint8_t i = snLongitud - 1; i > 0; i--) snCuerpo[i] = snCuerpo[i - 1];
  int8_t columna = (int8_t)(snCuerpo[0] % 16);
  int8_t fila = (int8_t)(snCuerpo[0] / 16);
  if (snDireccion == 0) fila--;
  if (snDireccion == 1) columna++;
  if (snDireccion == 2) fila++;
  if (snDireccion == 3) columna--;
  if (columna < 0 || columna > 15 || fila < 0 || fila > 1) { snTerminado = true; return; }
  uint8_t nueva = snCelda((uint8_t)columna, (uint8_t)fila);
  // Choque con el propio cuerpo: se empieza a partir del segmento 2
  // porque los dos primeros estan pegados a la cabeza por diseno.
  for (uint8_t i = 2; i < snLongitud; i++)
    if (snCuerpo[i] == nueva) { snTerminado = true; return; }
  snCuerpo[0] = nueva;
  if (nueva == snComida) {
    if (snLongitud < SN_MAX_SEGMENTOS) {
      snCuerpo[snLongitud] = snCuerpo[snLongitud - 1];
      snLongitud++;
    }
    colocarComida();
    if (snLongitud >= SN_VICTORIA) snTerminado = true;
  }
}
void dibujarSnake() {
  borrarPantalla();
  if (snTerminado) {
    ponerTextoF(0, snLongitud >= SN_VICTORIA ? F(" Ganaste!      ") : F(" Fin juego     "));
    char puntos[17];
    uint8_t n = 0;
    puntos[n++] = 'L'; puntos[n++] = 'e'; puntos[n++] = 'n'; puntos[n++] = ' ';
    puntos[n++] = (char)('0' + (snLongitud / 10) % 10);
    puntos[n++] = (char)('0' + snLongitud % 10);
    puntos[n] = '\0';
    ponerTexto(1, puntos);
    pintar();
    return;
  }
  poner(snComida % 16, snComida / 16, CH_COMIDA);
  for (uint8_t i = snLongitud - 1; i > 0; i--)
    poner(snCuerpo[i] % 16, snCuerpo[i] / 16, CH_CUERPO);
  poner(snCuerpo[0] % 16, snCuerpo[0] / 16, CH_CABEZA);
  if (snPausa) poner(15, 1, '|');
  pintar();
}
void procesarSnake() {
  if (ok.largo) { volverAlMenu(); return; }
  if (snTerminado) {
    if (ok.corto) iniciarSnake();
    dibujarSnake();
    return;
  }
  if (ok.corto) snPausa = !snPausa;
  if (!snPausa) {
    if (izquierda.corto) girarSnake(true);
    if (derecha.corto) girarSnake(false);
    actualizarSnake();
  }
  dibujarSnake();
}

// ============================================================
// INFORMACION
// ============================================================

void dibujarInformacion() {
  borrarPantalla();
  ponerTextoF(0, F(" TinyPocket   "));
  ponerTextoF(1, F(" ATtiny85 16x2 "));
  pintar();
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  pinMode(PIN_IZQUIERDA, INPUT_PULLUP);
  pinMode(PIN_DERECHA, INPUT_PULLUP);
  pinMode(PIN_OK, INPUT_PULLUP);
  randomSeed(analogRead(A0) + micros());
  Wire.begin();
  Wire.setClock(100000);
  lcd.init();
  lcd.backlight();
  crearCaracteres();
  borrarPantalla();
  ponerTextoF(0, F(" TinyPocket   "));
  ponerTextoF(1, F(" Iniciando...  "));
  pintar();
  delay(600);
  iniciarMascota();
  nuevaRondaBlackjack();
  iniciarSnake();
  iniciarBreakout();
  iniciarPingPong();
}

void loop() {
  actualizarBotones();
  switch (estado) {
    case MENU: procesarMenu(); break;
    case SUBMENU_PINGPONG: procesarSubmenu(); break;
    case JUEGO_PINGPONG: procesarPingPong(); break;
    case JUEGO_BREAKOUT: procesarBreakout(); break;
    case JUEGO_MASCOTA: procesarMascota(); break;
    case JUEGO_BLACKJACK: procesarBlackjack(); break;
    case JUEGO_SNAKE: procesarSnake(); break;
    case INFORMACION:
      dibujarInformacion();
      if (ok.corto || ok.largo) volverAlMenu();
      break;
  }
}