#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>

// ============================================================
// TinyPocket - version Arduino Uno (ATmega328P)
//
// Hardware (segun Arduino Uno/Img/circuitoDiagrama.png):
//   OLED 128x64 I2C : SDA -> A4, SCL -> A5 (pines I2C hardware)
//   Botones         : izquierda -> D2, derecha -> D3, OK -> D4
//                     (cada boton entre su pin y GND)
//   INPUT_PULLUP    : presionado = LOW
//   Alimentacion    : switch + bateria al pin BAT-5V
//
// ADAPTACIONES RESPECTO DE LA VERSION MEGA (obligatorias por hardware):
//   El ATmega328P tiene 2 KB de SRAM y 32 KB de flash, muy por debajo
//   de los 8 KB / 256 KB del Mega. Por eso:
//   1. La pantalla usa el buffer de UNA PAGINA de U8g2 (128 bytes en
//      vez de 1024). El dibujo es identico; U8g2 solo vuelca la
//      pantalla por paginas.
//   2. Solo se usan dos fuentes (6x10 y 4x6) para ahorrar flash.
//   3. El mazo del blackjack se guarda con un byte por carta en vez de
//      una estructura de dos bytes.
//   4. Las coordenadas de la serpiente y de las cartas animadas son
//      enteras de 16 bits en vez de float.
//   5. Los palos de las cartas se dibujan con un solo caracter en vez
//      de las cuatro figuras vectoriales.
//   6. Menos puntos de estela y de segmentos en la serpiente.
// No se pierde ninguna funcion: menu, Ping Pong, Breakout, Mascota,
// Blackjack, Snake e Informacion estan todos presentes.
//
// La logica de control con los botones NO se toca: izquierda /
// derecha / OK corto / OK largo se comportan exactamente igual.
// ============================================================

// _1_ = buffer de una pagina (128 bytes). Imprescindible en el Uno.
// Si se cambia la placa por una con mas RAM, se puede volver a _F_
// (buffer completo) sin tocar nada mas.
U8G2_SH1106_128X64_NONAME_F_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306, usa esta linea en lugar de la anterior:
// U8G2_SSD1306_128X64_NONAME_F_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// A4 y A5 son los pines I2C por defecto del Uno, por eso la variante
// _HW_I2C funciona sin indicar la SDA ni la SCL.

const byte PIN_IZQUIERDA = 2, PIN_DERECHA = 3, PIN_OK = 4;

// ============================================================
// TIPOS
// Todos los tipos van aqui arriba, antes de cualquier funcion.
// Es necesario porque el Arduino IDE mete los prototipos que genera
// solo despues de los #include: si un enum aparece mas abajo, el
// IDE genera prototipos rotos y la compilacion falla con
// "'FaseMascota' was not declared in this scope".
// ============================================================

// Los mismos tipos que en la version Mega, pero con las coordenadas
// de las cartas animadas como enteros de 16 bits: en el Uno cada byte
// cuenta y el mazo completo se guarda aparte como un solo byte por
// carta (ver bjCartas), lo que ahorra 52 bytes de SRAM.
struct Punto { float x; float y; };

struct Carta { byte palo; byte valor; };

struct CartaAnimada {
  Carta carta;
  int16_t x;
  int16_t y;
  int16_t destinoX;
  int16_t destinoY;
  bool jugador;
  bool oculta;
};

struct Boton {
  byte pin; bool lecturaAnterior; bool estadoEstable;
  unsigned long cambioLectura, inicioPulsacion; bool corto, largo;
};

// Estados de la aplicacion.
enum Estado { MENU, SUBMENU_PINGPONG, JUEGO_PINGPONG, JUEGO_BREAKOUT,
              JUEGO_MASCOTA, JUEGO_BLACKJACK, JUEGO_SNAKE, INFORMACION };

// Fases de la animacion de la mascota.
enum FaseMascota {
  MASCOTA_NORMAL,
  MASCOTA_MIRA_IZQ,
  MASCOTA_PAUSA_IZQ,
  MASCOTA_VUELVE_CENTRO_1,
  MASCOTA_PAUSA_CENTRO_1,
  MASCOTA_MIRA_DER,
  MASCOTA_PAUSA_DER,
  MASCOTA_VUELVE_CENTRO_2,
  MASCOTA_PAUSA_CENTRO_2,
  MASCOTA_PARPADEO_CIERRA,
  MASCOTA_PARPADEO_ESPERA,
  MASCOTA_PARPADEO_ABRE,
  MASCOTA_PARPADEO_PAUSA,
  MASCOTA_FELIZ,
  MASCOTA_FELIZ_ESPERA,
  MASCOTA_FELIZ_NORMAL,
  MASCOTA_FELIZ_PAUSA,
  MASCOTA_SORPRESA,
  MASCOTA_SORPRESA_REBOTE,
  MASCOTA_SORPRESA_ESPERA,
  MASCOTA_SORPRESA_NORMAL,
  MASCOTA_SORPRESA_PAUSA,
  MASCOTA_GUINO_CIERRA,
  MASCOTA_GUINO_ESPERA,
  MASCOTA_GUINO_ABRE,
  MASCOTA_GUINO_PAUSA,
  MASCOTA_MIRA_POCO_IZQ,
  MASCOTA_MIRA_POCO_PAUSA,
  MASCOTA_MIRA_POCO_CENTRO,
  MASCOTA_MIRA_POCO_PAUSA_2,
  MASCOTA_DORMIR_CIERRA,
  MASCOTA_DORMIR,
  MASCOTA_DESPERTAR,
  MASCOTA_DESPERTAR_PAUSA,
  MASCOTA_DESPERTAR_PARPADEO_CIERRA,
  MASCOTA_DESPERTAR_PARPADEO_ESPERA,
  MASCOTA_DESPERTAR_PARPADEO_ABRE
};

// Estados internos del blackjack.
enum EstadoJuegoBJ { BJ_REPARTO_INICIAL, BJ_JUGADOR, BJ_REVELAR_CASA,
                      BJ_CASA, BJ_ESPERANDO_RESULTADO, BJ_RESULTADO };
enum PasoInicialBJ { BJ_INICIO_JUGADOR_1, BJ_INICIO_JUGADOR_2,
                     BJ_INICIO_CASA_VISIBLE, BJ_INICIO_CASA_OCULTA, BJ_INICIO_TERMINADO };

// ============================================================
// PROTOTIPOS
// Declarados a mano para que el IDE no tenga que generarlos.
// ============================================================

// Botones y utilidades de pantalla.
void actualizarBoton(Boton &boton);
void actualizarBotones();
void volverAlMenu();
void textoCentrado(const char *texto, int y, const uint8_t *fuente);
void mostrarMensaje(const char *linea1, const char *linea2);
void dibujarCabecera(const char *titulo);
void llenarTriangulo(int x0, int y0, int x1, int y1, int x2, int y2);

// Menu.
void dibujarMenu();
void procesarMenu();
void dibujarSubmenuPingPong();
void procesarSubmenuPingPong();

// Ping Pong.
void iniciarPingPong();
void ppInicializarTrail();
void ppActualizarTrail();
void ppReiniciarPelota();
void ppComprobarParedesLaterales();
void ppComprobarParedSuperior();
void ppComprobarPaleta();
void ppActualizarPelota();
void dibujarPingPong();
void procesarPingPong();

// Breakout.
void iniciarBreakout();
void boInicializarBloques();
void boInicializarTrail();
void boActualizarTrail();
void boReiniciarPelota();
void boComprobarParedesLaterales();
void boComprobarParedSuperior();
void boComprobarPaleta();
void boComprobarBloques();
void boActualizarPelota();
void dibujarBreakout();
void procesarBreakout();

// Mascota.
float mascotaSuave(float t);
unsigned long mascotaDuracionFase(FaseMascota fase);
void mascotaCambiarFase(FaseMascota nuevaFase);
void mascotaSiguienteFase();
void iniciarMascota();
void mascotaDibujarOjo(float x, float y, float ancho, float alto);
void mascotaDibujarOjoFeliz(float x, float y);
void mascotaOjosCerrados();
void mascotaDibujarDosOjos(float desplazamiento, float ancho, float alto, float y);
void mascotaDibujarGuino(float altoIzquierdo);
void limitarStats();
void dibujarBarraStat(int x, int y, int valor);
void dibujarOjosMascota();
void actualizarFaseMascota();
void dibujarMascota();
void procesarMascota();

// Blackjack.
void bjCrearMazo();
void bjMezclarMazo();
byte bjSacarCarta();
int bjValorCarta(Carta carta);
int bjCalcularPuntaje(bool jugador, bool incluirOculta);
int bjPuntajeJugador();
int bjPuntajeCasaVisible();
int bjPuntajeCasaReal();
int bjAnchoGrupo(int cantidad);
void bjActualizarPosicionesCartas();
void bjAgregarCarta(bool jugador, bool oculta);
void bjRevelarCartaCasa();
void nuevaRondaBJ();
void bjActualizarAnimaciones();
void bjDibujarCorazon(int x, int y);
void bjDibujarDiamante(int x, int y);
void bjDibujarTrebol(int x, int y);
void bjDibujarPicas(int x, int y);
void bjDibujarValor(int x, int y, byte valor);
void bjDibujarCarta(CartaAnimada &carta);
void bjRepartoInicial();
void bjTurnoCasa();
void bjActualizarJuego();
void bjDibujarJuego();
void bjDibujarResultado();
void procesarBlackjack();

// Snake.
float snDistancia(float x1, float y1, float x2, float y2);
void snGenerarComida();
void iniciarSnake();
void snActualizarSerpiente();
void snDibujarSerpiente();
void dibujarSnake();
void snGirar(bool izquierdaGiro);
void procesarSnake();

// Informacion.
void dibujarInformacion();

// ============================================================
// ESTADOS DE LA APLICACION
// ============================================================

Estado estado = MENU;

const char *opcionesMenu[] = {"Ping Pong", "Mascota", "Blackjack", "Snake", "Informacion"};
const byte CANT_OPCIONES = 5;
byte seleccionMenu = 0, seleccionPingPong = 0;

// ============================================================
// BOTONES
// ============================================================

Boton izquierda = {PIN_IZQUIERDA, HIGH, HIGH, 0, 0, false, false};
Boton derecha   = {PIN_DERECHA, HIGH, HIGH, 0, 0, false, false};
Boton ok        = {PIN_OK, HIGH, HIGH, 0, 0, false, false};

void actualizarBoton(Boton &boton) {
  boton.corto = false; boton.largo = false;
  bool lectura = digitalRead(boton.pin); unsigned long ahora = millis();
  if (lectura != boton.lecturaAnterior) { boton.lecturaAnterior = lectura; boton.cambioLectura = ahora; }
  if (ahora - boton.cambioLectura >= 25 && lectura != boton.estadoEstable) {
    boton.estadoEstable = lectura;
    if (boton.estadoEstable == LOW) boton.inicioPulsacion = ahora;
    else if (ahora - boton.inicioPulsacion >= 800) boton.largo = true;
    else boton.corto = true;
  }
}
void actualizarBotones() { actualizarBoton(izquierda); actualizarBoton(derecha); actualizarBoton(ok); }
void volverAlMenu() { estado = MENU; }

// ============================================================
// UTILIDADES DE PANTALLA
// ============================================================

void textoCentrado(const char *texto, int y, const uint8_t *fuente) {
  pantalla.setFont(fuente);
  pantalla.drawStr((128 - pantalla.getStrWidth(texto)) / 2, y, texto);
}
void mostrarMensaje(const char *linea1, const char *linea2) {
  pantalla.clearBuffer(); pantalla.setFont(u8g2_font_6x10_tf);
  pantalla.drawStr((128 - pantalla.getStrWidth(linea1))/2, 27, linea1);
  pantalla.drawStr((128 - pantalla.getStrWidth(linea2))/2, 43, linea2);
  pantalla.sendBuffer();
}
void dibujarCabecera(const char *titulo) {
  pantalla.setFont(u8g2_font_6x10_tf); pantalla.drawStr(0, 9, titulo); pantalla.drawHLine(0, 12, 128);
}
// Triangulo relleno. Se usa para los palos de la baraja, porque
// U8g2 solo tiene el triangulo de contorno (drawTriangle).
void llenarTriangulo(int x0, int y0, int x1, int y1, int x2, int y2) {
  int xs[3]; int ys[3];
  xs[0] = x0; ys[0] = y0;
  xs[1] = x1; ys[1] = y1;
  xs[2] = x2; ys[2] = y2;
  int yMin = min(min(y0, y1), y2);
  int yMax = max(max(y0, y1), y2);
  for (int y = yMin; y <= yMax; y++) {
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

// ============================================================
// MENU CON MARCADOR ANIMADO
// ============================================================

float marcadorMenuY = 15.0;
unsigned long ultimoFrameMenu = 0;
void dibujarMenu() {
  unsigned long ahora = millis();
  if (ahora - ultimoFrameMenu >= 16) {
    ultimoFrameMenu = ahora;
    float destino = 15 + seleccionMenu * 9;
    marcadorMenuY += (destino - marcadorMenuY) * 0.38;
    if (fabs(destino - marcadorMenuY) < 0.25) marcadorMenuY = destino;
  }
  pantalla.clearBuffer(); pantalla.setFont(u8g2_font_6x10_tf);
  pantalla.drawStr(0, 9, "tinyPocket"); pantalla.drawHLine(0, 12, 128);
  pantalla.drawRBox(0, (int)marcadorMenuY, 128, 9, 2);
  for (byte i=0; i<CANT_OPCIONES; i++) {
    byte y = 23 + i*9;
    if (i == seleccionMenu) pantalla.setDrawColor(0);
    pantalla.drawStr(4, y, opcionesMenu[i]); pantalla.setDrawColor(1);
  }
  pantalla.sendBuffer();
}
void procesarMenu() {
  if (izquierda.corto) seleccionMenu = (seleccionMenu == 0) ? CANT_OPCIONES-1 : seleccionMenu-1;
  if (derecha.corto) seleccionMenu = (seleccionMenu+1) % CANT_OPCIONES;
  if (ok.corto) {
    if (seleccionMenu == 0) { seleccionPingPong=0; estado=SUBMENU_PINGPONG; }
    else if (seleccionMenu == 1) estado=JUEGO_MASCOTA;
    else if (seleccionMenu == 2) estado=JUEGO_BLACKJACK;
    else if (seleccionMenu == 3) estado=JUEGO_SNAKE;
    else estado=INFORMACION;
  }
  dibujarMenu();
}
void dibujarSubmenuPingPong() {
  pantalla.clearBuffer(); dibujarCabecera("Ping Pong");
  const char *modos[] = {"Normal", "Breakout"};
  for (byte i=0; i<2; i++) {
    int y=30+i*16;
    if (i==seleccionPingPong) { pantalla.drawRBox(0,y-10,128,13,2); pantalla.setDrawColor(0); }
    pantalla.drawStr(5,y,modos[i]); pantalla.setDrawColor(1);
  }
  pantalla.setFont(u8g2_font_4x6_tf); pantalla.drawStr(4,63,"OK elige | mant: menu"); pantalla.sendBuffer();
}
void procesarSubmenuPingPong() {
  if (izquierda.corto || derecha.corto) seleccionPingPong=1-seleccionPingPong;
  if (ok.largo) estado=MENU;
  else if (ok.corto) { if (seleccionPingPong==0) { estado=JUEGO_PINGPONG; iniciarPingPong(); }
                       else { estado=JUEGO_BREAKOUT; iniciarBreakout(); } }
  dibujarSubmenuPingPong();
}

// ============================================================
// PING PONG
// Animacion tomada de src/examples/ejemploPingPong.ino
// Solo cambia el control de la paleta: los botones mueven la
// paleta en lugar del movimiento automatico con sin() del ejemplo.
// El campo de juego se achico para que quepa entre el marcador
// de arriba y la linea de controles de abajo.
// ============================================================

const float PP_PADDLE_WIDTH = 24.0;
const float PP_PADDLE_HEIGHT = 3.0;
const float PP_PADDLE_Y = 52.0;
const float PP_BALL_RADIUS = 2.5;
const int PP_TRAIL_LENGTH = 6;
const float PP_TOPE = 12.0;
const float PP_LIMITE_INFERIOR = 57.0;
const unsigned long PP_FRAME_TIME = 16;

float ppPaletaX = 64.0;
float ppPelotaX = 64.0;
float ppPelotaY = 40.0;
float ppVelocidadX = 0.85;
float ppVelocidadY = -1.25;
float ppReboteAnimacion = 0.0;
Punto ppTrail[PP_TRAIL_LENGTH];
byte ppPuntos = 0;
unsigned long ppUltimoFrame = 0;

void ppInicializarTrail() {
  for (int i = 0; i < PP_TRAIL_LENGTH; i++) { ppTrail[i].x = ppPelotaX; ppTrail[i].y = ppPelotaY; }
}
void ppActualizarTrail() {
  for (int i = PP_TRAIL_LENGTH - 1; i > 0; i--) ppTrail[i] = ppTrail[i - 1];
  ppTrail[0].x = ppPelotaX; ppTrail[0].y = ppPelotaY;
}
void ppReiniciarPelota() {
  ppPelotaX = ppPaletaX; ppPelotaY = PP_PADDLE_Y - 7;
  ppVelocidadX = random(0, 2) ? 0.85 : -0.85; ppVelocidadY = -1.25;
  ppInicializarTrail();
}
void iniciarPingPong() {
  ppPaletaX = 64; ppPelotaX = 64; ppPelotaY = 40;
  ppVelocidadX = random(0, 2) ? 0.85 : -0.85; ppVelocidadY = -1.25;
  ppReboteAnimacion = 0; ppPuntos = 0; ppUltimoFrame = 0;
  ppInicializarTrail();
}
void ppComprobarParedesLaterales() {
  if (ppPelotaX - PP_BALL_RADIUS <= 1) {
    ppPelotaX = 1 + PP_BALL_RADIUS; ppVelocidadX = fabs(ppVelocidadX); ppReboteAnimacion = 1.0;
  }
  if (ppPelotaX + PP_BALL_RADIUS >= 127) {
    ppPelotaX = 127 - PP_BALL_RADIUS; ppVelocidadX = -fabs(ppVelocidadX); ppReboteAnimacion = 1.0;
  }
}
void ppComprobarParedSuperior() {
  if (ppPelotaY - PP_BALL_RADIUS <= PP_TOPE) {
    ppPelotaY = PP_TOPE + PP_BALL_RADIUS; ppVelocidadY = fabs(ppVelocidadY); ppReboteAnimacion = 1.0;
  }
}
void ppComprobarPaleta() {
  float izquierda = ppPaletaX - PP_PADDLE_WIDTH / 2;
  float derecha = ppPaletaX + PP_PADDLE_WIDTH / 2;
  float arriba = PP_PADDLE_Y;
  float abajo = PP_PADDLE_Y + PP_PADDLE_HEIGHT;
  bool tocaPaleta = ppPelotaX + PP_BALL_RADIUS >= izquierda &&
                    ppPelotaX - PP_BALL_RADIUS <= derecha &&
                    ppPelotaY + PP_BALL_RADIUS >= arriba &&
                    ppPelotaY - PP_BALL_RADIUS <= abajo;
  if (tocaPaleta && ppVelocidadY > 0) {
    ppPelotaY = arriba - PP_BALL_RADIUS - 0.5;
    ppVelocidadY = -fabs(ppVelocidadY);
    float diferencia = ppPelotaX - ppPaletaX;
    ppVelocidadX = diferencia * 0.08;
    if (fabs(ppVelocidadX) < 0.35) {
      if (ppVelocidadX >= 0) ppVelocidadX = 0.35;
      else ppVelocidadX = -0.35;
    }
    ppReboteAnimacion = 1.0;
    ppPuntos++;
  }
}
void ppActualizarPelota() {
  ppActualizarTrail();
  ppPelotaX += ppVelocidadX;
  ppPelotaY += ppVelocidadY;
  ppComprobarParedesLaterales();
  ppComprobarParedSuperior();
  ppComprobarPaleta();
  if (ppReboteAnimacion > 0) {
    ppReboteAnimacion -= 0.08;
    if (ppReboteAnimacion < 0) ppReboteAnimacion = 0;
  }
  if (ppPelotaY > PP_LIMITE_INFERIOR) ppReiniciarPelota();
}
void dibujarPingPong() {
  pantalla.clearBuffer();
  // Solo el marcador: el nombre del juego sobra mientras se juega.
  pantalla.setFont(u8g2_font_4x6_tf);
  char buf[10]; sprintf(buf, "Pts %d", ppPuntos);
  pantalla.drawStr(2, 6, buf);
  int alto = (int)(PP_LIMITE_INFERIOR - PP_TOPE + 1);
  pantalla.drawVLine(0, (int)PP_TOPE, alto);
  pantalla.drawVLine(127, (int)PP_TOPE, alto);
  pantalla.drawHLine(0, (int)PP_TOPE, 128);
  for (int i = PP_TRAIL_LENGTH - 1; i >= 0; i--) {
    int radio = 1;
    if (i < 2) radio = 2;
    pantalla.drawDisc((int)ppTrail[i].x, (int)ppTrail[i].y, radio);
  }
  pantalla.drawRBox((int)(ppPaletaX - PP_PADDLE_WIDTH / 2), (int)PP_PADDLE_Y,
                    (int)PP_PADDLE_WIDTH, (int)PP_PADDLE_HEIGHT, 1);
  int radioPelota = 2;
  if (ppReboteAnimacion > 0.5) radioPelota = 3;
  pantalla.drawDisc((int)ppPelotaX, (int)ppPelotaY, radioPelota);
  pantalla.drawStr(2, 63, "< > paleta   OK: menu");
  pantalla.sendBuffer();
}
void procesarPingPong() {
  if (ok.largo) { volverAlMenu(); return; }
  if (izquierda.estadoEstable == LOW) ppPaletaX -= 1.8;
  if (derecha.estadoEstable == LOW) ppPaletaX += 1.8;
  ppPaletaX = constrain(ppPaletaX, PP_PADDLE_WIDTH / 2, 128 - PP_PADDLE_WIDTH / 2);
  unsigned long ahora = millis();
  if (ahora - ppUltimoFrame >= PP_FRAME_TIME) {
    ppUltimoFrame = ahora;
    ppActualizarPelota();
  }
  dibujarPingPong();
}

// ============================================================
// BREAKOUT
// Animacion tomada de src/examples/ejemploBreakout.ino
// Solo cambia el control de la paleta: los botones la mueven
// en lugar del movimiento automatico con sin() del ejemplo.
// Los bloques se achican (11x4 en vez de 13x5) y se recolocan
// para dejar aire al marcador y a la linea de controles.
// ============================================================

const int BO_COLUMNAS_BLOQUES = 8;
const int BO_FILAS_BLOQUES = 4;
const float BO_BLOQUE_ANCHO = 11.0;
const float BO_BLOQUE_ALTO = 4.0;
const float BO_BLOQUE_SEPARACION = 2.0;
// 8 columnas de 11 + 7 separaciones de 2 = 102 px -> centrado en 13.
const float BO_BLOQUES_X = 13.0;
// 4 filas de 4 + 3 separaciones de 2 = 22 px, de 20 a 41.
const float BO_BLOQUES_Y = 20.0;
const float BO_RADIO = 2.0;
const float BO_PALETA_ANCHO = 22.0;
const float BO_PALETA_ALTO = 3.0;
const float BO_PALETA_Y = 52.0;
const int BO_TRAIL_LENGTH = 6;
const float BO_TOPE = 12.0;
const float BO_LIMITE_INFERIOR = 57.0;
const unsigned long BO_FRAME_TIME = 16;

bool boBloques[BO_FILAS_BLOQUES][BO_COLUMNAS_BLOQUES];
int boBloquesRestantes = 0;
float boPaletaX = 64.0;
float boPelotaX = 64.0;
float boPelotaY = 46.0;
float boPelotaAnteriorX = 64.0;
float boPelotaAnteriorY = 48.0;
float boVelocidadX = 0.85;
float boVelocidadY = -1.25;
float boReboteAnimacion = 0.0;
Punto boTrail[BO_TRAIL_LENGTH];
byte boVidas = 3;
bool boFin = false, boGano = false;
unsigned long boUltimoFrame = 0;

void boInicializarBloques() {
  boBloquesRestantes = 0;
  for (int fila = 0; fila < BO_FILAS_BLOQUES; fila++)
    for (int col = 0; col < BO_COLUMNAS_BLOQUES; col++) { boBloques[fila][col] = true; boBloquesRestantes++; }
}
void boInicializarTrail() {
  for (int i = 0; i < BO_TRAIL_LENGTH; i++) { boTrail[i].x = boPelotaX; boTrail[i].y = boPelotaY; }
}
void boActualizarTrail() {
  for (int i = BO_TRAIL_LENGTH - 1; i > 0; i--) boTrail[i] = boTrail[i - 1];
  boTrail[0].x = boPelotaX; boTrail[0].y = boPelotaY;
}
void boReiniciarPelota() {
  boPelotaX = boPaletaX; boPelotaY = BO_PALETA_Y - 7;
  boPelotaAnteriorX = boPelotaX; boPelotaAnteriorY = boPelotaY;
  boVelocidadX = random(0, 2) ? 0.85 : -0.85; boVelocidadY = -1.25;
  boInicializarTrail();
}
void iniciarBreakout() {
  boInicializarBloques();
  boPaletaX = 64; boPelotaX = 64; boPelotaY = 46;
  boPelotaAnteriorX = 64; boPelotaAnteriorY = 46;
  boVelocidadX = random(0, 2) ? 0.85 : -0.85; boVelocidadY = -1.25;
  boReboteAnimacion = 0; boVidas = 3; boFin = false; boGano = false;
  boUltimoFrame = 0; boInicializarTrail();
}
void boComprobarParedesLaterales() {
  if (boPelotaX - BO_RADIO <= 1) {
    boPelotaX = 1 + BO_RADIO; boVelocidadX = fabs(boVelocidadX); boReboteAnimacion = 1.0;
  }
  if (boPelotaX + BO_RADIO >= 127) {
    boPelotaX = 127 - BO_RADIO; boVelocidadX = -fabs(boVelocidadX); boReboteAnimacion = 1.0;
  }
}
void boComprobarParedSuperior() {
  if (boPelotaY - BO_RADIO <= BO_TOPE) {
    boPelotaY = BO_TOPE + BO_RADIO; boVelocidadY = fabs(boVelocidadY); boReboteAnimacion = 1.0;
  }
}
void boComprobarPaleta() {
  float izquierda = boPaletaX - BO_PALETA_ANCHO / 2;
  float derecha = boPaletaX + BO_PALETA_ANCHO / 2;
  float arriba = BO_PALETA_Y;
  float abajo = BO_PALETA_Y + BO_PALETA_ALTO;
  bool tocaPaleta = boPelotaX + BO_RADIO >= izquierda &&
                    boPelotaX - BO_RADIO <= derecha &&
                    boPelotaY + BO_RADIO >= arriba &&
                    boPelotaY - BO_RADIO <= abajo;
  if (tocaPaleta && boVelocidadY > 0) {
    boPelotaY = arriba - BO_RADIO - 0.5;
    boVelocidadY = -fabs(boVelocidadY);
    float diferencia = boPelotaX - boPaletaX;
    boVelocidadX = diferencia * 0.07;
    if (fabs(boVelocidadX) < 0.35) {
      if (boVelocidadX < 0) boVelocidadX = -0.35; else boVelocidadX = 0.35;
    }
    boReboteAnimacion = 1.0;
  }
}
void boComprobarBloques() {
  for (int fila = 0; fila < BO_FILAS_BLOQUES; fila++) {
    for (int col = 0; col < BO_COLUMNAS_BLOQUES; col++) {
      if (!boBloques[fila][col]) continue;
      float bloqueX = BO_BLOQUES_X + col * (BO_BLOQUE_ANCHO + BO_BLOQUE_SEPARACION);
      float bloqueY = BO_BLOQUES_Y + fila * (BO_BLOQUE_ALTO + BO_BLOQUE_SEPARACION);
      float bloqueDerecha = bloqueX + BO_BLOQUE_ANCHO;
      float bloqueAbajo = bloqueY + BO_BLOQUE_ALTO;
      bool tocaHorizontal = boPelotaX + BO_RADIO >= bloqueX && boPelotaX - BO_RADIO <= bloqueDerecha;
      bool tocaVertical = boPelotaY + BO_RADIO >= bloqueY && boPelotaY - BO_RADIO <= bloqueAbajo;
      if (!(tocaHorizontal && tocaVertical)) continue;
      boBloques[fila][col] = false;
      boBloquesRestantes--;
      bool desdeArriba = boPelotaAnteriorY + BO_RADIO <= bloqueY;
      bool desdeAbajo = boPelotaAnteriorY - BO_RADIO >= bloqueAbajo;
      bool desdeIzquierda = boPelotaAnteriorX + BO_RADIO <= bloqueX;
      bool desdeDerecha = boPelotaAnteriorX - BO_RADIO >= bloqueDerecha;
      if (desdeArriba) {
        boPelotaY = bloqueY - BO_RADIO - 0.5;
        boVelocidadY = -fabs(boVelocidadY);
      } else if (desdeAbajo) {
        boPelotaY = bloqueAbajo + BO_RADIO + 0.5;
        boVelocidadY = fabs(boVelocidadY);
      } else if (desdeIzquierda) {
        boPelotaX = bloqueX - BO_RADIO - 0.5;
        boVelocidadX = -fabs(boVelocidadX);
      } else if (desdeDerecha) {
        boPelotaX = bloqueDerecha + BO_RADIO + 0.5;
        boVelocidadX = fabs(boVelocidadX);
      } else {
        float distanciaVertical = min(fabs(boPelotaY - bloqueY), fabs(boPelotaY - bloqueAbajo));
        float distanciaHorizontal = min(fabs(boPelotaX - bloqueX), fabs(boPelotaX - bloqueDerecha));
        if (distanciaVertical <= distanciaHorizontal) boVelocidadY = -boVelocidadY;
        else boVelocidadX = -boVelocidadX;
      }
      boVelocidadX *= 1.015;
      boVelocidadY *= 1.015;
      if (fabs(boVelocidadX) > 2.8) { if (boVelocidadX > 0) boVelocidadX = 2.8; else boVelocidadX = -2.8; }
      if (fabs(boVelocidadY) > 2.8) { if (boVelocidadY > 0) boVelocidadY = 2.8; else boVelocidadY = -2.8; }
      boReboteAnimacion = 1.0;
      return;
    }
  }
}
void boActualizarPelota() {
  boPelotaAnteriorX = boPelotaX;
  boPelotaAnteriorY = boPelotaY;
  boActualizarTrail();
  boPelotaX += boVelocidadX;
  boPelotaY += boVelocidadY;
  boComprobarParedesLaterales();
  boComprobarParedSuperior();
  boComprobarBloques();
  boComprobarPaleta();
  if (boReboteAnimacion > 0) {
    boReboteAnimacion -= 0.08;
    if (boReboteAnimacion < 0) boReboteAnimacion = 0;
  }
  if (boPelotaY > BO_LIMITE_INFERIOR) {
    boVidas--;
    if (boVidas == 0) { boFin = true; }
    else boReiniciarPelota();
  }
  if (boBloquesRestantes <= 0) { boFin = true; boGano = true; }
}
void dibujarBreakout() {
  pantalla.clearBuffer();
  // Solo las vidas: el nombre del juego sobra mientras se juega.
  pantalla.setFont(u8g2_font_4x6_tf);
  char buf[8]; sprintf(buf, "V %d", boVidas);
  pantalla.drawStr(126 - pantalla.getStrWidth(buf), 6, buf);
  int alto = (int)(BO_LIMITE_INFERIOR - BO_TOPE + 1);
  pantalla.drawVLine(0, (int)BO_TOPE, alto);
  pantalla.drawVLine(127, (int)BO_TOPE, alto);
  pantalla.drawHLine(0, (int)BO_TOPE, 128);
  for (int fila = 0; fila < BO_FILAS_BLOQUES; fila++) {
    for (int col = 0; col < BO_COLUMNAS_BLOQUES; col++) {
      if (!boBloques[fila][col]) continue;
      int x = (int)(BO_BLOQUES_X + col * (BO_BLOQUE_ANCHO + BO_BLOQUE_SEPARACION));
      int y = (int)(BO_BLOQUES_Y + fila * (BO_BLOQUE_ALTO + BO_BLOQUE_SEPARACION));
      pantalla.drawBox(x, y, (int)BO_BLOQUE_ANCHO, (int)BO_BLOQUE_ALTO);
    }
  }
  for (int i = BO_TRAIL_LENGTH - 1; i >= 0; i--) {
    int radio = 1;
    if (i < 2) radio = 2;
    pantalla.drawDisc((int)boTrail[i].x, (int)boTrail[i].y, radio);
  }
  pantalla.drawRBox((int)(boPaletaX - BO_PALETA_ANCHO / 2), (int)BO_PALETA_Y,
                    (int)BO_PALETA_ANCHO, (int)BO_PALETA_ALTO, 1);
  int radioPelota = 2;
  if (boReboteAnimacion > 0.5) radioPelota = 3;
  pantalla.drawDisc((int)boPelotaX, (int)boPelotaY, radioPelota);
  pantalla.setFont(u8g2_font_4x6_tf);
  if (boFin) {
    pantalla.drawRBox(16, 24, 96, 20, 2);
    pantalla.setDrawColor(0);
    textoCentrado(boGano ? "Ganaste!" : "Fin del juego", 37, u8g2_font_6x10_tf);
    pantalla.setDrawColor(1);
    pantalla.drawStr(14, 63, "OK reinicia | menu");
  } else {
    pantalla.drawStr(2, 63, "< > paleta   OK: menu");
  }
  pantalla.sendBuffer();
}
void procesarBreakout() {
  if (ok.largo) { volverAlMenu(); return; }
  if (boFin) { if (ok.corto) iniciarBreakout(); dibujarBreakout(); return; }
  if (izquierda.estadoEstable == LOW) boPaletaX -= 1.8;
  if (derecha.estadoEstable == LOW) boPaletaX += 1.8;
  boPaletaX = constrain(boPaletaX, BO_PALETA_ANCHO / 2, 128 - BO_PALETA_ANCHO / 2);
  unsigned long ahora = millis();
  if (ahora - boUltimoFrame >= BO_FRAME_TIME) {
    boUltimoFrame = ahora;
    boActualizarPelota();
  }
  dibujarBreakout();
}

// ============================================================
// MASCOTA
// Animacion de ojos tomada de src/examples/ejemploOjosMascota.ino
// El ejemplo es bloqueante (usa delay). Aqui cada paso de su
// bucle() es una FASE con duracion, y se avanza con millis(),
// de modo que los botones siguen respondiendo.
// Se conservan las acciones, las estadisticas y los botones.
// ============================================================

const int MA_CENTRO_IZQ = 43;
const int MA_CENTRO_DER = 85;
// El ejemplo centra los ojos en 32 y usa 25x31 porque los dibuja a
// pantalla completa. Aqui se bajan a 29 y se achican a 22x24 para
// que quepan entre el titulo, las barras y la linea de controles.
const int MA_CENTRO_Y = 29;
const float MA_OJO_ANCHO = 22.0;
const float MA_OJO_ALTO = 24.0;

int hambre = 25, higiene = 80, felicidad = 75, energia = 80;
byte accionMascota = 0;
const char *accionesMascota[] = {"Comer", "Banar", "Acariciar", "Dormir"};
unsigned long tiempoStatsMascota = 0, inicioSueno = 0, finAccionMascota = 0;
unsigned long ultimoCambioEstadistica = 0, maUltimoFrame = 0;
bool durmiendo = false;
byte animacionAccion = 0;
byte estadisticaVisible = 0;
FaseMascota mascotaFase = MASCOTA_NORMAL;
unsigned long mascotaInicioFase = 0;

float mascotaSuave(float t) {
  if (t < 0) t = 0;
  if (t > 1) t = 1;
  return t * t * (3.0 - 2.0 * t);
}
unsigned long mascotaDuracionFase(FaseMascota fase) {
  switch (fase) {
    case MASCOTA_NORMAL: return 800;
    case MASCOTA_MIRA_IZQ: return 600;
    case MASCOTA_PAUSA_IZQ: return 500;
    case MASCOTA_VUELVE_CENTRO_1: return 550;
    case MASCOTA_PAUSA_CENTRO_1: return 300;
    case MASCOTA_MIRA_DER: return 650;
    case MASCOTA_PAUSA_DER: return 500;
    case MASCOTA_VUELVE_CENTRO_2: return 550;
    case MASCOTA_PAUSA_CENTRO_2: return 500;
    case MASCOTA_PARPADEO_CIERRA: return 240;
    case MASCOTA_PARPADEO_ESPERA: return 90;
    case MASCOTA_PARPADEO_ABRE: return 240;
    case MASCOTA_PARPADEO_PAUSA: return 600;
    case MASCOTA_FELIZ: return 240;
    case MASCOTA_FELIZ_ESPERA: return 1000;
    case MASCOTA_FELIZ_NORMAL: return 500;
    case MASCOTA_FELIZ_PAUSA: return 400;
    case MASCOTA_SORPRESA: return 300;
    case MASCOTA_SORPRESA_REBOTE: return 210;
    case MASCOTA_SORPRESA_ESPERA: return 700;
    case MASCOTA_SORPRESA_NORMAL: return 450;
    case MASCOTA_SORPRESA_PAUSA: return 400;
    case MASCOTA_GUINO_CIERRA: return 200;
    case MASCOTA_GUINO_ESPERA: return 150;
    case MASCOTA_GUINO_ABRE: return 200;
    case MASCOTA_GUINO_PAUSA: return 500;
    case MASCOTA_MIRA_POCO_IZQ: return 400;
    case MASCOTA_MIRA_POCO_PAUSA: return 300;
    case MASCOTA_MIRA_POCO_CENTRO: return 400;
    case MASCOTA_MIRA_POCO_PAUSA_2: return 500;
    case MASCOTA_DORMIR_CIERRA: return 360;
    case MASCOTA_DORMIR: return 1200;
    case MASCOTA_DESPERTAR: return 360;
    case MASCOTA_DESPERTAR_PAUSA: return 200;
    case MASCOTA_DESPERTAR_PARPADEO_CIERRA: return 240;
    case MASCOTA_DESPERTAR_PARPADEO_ESPERA: return 90;
    case MASCOTA_DESPERTAR_PARPADEO_ABRE: return 240;
  }
  return 500;
}
void mascotaCambiarFase(FaseMascota nuevaFase) {
  mascotaFase = nuevaFase;
  mascotaInicioFase = millis();
}
void mascotaSiguienteFase() {
  int siguiente = (int)mascotaFase + 1;
  if (siguiente > (int)MASCOTA_DESPERTAR_PARPADEO_ABRE) siguiente = 0;
  mascotaCambiarFase((FaseMascota)siguiente);
}
void iniciarMascota() {
  mascotaCambiarFase(MASCOTA_NORMAL);
}
// Dibujado de los ojos, calcado de ejemploOjosMascota.ino
void mascotaDibujarOjo(float x, float y, float ancho, float alto) {
  pantalla.drawRBox((int)(x - ancho / 2), (int)(y - alto / 2), (int)ancho, (int)alto, 8);
}
void mascotaDibujarOjoFeliz(float x, float y) {
  pantalla.drawLine((int)x - 11, (int)y + 2, (int)x - 6, (int)y - 3);
  pantalla.drawLine((int)x - 6, (int)y - 3, (int)x, (int)y - 5);
  pantalla.drawLine((int)x, (int)y - 5, (int)x + 6, (int)y - 3);
  pantalla.drawLine((int)x + 6, (int)y - 3, (int)x + 11, (int)y + 2);
}
void mascotaOjosCerrados() {
  pantalla.drawLine(MA_CENTRO_IZQ - 10, MA_CENTRO_Y, MA_CENTRO_IZQ + 10, MA_CENTRO_Y);
  pantalla.drawLine(MA_CENTRO_DER - 10, MA_CENTRO_Y, MA_CENTRO_DER + 10, MA_CENTRO_Y);
}
void mascotaDibujarDosOjos(float desplazamiento, float ancho, float alto, float y) {
  mascotaDibujarOjo(MA_CENTRO_IZQ + desplazamiento, y, ancho, alto);
  mascotaDibujarOjo(MA_CENTRO_DER + desplazamiento, y, ancho, alto);
}
void mascotaDibujarGuino(float altoIzquierdo) {
  mascotaDibujarOjo(MA_CENTRO_IZQ, MA_CENTRO_Y, MA_OJO_ANCHO, altoIzquierdo);
  mascotaDibujarOjo(MA_CENTRO_DER, MA_CENTRO_Y, MA_OJO_ANCHO, MA_OJO_ALTO);
}
void limitarStats() {
  hambre = constrain(hambre, 0, 100); higiene = constrain(higiene, 0, 100);
  felicidad = constrain(felicidad, 0, 100); energia = constrain(energia, 0, 100);
}
void dibujarBarraStat(int x, int y, int valor) {
  pantalla.drawFrame(x, y, 46, 5);
  pantalla.drawBox(x + 1, y + 1, (44 * valor) / 100, 3);
}
void dibujarOjosMascota() {
  unsigned long transcurrido = millis() - mascotaInicioFase;
  float p = (float)transcurrido / (float)mascotaDuracionFase(mascotaFase);
  if (p > 1) p = 1;
  float suave = mascotaSuave(p);

  float desplazamiento = 0;
  float ancho = MA_OJO_ANCHO;
  float alto = MA_OJO_ALTO;
  float y = MA_CENTRO_Y;

  // Miradas suaves: mismo recorrido que el ejemplo (-15 / +15 / -8).
  if (mascotaFase == MASCOTA_MIRA_IZQ) desplazamiento = -15 * suave;
  else if (mascotaFase == MASCOTA_VUELVE_CENTRO_1) desplazamiento = -15 + 15 * suave;
  else if (mascotaFase == MASCOTA_MIRA_DER) desplazamiento = 15 * suave;
  else if (mascotaFase == MASCOTA_VUELVE_CENTRO_2) desplazamiento = 15 - 15 * suave;
  else if (mascotaFase == MASCOTA_MIRA_POCO_IZQ) desplazamiento = -8 * suave;
  else if (mascotaFase == MASCOTA_MIRA_POCO_CENTRO) desplazamiento = -8 + 8 * suave;

  // Parpadeo.
  if (mascotaFase == MASCOTA_PARPADEO_CIERRA || mascotaFase == MASCOTA_DESPERTAR_PARPADEO_CIERRA) {
    alto = MA_OJO_ALTO * (1.0 - suave);
    if (alto < 2) alto = 2;
  } else if (mascotaFase == MASCOTA_PARPADEO_ABRE || mascotaFase == MASCOTA_DESPERTAR_PARPADEO_ABRE) {
    alto = 2 + (MA_OJO_ALTO - 2) * suave;
  } else if (mascotaFase == MASCOTA_PARPADEO_ESPERA || mascotaFase == MASCOTA_DESPERTAR_PARPADEO_ESPERA) {
    mascotaOjosCerrados();
    return;
  }

  // Feliz: se achican y quedan en "^".
  if (mascotaFase == MASCOTA_FELIZ) {
    alto = MA_OJO_ALTO - (MA_OJO_ALTO - 6) * suave;
    y = MA_CENTRO_Y + 3 * suave;
  } else if (mascotaFase == MASCOTA_FELIZ_ESPERA) {
    mascotaDibujarOjoFeliz(MA_CENTRO_IZQ, MA_CENTRO_Y);
    mascotaDibujarOjoFeliz(MA_CENTRO_DER, MA_CENTRO_Y);
    return;
  } else if (mascotaFase == MASCOTA_FELIZ_NORMAL) {
    alto = 6 + (MA_OJO_ALTO - 6) * suave;
    y = MA_CENTRO_Y + 3 - 3 * suave;
  }

  // Sorpresa: crecen y rebotan.
  if (mascotaFase == MASCOTA_SORPRESA) {
    ancho = MA_OJO_ANCHO + 7 * suave;
    alto = MA_OJO_ALTO + 8 * suave;
  } else if (mascotaFase == MASCOTA_SORPRESA_REBOTE) {
    ancho = MA_OJO_ANCHO + 7;
    alto = MA_OJO_ALTO + 8;
    y = MA_CENTRO_Y + sin(p * PI * 3.0) * (1.0 - p) * 2.0;
  } else if (mascotaFase == MASCOTA_SORPRESA_NORMAL) {
    ancho = MA_OJO_ANCHO + 7 - 7 * suave;
    alto = MA_OJO_ALTO + 8 - 8 * suave;
  }

  // Guiño: solo se cierra el ojo izquierdo.
  if (mascotaFase == MASCOTA_GUINO_CIERRA) {
    float altoIzq = MA_OJO_ALTO - (MA_OJO_ALTO - 3) * suave;
    if (altoIzq < 3) altoIzq = 3;
    mascotaDibujarGuino(altoIzq); return;
  } else if (mascotaFase == MASCOTA_GUINO_ESPERA) {
    mascotaDibujarGuino(3); return;
  } else if (mascotaFase == MASCOTA_GUINO_ABRE) {
    mascotaDibujarGuino(3 + (MA_OJO_ALTO - 3) * suave); return;
  }

  // Dormir / despertar.
  if (mascotaFase == MASCOTA_DORMIR_CIERRA) {
    alto = MA_OJO_ALTO * (1.0 - suave);
    if (alto < 2) alto = 2;
  } else if (mascotaFase == MASCOTA_DORMIR) {
    mascotaOjosCerrados(); return;
  } else if (mascotaFase == MASCOTA_DESPERTAR) {
    alto = 2 + (MA_OJO_ALTO - 2) * suave;
  }

  mascotaDibujarDosOjos(desplazamiento, ancho, alto, y);
}
void actualizarFaseMascota() {
  // Si la mascota esta durmiendo se fuerza el tramo de sueno.
  if (durmiendo) {
    if (mascotaFase != MASCOTA_DORMIR && mascotaFase != MASCOTA_DORMIR_CIERRA)
      mascotaCambiarFase(MASCOTA_DORMIR_CIERRA);
    return;
  }
  if (millis() - mascotaInicioFase < mascotaDuracionFase(mascotaFase)) return;
  // Terminado el despertar, la secuencia vuelve al ciclo normal.
  if (mascotaFase == MASCOTA_DESPERTAR_PARPADEO_ABRE) { mascotaCambiarFase(MASCOTA_NORMAL); return; }
  mascotaSiguienteFase();
}
void dibujarMascota() {
  unsigned long ahora = millis();
  if (ahora - tiempoStatsMascota >= 10000) {
    tiempoStatsMascota = ahora; hambre += 2; higiene -= 2; felicidad -= 1;
    if (!durmiendo) energia -= 2;
    limitarStats();
  }
  if (ahora - ultimoCambioEstadistica >= 2200) {
    ultimoCambioEstadistica = ahora; estadisticaVisible = (estadisticaVisible + 1) % 4;
  }
  actualizarFaseMascota();

  pantalla.clearBuffer();
  pantalla.setFont(u8g2_font_6x10_tf);
  pantalla.drawStr(0, 9, durmiendo ? "Mascota - durmiendo" : "Mascota");
  pantalla.drawHLine(0, 11, 128);
  dibujarOjosMascota();
  // Los textos sueltos van en la esquina para no pisar los ojos.
  if (durmiendo) {
    int fase = (int)((ahora / 350) % 3);
    const char *z = fase == 0 ? "z" : (fase == 1 ? "zZ" : "Zz");
    pantalla.setFont(u8g2_font_4x6_tf);
    pantalla.drawStr(126 - pantalla.getStrWidth(z), 18, z);
  }
  if (animacionAccion && !durmiendo && ahora - finAccionMascota < 550) {
    const char *msg = animacionAccion == 1 ? "nom nom!" : (animacionAccion == 2 ? "limpio!" : "<3");
    pantalla.setFont(u8g2_font_4x6_tf);
    pantalla.drawStr(126 - pantalla.getStrWidth(msg), 6, msg);
  }
  pantalla.setFont(u8g2_font_5x8_tf);
  const char *nombres[] = {"Hambre", "Higiene", "Felicidad", "Energia"};
  int valores[] = {hambre, higiene, felicidad, energia};
  char buf[20];
  sprintf(buf, "%s: %d", nombres[estadisticaVisible], valores[estadisticaVisible]);
  pantalla.drawStr(2, 56, buf);
  dibujarBarraStat(78, 51, valores[estadisticaVisible]);
  pantalla.setFont(u8g2_font_4x6_tf);
  pantalla.drawStr(2, 63, durmiendo ? "OK: despierta" : "< > accion:");
  if (!durmiendo) pantalla.drawStr(68, 63, accionesMascota[accionMascota]);
  pantalla.sendBuffer();
}
void procesarMascota() {
  if (ok.largo) { volverAlMenu(); return; }
  unsigned long ahora = millis();
  if (ahora - maUltimoFrame < 16) return;
  if (durmiendo) {
    if (ahora - inicioSueno >= 30000) { durmiendo = false; energia += 25; limitarStats(); mascotaCambiarFase(MASCOTA_DESPERTAR); }
    if (ok.corto) { durmiendo = false; energia += 10; limitarStats(); mascotaCambiarFase(MASCOTA_DESPERTAR); }
  } else {
    if (izquierda.corto) accionMascota = (accionMascota == 0) ? 3 : accionMascota - 1;
    if (derecha.corto) accionMascota = (accionMascota + 1) % 4;
    if (ok.corto) {
      if (accionMascota == 0) { hambre -= 25; felicidad += 2; animacionAccion = 1; }
      if (accionMascota == 1) { higiene += 25; energia -= 3; animacionAccion = 2; }
      if (accionMascota == 2) { felicidad += 20; energia -= 2; animacionAccion = 3; }
      if (accionMascota == 3) { durmiendo = true; inicioSueno = ahora; }
      finAccionMascota = ahora; limitarStats();
    }
  }
  dibujarMascota();
  maUltimoFrame = millis();
}

// ============================================================
// BLACKJACK
// Animacion tomada de src/examples/ejemploBlackjack.ino
// Cartas que entran volando desde los laterales, mazo real de
// 52 cartas, palos dibujados y reparto temporizado.
// La logica de botones se respeta: izquierda = pedir carta,
// derecha = plantarse, OK corto = otra ronda, OK largo = menu.
// ============================================================

const int BJ_MAX_CARTAS = 10;
const int BJ_MAX_CARTAS_JUGADOR = 5;
const int BJ_MAX_CARTAS_CASA = 5;
const unsigned long BJ_FRAME_TIME = 16;
const unsigned long BJ_TIEMPO_ENTRE_CARTAS = 900;
const unsigned long BJ_TIEMPO_ANTES_RESULTADO = 2000;
const unsigned long BJ_TIEMPO_RESULTADO = 3000;
const float BJ_VELOCIDAD_ANIMACION = 0.18;

const int BJ_ANCHO_CARTA = 15;
const int BJ_ALTO_CARTA = 20;
const int BJ_ESPACIO_CARTAS = 2;
const int BJ_Y_TEXTO_CASA = 7;
const int BJ_Y_CARTAS_CASA = 8;
const int BJ_Y_SEPARADOR = 28;
const int BJ_Y_TEXTO_JUGADOR = 36;
const int BJ_Y_CARTAS_JUGADOR = 37;

const int BJ_RESULTADO_JUGADOR = 1;
const int BJ_RESULTADO_CASA = 2;
const int BJ_RESULTADO_EMPATE = 3;

// Mazo compacto: un solo byte por carta (0..51). El palo y el valor
// se deducen dividiendo entre 13, asi que la baraja completa ocupa
// 52 bytes en vez de los 104 de una estructura de dos bytes.
byte bjMazo[52];
int bjCartasEnMazo = 52;
CartaAnimada bjCartas[BJ_MAX_CARTAS];
int bjCantidadCartas = 0, bjCartasJugador = 0, bjCartasCasa = 0;
int bjResultado = 0;
EstadoJuegoBJ bjEstadoJuego = BJ_REPARTO_INICIAL;
PasoInicialBJ bjPasoInicial = BJ_INICIO_JUGADOR_1;
unsigned long bjUltimoFrame = 0, bjUltimaCarta = 0;
unsigned long bjInicioEsperaResultado = 0, bjInicioResultado = 0;

void bjCrearMazo() {
  for (int i = 0; i < 52; i++) bjMazo[i] = (byte)i;
  bjCartasEnMazo = 52;
}
void bjMezclarMazo() {
  for (int i = bjCartasEnMazo - 1; i > 0; i--) {
    int posicionAleatoria = random(0, i + 1);
    byte temporal = bjMazo[i];
    bjMazo[i] = bjMazo[posicionAleatoria];
    bjMazo[posicionAleatoria] = temporal;
  }
}
byte bjSacarCarta() {
  if (bjCartasEnMazo <= 0) { bjCrearMazo(); bjMezclarMazo(); }
  bjCartasEnMazo--;
  return bjMazo[bjCartasEnMazo];
}
int bjValorCarta(Carta carta) {
  if (carta.valor == 1) return 11;
  if (carta.valor >= 10) return 10;
  return carta.valor;
}
int bjCalcularPuntaje(bool jugador, bool incluirOculta) {
  int suma = 0, cantidadAses = 0;
  for (int i = 0; i < bjCantidadCartas; i++) {
    if (bjCartas[i].jugador != jugador) continue;
    if (!jugador && bjCartas[i].oculta && !incluirOculta) continue;
    suma += bjValorCarta(bjCartas[i].carta);
    if (bjCartas[i].carta.valor == 1) cantidadAses++;
  }
  while (suma > 21 && cantidadAses > 0) { suma -= 10; cantidadAses--; }
  return suma;
}
int bjPuntajeJugador() { return bjCalcularPuntaje(true, true); }
int bjPuntajeCasaVisible() { return bjCalcularPuntaje(false, false); }
int bjPuntajeCasaReal() { return bjCalcularPuntaje(false, true); }
int bjAnchoGrupo(int cantidad) {
  if (cantidad <= 0) return 0;
  return cantidad * BJ_ANCHO_CARTA + (cantidad - 1) * BJ_ESPACIO_CARTAS;
}
void bjActualizarPosicionesCartas() {
  int anchoJugador = bjAnchoGrupo(bjCartasJugador);
  int anchoCasa = bjAnchoGrupo(bjCartasCasa);
  int inicioJugador = (128 - anchoJugador) / 2;
  int inicioCasa = (128 - anchoCasa) / 2;
  int indiceJugador = 0, indiceCasa = 0;
  for (int i = 0; i < bjCantidadCartas; i++) {
    if (bjCartas[i].jugador) {
      bjCartas[i].destinoX = inicioJugador + indiceJugador * (BJ_ANCHO_CARTA + BJ_ESPACIO_CARTAS);
      bjCartas[i].destinoY = BJ_Y_CARTAS_JUGADOR;
      indiceJugador++;
    } else {
      bjCartas[i].destinoX = inicioCasa + indiceCasa * (BJ_ANCHO_CARTA + BJ_ESPACIO_CARTAS);
      bjCartas[i].destinoY = BJ_Y_CARTAS_CASA;
      indiceCasa++;
    }
  }
}
void bjAgregarCarta(bool jugador, bool oculta) {
  if (bjCantidadCartas >= BJ_MAX_CARTAS) return;
  if (jugador && bjCartasJugador >= BJ_MAX_CARTAS_JUGADOR) return;
  if (!jugador && bjCartasCasa >= BJ_MAX_CARTAS_CASA) return;
  byte nuevaCarta = bjSacarCarta();
  CartaAnimada &carta = bjCartas[bjCantidadCartas];
  carta.carta.palo = nuevaCarta / 13;
  carta.carta.valor = (nuevaCarta % 13) + 1;
  carta.jugador = jugador;
  carta.oculta = oculta;
  if ((jugador && bjCartasJugador % 2 == 0) || (!jugador && bjCartasCasa % 2 == 0)) carta.x = -20;
  else carta.x = 128 + 20;
  carta.y = jugador ? BJ_Y_CARTAS_JUGADOR : BJ_Y_CARTAS_CASA;
  carta.destinoX = carta.x;
  carta.destinoY = carta.y;
  bjCantidadCartas++;
  if (jugador) bjCartasJugador++; else bjCartasCasa++;
  bjActualizarPosicionesCartas();
}
void bjRevelarCartaCasa() {
  for (int i = 0; i < bjCantidadCartas; i++)
    if (!bjCartas[i].jugador && bjCartas[i].oculta) { bjCartas[i].oculta = false; break; }
}
void nuevaRondaBJ() {
  bjCantidadCartas = 0; bjCartasJugador = 0; bjCartasCasa = 0;
  bjResultado = 0; bjEstadoJuego = BJ_REPARTO_INICIAL;
  bjPasoInicial = BJ_INICIO_JUGADOR_1;
  bjCrearMazo(); bjMezclarMazo();
  bjUltimaCarta = millis();
}
void bjActualizarAnimaciones() {
  for (int i = 0; i < bjCantidadCartas; i++) {
    int diferenciaX = bjCartas[i].destinoX - bjCartas[i].x;
    int diferenciaY = bjCartas[i].destinoY - bjCartas[i].y;
    bjCartas[i].x += (int16_t)(diferenciaX * BJ_VELOCIDAD_ANIMACION);
    bjCartas[i].y += (int16_t)(diferenciaY * BJ_VELOCIDAD_ANIMACION);
    if (abs(diferenciaX) < 1) bjCartas[i].x = bjCartas[i].destinoX;
    if (abs(diferenciaY) < 1) bjCartas[i].y = bjCartas[i].destinoY;
  }
}
void bjDibujarCorazon(int x, int y) {
  pantalla.drawDisc(x + 4, y + 4, 3);
  pantalla.drawDisc(x + 9, y + 4, 3);
  llenarTriangulo(x + 1, y + 5, x + 12, y + 5, x + 6, y + 12);
}
void bjDibujarDiamante(int x, int y) {
  llenarTriangulo(x + 6, y, x + 12, y + 6, x + 6, y + 12);
  llenarTriangulo(x + 6, y, x, y + 6, x + 6, y + 12);
}
void bjDibujarTrebol(int x, int y) {
  pantalla.drawDisc(x + 3, y + 4, 3);
  pantalla.drawDisc(x + 9, y + 4, 3);
  pantalla.drawDisc(x + 6, y + 8, 3);
  llenarTriangulo(x + 6, y + 7, x + 3, y + 13, x + 9, y + 13);
}
void bjDibujarPicas(int x, int y) {
  llenarTriangulo(x + 6, y, x, y + 8, x + 12, y + 8);
  pantalla.drawDisc(x + 3, y + 8, 3);
  pantalla.drawDisc(x + 9, y + 8, 3);
  pantalla.drawBox(x + 5, y + 7, 3, 6);
}
void bjDibujarValor(int x, int y, byte valor) {
  pantalla.setFont(u8g2_font_4x6_tf);
  pantalla.setCursor(x, y);
  if (valor == 1) pantalla.print("A");
  else if (valor >= 2 && valor <= 9) pantalla.print(valor);
  else if (valor == 10) pantalla.print("10");
  else if (valor == 11) pantalla.print("J");
  else if (valor == 12) pantalla.print("Q");
  else if (valor == 13) pantalla.print("K");
}
void bjDibujarCarta(CartaAnimada &carta) {
  int x = (int)carta.x, y = (int)carta.y;
  if (carta.oculta) {
    pantalla.drawRBox(x, y, BJ_ANCHO_CARTA, BJ_ALTO_CARTA, 2);
    return;
  }
  pantalla.drawRBox(x, y, BJ_ANCHO_CARTA, BJ_ALTO_CARTA, 2);
  pantalla.setDrawColor(0);
  bjDibujarValor(x + 2, y + 5, carta.carta.valor);
  int paloX = x + 2;
  int paloY = y + 6;
  if (carta.carta.palo == 0) bjDibujarCorazon(paloX, paloY);
  else if (carta.carta.palo == 1) bjDibujarDiamante(paloX, paloY);
  else if (carta.carta.palo == 2) bjDibujarTrebol(paloX, paloY);
  else bjDibujarPicas(paloX, paloY);
  pantalla.setDrawColor(1);
}
void bjRepartoInicial() {
  if (millis() - bjUltimaCarta < BJ_TIEMPO_ENTRE_CARTAS) return;
  switch (bjPasoInicial) {
    case BJ_INICIO_JUGADOR_1:
      bjAgregarCarta(true, false); bjPasoInicial = BJ_INICIO_JUGADOR_2; bjUltimaCarta = millis(); break;
    case BJ_INICIO_JUGADOR_2:
      bjAgregarCarta(true, false); bjPasoInicial = BJ_INICIO_CASA_VISIBLE; bjUltimaCarta = millis(); break;
    case BJ_INICIO_CASA_VISIBLE:
      bjAgregarCarta(false, false); bjPasoInicial = BJ_INICIO_CASA_OCULTA; bjUltimaCarta = millis(); break;
    case BJ_INICIO_CASA_OCULTA:
      bjAgregarCarta(false, true); bjPasoInicial = BJ_INICIO_TERMINADO; bjUltimaCarta = millis(); break;
    case BJ_INICIO_TERMINADO:
      bjEstadoJuego = BJ_JUGADOR; bjUltimaCarta = millis(); break;
  }
}
void bjTurnoCasa() {
  int puntajeCasa = bjPuntajeCasaReal();
  int puntajeJugador = bjPuntajeJugador();
  if (puntajeCasa > 21) {
    bjResultado = BJ_RESULTADO_JUGADOR; bjEstadoJuego = BJ_ESPERANDO_RESULTADO;
    bjInicioEsperaResultado = millis(); return;
  }
  if (puntajeCasa >= 17) {
    if (puntajeJugador > puntajeCasa) bjResultado = BJ_RESULTADO_JUGADOR;
    else if (puntajeCasa > puntajeJugador) bjResultado = BJ_RESULTADO_CASA;
    else bjResultado = BJ_RESULTADO_EMPATE;
    bjEstadoJuego = BJ_ESPERANDO_RESULTADO;
    bjInicioEsperaResultado = millis(); return;
  }
  if (millis() - bjUltimaCarta >= BJ_TIEMPO_ENTRE_CARTAS) {
    bjAgregarCarta(false, false); bjUltimaCarta = millis();
  }
}
void bjActualizarJuego() {
  if (bjEstadoJuego == BJ_REPARTO_INICIAL) bjRepartoInicial();
  else if (bjEstadoJuego == BJ_REVELAR_CASA) {
    if (millis() - bjUltimaCarta >= BJ_TIEMPO_ENTRE_CARTAS) {
      bjRevelarCartaCasa(); bjEstadoJuego = BJ_CASA; bjUltimaCarta = millis();
    }
  } else if (bjEstadoJuego == BJ_CASA) bjTurnoCasa();
  else if (bjEstadoJuego == BJ_ESPERANDO_RESULTADO) {
    if (millis() - bjInicioEsperaResultado >= BJ_TIEMPO_ANTES_RESULTADO) {
      bjEstadoJuego = BJ_RESULTADO; bjInicioResultado = millis();
    }
  }
}
void bjDibujarJuego() {
  pantalla.clearBuffer();
  // Etiquetas cortas: el nombre del juego no hace falta aqui.
  pantalla.setFont(u8g2_font_5x8_tf);
  char buf[8]; sprintf(buf, "C %d", bjPuntajeCasaVisible());
  pantalla.drawStr(2, BJ_Y_TEXTO_CASA, buf);
  pantalla.drawHLine(0, BJ_Y_SEPARADOR, 128);
  sprintf(buf, "V %d", bjPuntajeJugador());
  pantalla.drawStr(2, BJ_Y_TEXTO_JUGADOR, buf);
  for (int i = 0; i < bjCantidadCartas; i++) bjDibujarCarta(bjCartas[i]);
  pantalla.setFont(u8g2_font_4x6_tf);
  pantalla.drawStr(2, 63, "IZQ carta   DER planta");
  pantalla.sendBuffer();
}
void bjDibujarResultado() {
  pantalla.clearBuffer();
  if (bjResultado == BJ_RESULTADO_JUGADOR) {
    textoCentrado("Ganaste!", 30, u8g2_font_10x20_tf);
  } else if (bjResultado == BJ_RESULTADO_CASA) {
    textoCentrado("Gana", 22, u8g2_font_10x20_tf);
    textoCentrado("la casa", 46, u8g2_font_10x20_tf);
  } else {
    textoCentrado("Empate", 30, u8g2_font_10x20_tf);
  }
  char buf[12];
  pantalla.setFont(u8g2_font_5x8_tf);
  sprintf(buf, "Casa %d", bjPuntajeCasaReal());
  pantalla.drawStr(6, 56, buf);
  sprintf(buf, "Vos %d", bjPuntajeJugador());
  pantalla.drawStr(84, 56, buf);
  textoCentrado("OK otra | mant: menu", 63, u8g2_font_4x6_tf);
  pantalla.sendBuffer();
}
void procesarBlackjack() {
  if (ok.largo) { volverAlMenu(); return; }
  if (bjEstadoJuego == BJ_RESULTADO) {
    if (ok.corto) nuevaRondaBJ();
    else if (millis() - bjInicioResultado >= BJ_TIEMPO_RESULTADO) nuevaRondaBJ();
  } else if (bjEstadoJuego == BJ_JUGADOR) {
    // Izquierda = pedir carta. Derecha = plantarse.
    if (izquierda.corto && bjCartasJugador < BJ_MAX_CARTAS_JUGADOR && bjPuntajeJugador() < 21) {
      bjAgregarCarta(true, false);
      bjUltimaCarta = millis();
    }
    if (derecha.corto) {
      bjEstadoJuego = BJ_REVELAR_CASA; bjUltimaCarta = millis();
    }
    if (bjPuntajeJugador() > 21) {
      bjResultado = BJ_RESULTADO_CASA; bjEstadoJuego = BJ_ESPERANDO_RESULTADO;
      bjInicioEsperaResultado = millis();
    } else if (bjPuntajeJugador() == 21) {
      bjEstadoJuego = BJ_REVELAR_CASA; bjUltimaCarta = millis();
    }
  }
  bjActualizarJuego();
  unsigned long ahora = millis();
  if (ahora - bjUltimoFrame >= BJ_FRAME_TIME) {
    bjUltimoFrame = ahora;
    bjActualizarAnimaciones();
    if (bjEstadoJuego == BJ_RESULTADO) bjDibujarResultado();
    else bjDibujarJuego();
  }
}

// ============================================================
// SNAKE
// Animacion tomada de src/examples/ejemploSnake.ino
// Serpiente con movimiento suave, cabeza redonda con ojos,
// comida que late y borde. El ejemplo la juega solo con IA;
// aqui manda el control de botones: izquierda gira a la
// izquierda, derecha gira a la derecha, OK pausa.
// El tablero se reduce a un marco interior para que la serpiente
// no pise ni el marcador ni la linea de controles.
// ============================================================

const int SN_MAX_SEGMENTOS = 40;
const float SN_VELOCIDAD = 1.35;
const float SN_DISTANCIA_COMIDA = 7.0;
const unsigned long SN_FRAME_TIME = 20;

// Marco del tablero: de (6,12) a (121,51).
const int SN_BORDE_X = 6;
const int SN_BORDE_Y = 12;
const int SN_BORDE_ANCHO = 116;
const int SN_BORDE_ALTO = 40;

float snX[SN_MAX_SEGMENTOS];
float snY[SN_MAX_SEGMENTOS];
int snLongitud = 8;
int snDireccionX = 0;
int snDireccionY = -1;
float snComidaX = 32;
float snComidaY = 20;
bool snComidaVisible = true;
byte snPuntos = 0;
bool snFin = false, snPausa = false;
unsigned long snUltimoFrame = 0, snTiempoMensaje = 0;
bool snEfectoComida = false;

float snDistancia(float x1, float y1, float x2, float y2) {
  float dx = x2 - x1;
  float dy = y2 - y1;
  return sqrt(dx * dx + dy * dy);
}
void snGenerarComida() {
  for (int intento = 0; intento < 50; intento++) {
    float x = random(SN_BORDE_X + 8, SN_BORDE_X + SN_BORDE_ANCHO - 10);
    float y = random(SN_BORDE_Y + 6, SN_BORDE_Y + SN_BORDE_ALTO - 8);
    x = ((int)x / 4) * 4;
    y = ((int)y / 4) * 4;
    bool libre = true;
    for (int i = 0; i < snLongitud; i++)
      if (snDistancia(x, y, snX[i], snY[i]) < SN_DISTANCIA_COMIDA) { libre = false; break; }
    if (libre) { snComidaX = x; snComidaY = y; snComidaVisible = true; return; }
  }
}
void iniciarSnake() {
  snLongitud = 8;
  float inicioX = 64, inicioY = SN_BORDE_Y + SN_BORDE_ALTO - 4;
  for (int i = 0; i < snLongitud; i++) { snX[i] = inicioX - i * 4; snY[i] = inicioY; }
  // IMPORTANTE: arrancar hacia arriba (igual que el ejemplo).
  snDireccionX = 0; snDireccionY = -1;
  snPuntos = 0; snFin = false; snPausa = false;
  snEfectoComida = false; snComidaVisible = true;
  snGenerarComida();
  snUltimoFrame = millis();
}
void snActualizarSerpiente() {
  for (int i = snLongitud - 1; i > 0; i--) { snX[i] = snX[i - 1]; snY[i] = snY[i - 1]; }
  snX[0] += snDireccionX * SN_VELOCIDAD;
  snY[0] += snDireccionY * SN_VELOCIDAD;
  if (snX[0] <= SN_BORDE_X + 1 || snX[0] >= SN_BORDE_X + SN_BORDE_ANCHO - 1 ||
      snY[0] <= SN_BORDE_Y + 1 || snY[0] >= SN_BORDE_Y + SN_BORDE_ALTO - 1) { snFin = true; return; }
  for (int i = 4; i < snLongitud; i++)
    if (snDistancia(snX[0], snY[0], snX[i], snY[i]) < 4.0) { snFin = true; return; }
  if (snComidaVisible && snDistancia(snX[0], snY[0], snComidaX, snComidaY) < SN_DISTANCIA_COMIDA) {
    if (snLongitud < SN_MAX_SEGMENTOS) {
      snX[snLongitud] = snX[snLongitud - 1];
      snY[snLongitud] = snY[snLongitud - 1];
      snLongitud++;
    }
    snPuntos++; snEfectoComida = true; snTiempoMensaje = millis();
    snGenerarComida();
  }
}
void snDibujarSerpiente() {
  for (int i = snLongitud - 1; i >= 0; i--) {
    int x = (int)snX[i];
    int y = (int)snY[i];
    if (i == 0) {
      pantalla.drawDisc(x, y, 3);
      int ojo1X = x, ojo1Y = y, ojo2X = x, ojo2Y = y;
      if (snDireccionX > 0) { ojo1X += 1; ojo1Y -= 1; ojo2X += 1; ojo2Y += 1; }
      else if (snDireccionX < 0) { ojo1X -= 1; ojo1Y -= 1; ojo2X -= 1; ojo2Y += 1; }
      else if (snDireccionY < 0) { ojo1X -= 1; ojo1Y -= 1; ojo2X += 1; ojo2Y -= 1; }
      else { ojo1X -= 1; ojo1Y += 1; ojo2X += 1; ojo2Y += 1; }
      pantalla.setDrawColor(0);
      pantalla.drawPixel(ojo1X, ojo1Y);
      pantalla.drawPixel(ojo2X, ojo2Y);
      pantalla.setDrawColor(1);
    } else {
      pantalla.drawDisc(x, y, 2);
    }
  }
}
void dibujarSnake() {
  unsigned long ahora = millis();
  pantalla.clearBuffer();
  // Solo el marcador: el nombre del juego sobra mientras se juega.
  pantalla.setFont(u8g2_font_4x6_tf);
  char buf[10]; sprintf(buf, "Pts %d", snPuntos);
  pantalla.drawStr(SN_BORDE_X + SN_BORDE_ANCHO - pantalla.getStrWidth(buf) - 2, 8, buf);
  pantalla.drawFrame(SN_BORDE_X, SN_BORDE_Y, SN_BORDE_ANCHO, SN_BORDE_ALTO);
  if (snComidaVisible) {
    int radio = 2;
    if (((millis() / 180) % 2) == 0) radio = 3;
    pantalla.drawDisc((int)snComidaX, (int)snComidaY, radio);
    pantalla.drawPixel((int)snComidaX, (int)snComidaY - radio - 1);
  }
  snDibujarSerpiente();
  if (snEfectoComida && ahora - snTiempoMensaje < 280) {
    pantalla.drawFrame((int)snComidaX - 4, (int)snComidaY - 4, 9, 9);
    pantalla.drawFrame((int)snComidaX - 5, (int)snComidaY - 5, 11, 11);
  }
  pantalla.setFont(u8g2_font_4x6_tf);
  if (snFin) {
    pantalla.drawRBox(18, 26, 92, 18, 2);
    pantalla.setDrawColor(0);
    textoCentrado("Fin del juego", 38, u8g2_font_6x10_tf);
    pantalla.setDrawColor(1);
    pantalla.drawStr(20, 63, "OK reinicia | menu");
  } else if (snPausa) {
    pantalla.drawRBox(34, 26, 60, 18, 2);
    pantalla.setDrawColor(0);
    textoCentrado("PAUSA", 38, u8g2_font_6x10_tf);
    pantalla.setDrawColor(1);
    pantalla.drawStr(20, 63, "OK sigue | menu");
  } else {
    pantalla.drawStr(2, 63, "< izq  > der  OK pausa");
  }
  pantalla.sendBuffer();
}
void snGirar(bool izquierdaGiro) {
  // Solo giros de 90 grados: no se permite dar media vuelta.
  int nuevaX, nuevaY;
  if (izquierdaGiro) { nuevaX = snDireccionY; nuevaY = -snDireccionX; }
  else { nuevaX = -snDireccionY; nuevaY = snDireccionX; }
  if (nuevaX == -snDireccionX && nuevaY == -snDireccionY) return;
  snDireccionX = nuevaX; snDireccionY = nuevaY;
}
void procesarSnake() {
  if (ok.largo) { volverAlMenu(); return; }
  if (snFin) { if (ok.corto) iniciarSnake(); dibujarSnake(); return; }
  if (ok.corto) { snPausa = !snPausa; if (!snPausa) snUltimoFrame = millis(); }
  if (!snPausa) {
    if (izquierda.corto) snGirar(true);
    if (derecha.corto) snGirar(false);
    unsigned long ahora = millis();
    if (ahora - snUltimoFrame >= SN_FRAME_TIME) {
      snUltimoFrame = ahora;
      snActualizarSerpiente();
    }
  }
  dibujarSnake();
}

// ============================================================
// INFORMACION
// ============================================================

void dibujarInformacion() {
  pantalla.clearBuffer(); dibujarCabecera("Informacion");
  pantalla.drawStr(0, 25, "Izq / Der: moverse");
  pantalla.drawStr(0, 36, "OK: elegir o actuar");
  pantalla.drawStr(0, 47, "Mantener OK: menu");
  pantalla.setFont(u8g2_font_5x8_tf); pantalla.drawStr(0, 61, "tinyPocket - Mega");
  pantalla.sendBuffer();
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  pinMode(PIN_IZQUIERDA, INPUT_PULLUP);
  pinMode(PIN_DERECHA, INPUT_PULLUP);
  pinMode(PIN_OK, INPUT_PULLUP);
  randomSeed(analogRead(A0) + micros());
  pantalla.begin();
  pantalla.setContrast(180);
  mostrarMensaje("tinyPocket", "Iniciando...");
  delay(500);
  tiempoStatsMascota = millis();
  ultimoCambioEstadistica = millis();
  iniciarMascota();
  nuevaRondaBJ();
  iniciarSnake();
}

void loop() {
  actualizarBotones();
  switch (estado) {
    case MENU: procesarMenu(); break;
    case SUBMENU_PINGPONG: procesarSubmenuPingPong(); break;
    case JUEGO_PINGPONG: procesarPingPong(); break;
    case JUEGO_BREAKOUT: procesarBreakout(); break;
    case JUEGO_MASCOTA: procesarMascota(); break;
    case JUEGO_BLACKJACK: procesarBlackjack(); break;
    case JUEGO_SNAKE: procesarSnake(); break;
    case INFORMACION: dibujarInformacion(); if (ok.corto || ok.largo) volverAlMenu(); break;
  }
}