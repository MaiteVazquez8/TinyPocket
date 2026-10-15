#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// ============================================================
// TinyPocket - version integrada
// OLED 128x64 I2C - direccion 0x3C
// Todo el programa usa millis(): no hay delay() en los juegos.
// ============================================================

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 pantalla(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ------------------------------------------------------------
// TIPOS: todos declarados arriba para evitar problemas con los
// prototipos automaticos del Arduino IDE.
// ------------------------------------------------------------

struct Carta {
  byte palo;
  byte valor;
};

struct Punto {
  float x;
  float y;
};

struct CartaAnimada {
  Carta carta;
  float x;
  float y;
  float destinoX;
  float destinoY;
  bool jugador;
  bool oculta;
};

enum EstadoPrincipal {
  ESTADO_INICIO,
  ESTADO_HOLA,
  ESTADO_BOTONES,
  ESTADO_MENU_PRINCIPAL,
  ESTADO_MENU_PINGPONG,
  ESTADO_EXPLICACION,
  ESTADO_JUEGO,
  ESTADO_RESULTADO,
  ESTADO_MASCOTA,
  ESTADO_INFORMACION
};

enum TipoJuego {
  JUEGO_NORMAL,
  JUEGO_BREAKOUT,
  JUEGO_SNAKE,
  JUEGO_BLACKJACK
};

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

// ------------------------------------------------------------
// TIEMPOS GENERALES
// ------------------------------------------------------------

const unsigned long FRAME_TIME = 16;
const unsigned long TIEMPO_INICIO = 2500;
const unsigned long TIEMPO_HOLA = 1800;
const unsigned long TIEMPO_BOTONES = 5000;
const unsigned long TIEMPO_OPCION_MENU = 2500;
const unsigned long TIEMPO_OPCION_PINGPONG = 2500;
const unsigned long TIEMPO_EXPLICACION = 4000;
const unsigned long TIEMPO_RESULTADO = 1500;
const unsigned long TIEMPO_INFORMACION = 5000;

unsigned long ultimoFrame = 0;
unsigned long inicioEstado = 0;
EstadoPrincipal estadoActual = ESTADO_INICIO;
TipoJuego juegoActual = JUEGO_NORMAL;

int seleccionMenu = 0;
int seleccionPingPong = 0;
int rondaActual = 0;

void cambiarEstado(EstadoPrincipal nuevoEstado) {
  estadoActual = nuevoEstado;
  inicioEstado = millis();

  if (nuevoEstado == ESTADO_MASCOTA) {
    mascotaIniciar();
  }

  if (nuevoEstado == ESTADO_JUEGO) {
    if (juegoActual == JUEGO_NORMAL) {
      pingpongReiniciar();
    } else if (juegoActual == JUEGO_BREAKOUT) {
      breakoutReiniciar();
    } else if (juegoActual == JUEGO_SNAKE) {
      snakeReiniciar();
    } else if (juegoActual == JUEGO_BLACKJACK) {
      blackjackIniciar();
    }
  }
}

bool pasoTiempo(unsigned long tiempo) {
  return millis() - inicioEstado >= tiempo;
}

void textoCentrado(const String &texto, int y, int tamano) {
  pantalla.setTextSize(tamano);
  pantalla.setTextColor(SSD1306_WHITE);
  int16_t x1, y1;
  uint16_t ancho, alto;
  pantalla.getTextBounds(texto, 0, y, &x1, &y1, &ancho, &alto);
  pantalla.setCursor((OLED_WIDTH - ancho) / 2, y);
  pantalla.print(texto);
}

void dibujarFlecha(int x, int y) {
  pantalla.fillTriangle(x, y, x + 6, y + 4, x, y + 8, SSD1306_WHITE);
}

// ============================================================
// PING PONG NORMAL
// ============================================================

const float PP_PADDLE_WIDTH = 28.0;
const float PP_PADDLE_HEIGHT = 3.0;
const float PP_PADDLE_Y = 58.0;
const float PP_BALL_RADIUS = 2.5;
const int PP_TRAIL_LENGTH = 8;

float ppPaletaX = 64.0;
float ppPaletaObjetivoX = 64.0;
float ppPelotaX = 64.0;
float ppPelotaY = 45.0;
float ppPelotaAnteriorX = 64.0;
float ppPelotaAnteriorY = 45.0;
float ppVelocidadX = 0.85;
float ppVelocidadY = -1.25;
float ppReboteAnimacion = 0.0;
Punto ppTrail[PP_TRAIL_LENGTH];

void pingpongInicializarTrail() {
  for (int i = 0; i < PP_TRAIL_LENGTH; i++) {
    ppTrail[i].x = ppPelotaX;
    ppTrail[i].y = ppPelotaY;
  }
}

void pingpongActualizarTrail() {
  for (int i = PP_TRAIL_LENGTH - 1; i > 0; i--) {
    ppTrail[i] = ppTrail[i - 1];
  }
  ppTrail[0].x = ppPelotaX;
  ppTrail[0].y = ppPelotaY;
}

void pingpongReiniciar() {
  ppPaletaX = 64.0;
  ppPaletaObjetivoX = 64.0;
  ppPelotaX = 64.0;
  ppPelotaY = 45.0;
  ppPelotaAnteriorX = 64.0;
  ppPelotaAnteriorY = 45.0;
  ppVelocidadX = 0.85;
  ppVelocidadY = -1.25;
  ppReboteAnimacion = 0.0;
  pingpongInicializarTrail();
}

void pingpongActualizarPaleta() {
  float tiempo = millis() / 1000.0;
  ppPaletaObjetivoX = 64.0 + sin(tiempo * 1.35) * 42.0;
  ppPaletaX += (ppPaletaObjetivoX - ppPaletaX) * 0.15;

  float limite = PP_PADDLE_WIDTH / 2.0 + 1.0;
  if (ppPaletaX < limite) ppPaletaX = limite;
  if (ppPaletaX > OLED_WIDTH - limite) ppPaletaX = OLED_WIDTH - limite;
}

void pingpongComprobarParedes() {
  if (ppPelotaX <= 1.0 + PP_BALL_RADIUS) {
    ppPelotaX = 1.0 + PP_BALL_RADIUS;
    ppVelocidadX = fabs(ppVelocidadX);
    ppReboteAnimacion = 1.0;
  }

  if (ppPelotaX >= OLED_WIDTH - 1.0 - PP_BALL_RADIUS) {
    ppPelotaX = OLED_WIDTH - 1.0 - PP_BALL_RADIUS;
    ppVelocidadX = -fabs(ppVelocidadX);
    ppReboteAnimacion = 1.0;
  }

  if (ppPelotaY <= 1.0 + PP_BALL_RADIUS) {
    ppPelotaY = 1.0 + PP_BALL_RADIUS;
    ppVelocidadY = fabs(ppVelocidadY);
    ppReboteAnimacion = 1.0;
  }
}

void pingpongComprobarPaleta() {
  if (ppVelocidadY <= 0) return;

  float izquierda = ppPaletaX - PP_PADDLE_WIDTH / 2.0;
  float derecha = ppPaletaX + PP_PADDLE_WIDTH / 2.0;
  float arriba = PP_PADDLE_Y - PP_PADDLE_HEIGHT / 2.0;

  if (ppPelotaY + PP_BALL_RADIUS >= arriba &&
      ppPelotaAnteriorY + PP_BALL_RADIUS <= arriba &&
      ppPelotaX >= izquierda && ppPelotaX <= derecha) {

    ppPelotaY = arriba - PP_BALL_RADIUS;
    ppVelocidadY = -fabs(ppVelocidadY);

    float diferencia = ppPelotaX - ppPaletaX;
    ppVelocidadX = diferencia * 0.08;
    if (fabs(ppVelocidadX) < 0.35) {
      if (diferencia < 0) ppVelocidadX = -0.35;
      else ppVelocidadX = 0.35;
    }

    ppReboteAnimacion = 1.0;
  }
}

void pingpongActualizarPelota() {
  ppPelotaAnteriorX = ppPelotaX;
  ppPelotaAnteriorY = ppPelotaY;
  pingpongActualizarTrail();

  ppPelotaX += ppVelocidadX;
  ppPelotaY += ppVelocidadY;

  pingpongComprobarParedes();
  pingpongComprobarPaleta();

  if (ppReboteAnimacion > 0) {
    ppReboteAnimacion -= 0.08;
    if (ppReboteAnimacion < 0) ppReboteAnimacion = 0;
  }

  if (ppPelotaY > OLED_HEIGHT + 5) {
    ppPelotaX = ppPaletaX;
    ppPelotaY = PP_PADDLE_Y - 7;
    ppVelocidadX = 0.85;
    ppVelocidadY = -1.25;
    pingpongInicializarTrail();
  }
}

void pingpongDibujar() {
  pantalla.clearDisplay();
  pantalla.drawFastVLine(0, 0, OLED_HEIGHT, SSD1306_WHITE);
  pantalla.drawFastVLine(OLED_WIDTH - 1, 0, OLED_HEIGHT, SSD1306_WHITE);
  pantalla.drawFastHLine(0, 0, OLED_WIDTH, SSD1306_WHITE);

  for (int i = PP_TRAIL_LENGTH - 1; i >= 0; i--) {
    int radio = 1;
    if (i < 2) radio = 2;
    pantalla.fillCircle((int)ppTrail[i].x, (int)ppTrail[i].y, radio, SSD1306_WHITE);
  }

  pantalla.fillRoundRect((int)(ppPaletaX - PP_PADDLE_WIDTH / 2),
                         (int)(PP_PADDLE_Y - PP_PADDLE_HEIGHT / 2),
                         (int)PP_PADDLE_WIDTH, (int)PP_PADDLE_HEIGHT, 2, SSD1306_WHITE);

  int radioPelota = 2;
  if (ppReboteAnimacion > 0.5) radioPelota = 3;
  pantalla.fillCircle((int)ppPelotaX, (int)ppPelotaY, radioPelota, SSD1306_WHITE);
  pantalla.display();
}

// ============================================================
// BREAKOUT
// ============================================================

const int BO_FILAS = 4;
const int BO_COLUMNAS = 8;
const int BO_BLOQUE_ANCHO = 13;
const int BO_BLOQUE_ALTO = 5;
const int BO_BLOQUE_SEPARACION = 2;
const int BO_BLOQUES_X = 3;
const int BO_BLOQUES_Y = 7;
const float BO_RADIO = 2.5;
const float BO_PALETA_ANCHO = 28.0;
const float BO_PALETA_ALTO = 3.0;
const float BO_PALETA_Y = 58.0;
const int BO_TRAIL_LENGTH = 8;

bool boBloqueVivo[BO_FILAS][BO_COLUMNAS];
int boBloquesRestantes = 0;
float boPaletaX = 64.0;
float boPaletaObjetivoX = 64.0;
float boPelotaX = 64.0;
float boPelotaY = 48.0;
float boPelotaAnteriorX = 64.0;
float boPelotaAnteriorY = 48.0;
float boVelocidadX = 1.0;
float boVelocidadY = -1.4;
float boReboteAnimacion = 0;
Punto boTrail[BO_TRAIL_LENGTH];

void breakoutInicializarTrail() {
  for (int i = 0; i < BO_TRAIL_LENGTH; i++) {
    boTrail[i].x = boPelotaX;
    boTrail[i].y = boPelotaY;
  }
}

void breakoutInicializarBloques() {
  boBloquesRestantes = 0;
  for (int fila = 0; fila < BO_FILAS; fila++) {
    for (int columna = 0; columna < BO_COLUMNAS; columna++) {
      boBloqueVivo[fila][columna] = true;
      boBloquesRestantes++;
    }
  }
}

void breakoutReiniciarPelota() {
  boPelotaX = boPaletaX;
  boPelotaY = BO_PALETA_Y - 7;
  boPelotaAnteriorX = boPelotaX;
  boPelotaAnteriorY = boPelotaY;
  boVelocidadX = 1.0;
  boVelocidadY = -1.4;
  boReboteAnimacion = 0;
  breakoutInicializarTrail();
}

void breakoutReiniciar() {
  boPaletaX = 64.0;
  boPaletaObjetivoX = 64.0;
  breakoutInicializarBloques();
  breakoutReiniciarPelota();
}

void breakoutActualizarPaleta() {
  float tiempo = millis() / 1000.0;
  boPaletaObjetivoX = 64.0 + sin(tiempo * 1.15) * 43.0;
  boPaletaX += (boPaletaObjetivoX - boPaletaX) * 0.15;
  float limite = BO_PALETA_ANCHO / 2.0 + 1;
  if (boPaletaX < limite) boPaletaX = limite;
  if (boPaletaX > OLED_WIDTH - limite) boPaletaX = OLED_WIDTH - limite;
}

void breakoutActualizarTrail() {
  for (int i = BO_TRAIL_LENGTH - 1; i > 0; i--) boTrail[i] = boTrail[i - 1];
  boTrail[0].x = boPelotaX;
  boTrail[0].y = boPelotaY;
}

void breakoutComprobarParedes() {
  if (boPelotaX <= 1 + BO_RADIO) {
    boPelotaX = 1 + BO_RADIO;
    boVelocidadX = fabs(boVelocidadX);
    boReboteAnimacion = 1;
  }
  if (boPelotaX >= OLED_WIDTH - 1 - BO_RADIO) {
    boPelotaX = OLED_WIDTH - 1 - BO_RADIO;
    boVelocidadX = -fabs(boVelocidadX);
    boReboteAnimacion = 1;
  }
  if (boPelotaY <= 1 + BO_RADIO) {
    boPelotaY = 1 + BO_RADIO;
    boVelocidadY = fabs(boVelocidadY);
    boReboteAnimacion = 1;
  }
}

void breakoutComprobarBloques() {
  for (int fila = 0; fila < BO_FILAS; fila++) {
    for (int columna = 0; columna < BO_COLUMNAS; columna++) {
      if (!boBloqueVivo[fila][columna]) continue;

      float bloqueX = BO_BLOQUES_X + columna * (BO_BLOQUE_ANCHO + BO_BLOQUE_SEPARACION);
      float bloqueY = BO_BLOQUES_Y + fila * (BO_BLOQUE_ALTO + BO_BLOQUE_SEPARACION);
      float bloqueDerecha = bloqueX + BO_BLOQUE_ANCHO;
      float bloqueAbajo = bloqueY + BO_BLOQUE_ALTO;

      bool tocaX = boPelotaX + BO_RADIO >= bloqueX && boPelotaX - BO_RADIO <= bloqueDerecha;
      bool tocaY = boPelotaY + BO_RADIO >= bloqueY && boPelotaY - BO_RADIO <= bloqueAbajo;

      if (!tocaX || !tocaY) continue;

      bool desdeArriba = boPelotaAnteriorY + BO_RADIO <= bloqueY;
      bool desdeAbajo = boPelotaAnteriorY - BO_RADIO >= bloqueAbajo;
      bool desdeIzquierda = boPelotaAnteriorX + BO_RADIO <= bloqueX;
      bool desdeDerecha = boPelotaAnteriorX - BO_RADIO >= bloqueDerecha;

      boBloqueVivo[fila][columna] = false;
      boBloquesRestantes--;
      boReboteAnimacion = 1;

      if (desdeArriba || desdeAbajo) boVelocidadY = -boVelocidadY;
      else if (desdeIzquierda || desdeDerecha) boVelocidadX = -boVelocidadX;
      else boVelocidadY = -boVelocidadY;

      if (fabs(boVelocidadX) < 0.45) {
        if (boVelocidadX < 0) boVelocidadX = -0.45;
        else boVelocidadX = 0.45;
      }
      if (fabs(boVelocidadX) < 2.8) boVelocidadX *= 1.03;
      if (fabs(boVelocidadY) < 2.8) boVelocidadY *= 1.03;
      return;
    }
  }
}

void breakoutComprobarPaleta() {
  if (boVelocidadY <= 0) return;

  float izquierda = boPaletaX - BO_PALETA_ANCHO / 2;
  float derecha = boPaletaX + BO_PALETA_ANCHO / 2;
  float arriba = BO_PALETA_Y - BO_PALETA_ALTO / 2;

  if (boPelotaY + BO_RADIO >= arriba &&
      boPelotaAnteriorY + BO_RADIO <= arriba &&
      boPelotaX >= izquierda && boPelotaX <= derecha) {
    boPelotaY = arriba - BO_RADIO;
    boVelocidadY = -fabs(boVelocidadY);
    float diferencia = boPelotaX - boPaletaX;
    boVelocidadX = diferencia * 0.08;
    if (fabs(boVelocidadX) < 0.35) {
      if (diferencia < 0) boVelocidadX = -0.35;
      else boVelocidadX = 0.35;
    }
    boReboteAnimacion = 1;
  }
}

void breakoutActualizarPelota() {
  boPelotaAnteriorX = boPelotaX;
  boPelotaAnteriorY = boPelotaY;
  breakoutActualizarTrail();
  boPelotaX += boVelocidadX;
  boPelotaY += boVelocidadY;
  breakoutComprobarParedes();
  breakoutComprobarBloques();
  breakoutComprobarPaleta();

  if (boReboteAnimacion > 0) {
    boReboteAnimacion -= 0.08;
    if (boReboteAnimacion < 0) boReboteAnimacion = 0;
  }

  if (boPelotaY > OLED_HEIGHT + 5) breakoutReiniciarPelota();
  if (boBloquesRestantes <= 0) breakoutInicializarBloques();
}

void breakoutDibujar() {
  pantalla.clearDisplay();
  pantalla.drawFastVLine(0, 0, OLED_HEIGHT, SSD1306_WHITE);
  pantalla.drawFastVLine(OLED_WIDTH - 1, 0, OLED_HEIGHT, SSD1306_WHITE);
  pantalla.drawFastHLine(0, 0, OLED_WIDTH, SSD1306_WHITE);

  for (int fila = 0; fila < BO_FILAS; fila++) {
    for (int columna = 0; columna < BO_COLUMNAS; columna++) {
      if (boBloqueVivo[fila][columna]) {
        int x = BO_BLOQUES_X + columna * (BO_BLOQUE_ANCHO + BO_BLOQUE_SEPARACION);
        int y = BO_BLOQUES_Y + fila * (BO_BLOQUE_ALTO + BO_BLOQUE_SEPARACION);
        pantalla.fillRect(x, y, BO_BLOQUE_ANCHO, BO_BLOQUE_ALTO, SSD1306_WHITE);
      }
    }
  }

  for (int i = BO_TRAIL_LENGTH - 1; i >= 0; i--) {
    int radio = 1;
    if (i < 2) radio = 2;
    pantalla.fillCircle((int)boTrail[i].x, (int)boTrail[i].y, radio, SSD1306_WHITE);
  }

  pantalla.fillRoundRect((int)(boPaletaX - BO_PALETA_ANCHO / 2),
                         (int)(BO_PALETA_Y - BO_PALETA_ALTO / 2),
                         (int)BO_PALETA_ANCHO, (int)BO_PALETA_ALTO, 2, SSD1306_WHITE);

  int radio = 2;
  if (boReboteAnimacion > 0.5) radio = 3;
  pantalla.fillCircle((int)boPelotaX, (int)boPelotaY, radio, SSD1306_WHITE);
  pantalla.display();
}

// ============================================================
// SNAKE
// ============================================================

const int SN_MAX_SEGMENTOS = 40;
const float SN_VELOCIDAD = 1.55;
const float SN_DISTANCIA_COMIDA = 8.0;

float snX[SN_MAX_SEGMENTOS];
float snY[SN_MAX_SEGMENTOS];
int snLongitud = 8;
float snDirX = 0;
float snDirY = -1;
float snComidaX = 32;
float snComidaY = 20;
bool snComidaVisible = true;
unsigned long snUltimoMovimiento = 0;
unsigned long snUltimoPensamiento = 0;

float snDistancia(float x1, float y1, float x2, float y2) {
  float dx = x1 - x2;
  float dy = y1 - y2;
  return sqrt(dx * dx + dy * dy);
}

void snakeGenerarComida() {
  for (int intento = 0; intento < 50; intento++) {
    float x = random(12, 116);
    float y = random(8, 56);
    x = ((int)x / 4) * 4;
    y = ((int)y / 4) * 4;

    bool libre = true;
    for (int i = 0; i < snLongitud; i++) {
      if (snDistancia(x, y, snX[i], snY[i]) < SN_DISTANCIA_COMIDA) {
        libre = false;
        break;
      }
    }
    if (libre) {
      snComidaX = x;
      snComidaY = y;
      snComidaVisible = true;
      return;
    }
  }
}

bool snakePosicionPeligrosa(float x, float y) {
  if (x <= 4 || x >= 124 || y <= 4 || y >= 60) return true;
  for (int i = 4; i < snLongitud - 2; i++) {
    if (snDistancia(x, y, snX[i], snY[i]) < 5.5) return true;
  }
  return false;
}

bool snakeDireccionSegura(float dx, float dy) {
  for (int distancia = 5; distancia <= 15; distancia += 5) {
    float x = snX[0] + dx * distancia;
    float y = snY[0] + dy * distancia;
    if (snakePosicionPeligrosa(x, y)) return false;
  }
  return true;
}

float snakeEvaluarDireccion(float dx, float dy) {
  float futuroX = snX[0] + dx * 25;
  float futuroY = snY[0] + dy * 25;
  float distanciaComida = snDistancia(futuroX, futuroY, snComidaX, snComidaY);
  float puntaje = distanciaComida;

  if (snakeDireccionSegura(dx, dy)) puntaje -= 8;
  else puntaje += 1000;

  if (dx == -snDirX && dy == -snDirY) puntaje += 500;

  if (fabs(dx) > 0 && fabs(snComidaX - snX[0]) < fabs(snComidaY - snY[0])) puntaje -= 2;
  if (fabs(dy) > 0 && fabs(snComidaY - snY[0]) < fabs(snComidaX - snX[0])) puntaje -= 2;

  puntaje += random(0, 6);
  return puntaje;
}

void snakePensarMovimiento() {
  float dirsX[4] = {1, -1, 0, 0};
  float dirsY[4] = {0, 0, 1, -1};
  float mejorPuntaje = 1000000;
  int mejor = -1;

  for (int i = 0; i < 4; i++) {
    if (dirsX[i] == -snDirX && dirsY[i] == -snDirY) continue;
    float puntaje = snakeEvaluarDireccion(dirsX[i], dirsY[i]);
    if (puntaje < mejorPuntaje) {
      mejorPuntaje = puntaje;
      mejor = i;
    }
  }

  if (mejor >= 0) {
    snDirX = dirsX[mejor];
    snDirY = dirsY[mejor];
  }
}

bool snakeChocoConSuCuerpo() {
  for (int i = 4; i < snLongitud; i++) {
    if (snDistancia(snX[0], snY[0], snX[i], snY[i]) < 4) return true;
  }
  return false;
}

bool snakeEstaComiendo() {
  return snDistancia(snX[0], snY[0], snComidaX, snComidaY) < 8;
}

void snakeReiniciar() {
  snLongitud = 8;
  snDirX = 0;
  snDirY = -1;
  for (int i = 0; i < snLongitud; i++) {
    snX[i] = 64;
    snY[i] = 40 + i * 4;
  }
  snakeGenerarComida();
  snUltimoMovimiento = millis();
  snUltimoPensamiento = millis();
}

void snakeActualizar() {
  unsigned long ahora = millis();
  if (ahora - snUltimoPensamiento >= 80) {
    snUltimoPensamiento = ahora;
    snakePensarMovimiento();
  }

  if (ahora - snUltimoMovimiento < 20) return;
  snUltimoMovimiento = ahora;

  for (int i = snLongitud - 1; i > 0; i--) {
    snX[i] = snX[i - 1];
    snY[i] = snY[i - 1];
  }

  snX[0] += snDirX * SN_VELOCIDAD;
  snY[0] += snDirY * SN_VELOCIDAD;

  if (snX[0] <= 1 || snX[0] >= 127 || snY[0] <= 1 || snY[0] >= 63) {
    snakeReiniciar();
    return;
  }

  if (snakeChocoConSuCuerpo()) {
    snakeReiniciar();
    return;
  }

  if (snakeEstaComiendo()) {
    if (snLongitud < SN_MAX_SEGMENTOS) {
      snX[snLongitud] = snX[snLongitud - 1];
      snY[snLongitud] = snY[snLongitud - 1];
      snLongitud++;
    }
    snakeGenerarComida();
  }
}

void snakeDibujar() {
  pantalla.clearDisplay();
  pantalla.drawRect(0, 0, OLED_WIDTH, OLED_HEIGHT, SSD1306_WHITE);

  if (snComidaVisible) {
    int radio = 2;
    if ((millis() / 180) % 2 == 0) radio = 3;
    pantalla.fillCircle((int)snComidaX, (int)snComidaY, radio, SSD1306_WHITE);
    pantalla.drawPixel((int)snComidaX, (int)snComidaY - 4, SSD1306_WHITE);
  }

  for (int i = snLongitud - 1; i >= 1; i--) {
    pantalla.fillCircle((int)snX[i], (int)snY[i], 2, SSD1306_WHITE);
  }

  pantalla.fillCircle((int)snX[0], (int)snY[0], 3, SSD1306_WHITE);

  int ojo1X = (int)snX[0];
  int ojo1Y = (int)snY[0];
  int ojo2X = ojo1X;
  int ojo2Y = ojo1Y;

  if (snDirX > 0) {
    ojo1X += 1; ojo2X += 1; ojo1Y -= 1; ojo2Y += 1;
  } else if (snDirX < 0) {
    ojo1X -= 1; ojo2X -= 1; ojo1Y -= 1; ojo2Y += 1;
  } else if (snDirY < 0) {
    ojo1Y -= 1; ojo2Y -= 1; ojo1X -= 1; ojo2X += 1;
  } else {
    ojo1Y += 1; ojo2Y += 1; ojo1X -= 1; ojo2X += 1;
  }

  pantalla.drawPixel(ojo1X, ojo1Y, SSD1306_BLACK);
  pantalla.drawPixel(ojo2X, ojo2Y, SSD1306_BLACK);
  pantalla.display();
}

// ============================================================
// BLACKJACK
// ============================================================

const int BJ_MAX_CARTAS = 10;
const int BJ_MAX_CARTAS_JUGADOR = 5;
const int BJ_MAX_CARTAS_CASA = 5;
const unsigned long BJ_TIEMPO_ENTRE_CARTAS = 900;
const unsigned long BJ_TIEMPO_ANTES_RESULTADO = 2000;
const unsigned long BJ_TIEMPO_RESULTADO = 3000;
const float BJ_VELOCIDAD_ANIMACION = 0.18;

Carta bjMazo[52];
int bjCartasEnMazo = 52;
CartaAnimada bjCartas[BJ_MAX_CARTAS];
int bjCantidadCartas = 0;
int bjCartasJugador = 0;
int bjCartasCasa = 0;

int bjPasoReparto = 0;
unsigned long bjUltimaCarta = 0;
unsigned long bjInicioEspera = 0;
unsigned long bjInicioResultado = 0;
int bjResultado = 0;
int bjEstado = 0;
// 0 reparto, 1 jugador, 2 revelar casa, 3 casa, 4 espera, 5 resultado

const int BJ_ANCHO_CARTA = 17;
const int BJ_ALTO_CARTA = 18;
const int BJ_ESPACIO_CARTAS = 3;
const int BJ_Y_TEXTO_CASA = 0;
const int BJ_Y_CARTAS_CASA = 8;
const int BJ_Y_SEPARADOR = 29;
const int BJ_Y_TEXTO_JUGADOR = 32;
const int BJ_Y_CARTAS_JUGADOR = 41;

void blackjackCrearMazo() {
  int indice = 0;
  for (byte palo = 0; palo < 4; palo++) {
    for (byte valor = 1; valor <= 13; valor++) {
      bjMazo[indice].palo = palo;
      bjMazo[indice].valor = valor;
      indice++;
    }
  }
  bjCartasEnMazo = 52;
}

void blackjackMezclarMazo() {
  for (int i = 51; i > 0; i--) {
    int j = random(0, i + 1);
    Carta temporal = bjMazo[i];
    bjMazo[i] = bjMazo[j];
    bjMazo[j] = temporal;
  }
}

Carta blackjackSacarCarta() {
  if (bjCartasEnMazo <= 0) {
    blackjackCrearMazo();
    blackjackMezclarMazo();
  }
  bjCartasEnMazo--;
  return bjMazo[bjCartasEnMazo];
}

int blackjackValorCarta(Carta carta) {
  if (carta.valor == 1) return 11;
  if (carta.valor >= 10) return 10;
  return carta.valor;
}

int blackjackCalcularPuntaje(bool jugador, bool incluirOculta) {
  int puntaje = 0;
  int ases = 0;

  for (int i = 0; i < bjCantidadCartas; i++) {
    if (bjCartas[i].jugador != jugador) continue;
    if (!incluirOculta && bjCartas[i].oculta) continue;

    int valor = blackjackValorCarta(bjCartas[i].carta);
    puntaje += valor;
    if (bjCartas[i].carta.valor == 1) ases++;
  }

  while (puntaje > 21 && ases > 0) {
    puntaje -= 10;
    ases--;
  }
  return puntaje;
}

int blackjackPuntajeJugador() {
  return blackjackCalcularPuntaje(true, true);
}

int blackjackPuntajeCasaVisible() {
  return blackjackCalcularPuntaje(false, false);
}

int blackjackPuntajeCasaReal() {
  return blackjackCalcularPuntaje(false, true);
}

int blackjackAnchoGrupo(int cantidad) {
  if (cantidad <= 0) return 0;
  return cantidad * BJ_ANCHO_CARTA + (cantidad - 1) * BJ_ESPACIO_CARTAS;
}

void blackjackActualizarPosicionesCartas() {
  int anchoJugador = blackjackAnchoGrupo(bjCartasJugador);
  int anchoCasa = blackjackAnchoGrupo(bjCartasCasa);

  int inicioJugador = (OLED_WIDTH - anchoJugador) / 2;
  int inicioCasa = (OLED_WIDTH - anchoCasa) / 2;

  int indiceJugador = 0;
  int indiceCasa = 0;

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

void blackjackAgregarCarta(bool jugador, bool oculta) {
  if (bjCantidadCartas >= BJ_MAX_CARTAS) return;
  if (jugador && bjCartasJugador >= BJ_MAX_CARTAS_JUGADOR) return;
  if (!jugador && bjCartasCasa >= BJ_MAX_CARTAS_CASA) return;

  Carta nueva = blackjackSacarCarta();
  CartaAnimada &carta = bjCartas[bjCantidadCartas];
  carta.carta = nueva;
  carta.jugador = jugador;
  carta.oculta = oculta;
  carta.x = 55;
  carta.y = 30;
  carta.destinoX = 55;
  carta.destinoY = 30;

  bjCantidadCartas++;
  if (jugador) bjCartasJugador++;
  else bjCartasCasa++;

  blackjackActualizarPosicionesCartas();
}

void blackjackIniciar() {
  blackjackCrearMazo();
  blackjackMezclarMazo();
  bjCantidadCartas = 0;
  bjCartasJugador = 0;
  bjCartasCasa = 0;
  bjPasoReparto = 0;
  bjEstado = 0;
  bjResultado = 0;
  bjUltimaCarta = millis() - BJ_TIEMPO_ENTRE_CARTAS;
}

void blackjackActualizarAnimaciones() {
  for (int i = 0; i < bjCantidadCartas; i++) {
    bjCartas[i].x += (bjCartas[i].destinoX - bjCartas[i].x) * BJ_VELOCIDAD_ANIMACION;
    bjCartas[i].y += (bjCartas[i].destinoY - bjCartas[i].y) * BJ_VELOCIDAD_ANIMACION;
  }
}

void blackjackDibujarCorazon(int x, int y) {
  pantalla.fillCircle(x - 2, y - 1, 3, SSD1306_BLACK);
  pantalla.fillCircle(x + 2, y - 1, 3, SSD1306_BLACK);
  pantalla.fillTriangle(x - 5, y, x + 5, y, x, y + 7, SSD1306_BLACK);
}

void blackjackDibujarDiamante(int x, int y) {
  pantalla.fillTriangle(x, y - 4, x - 4, y, x, y + 4, SSD1306_BLACK);
  pantalla.fillTriangle(x, y - 4, x + 4, y, x, y + 4, SSD1306_BLACK);
}

void blackjackDibujarCarta(CartaAnimada &carta) {
  int x = (int)carta.x;
  int y = (int)carta.y;
  pantalla.drawRoundRect(x, y, BJ_ANCHO_CARTA, BJ_ALTO_CARTA, 2, SSD1306_WHITE);

  if (carta.oculta) {
    pantalla.drawRect(x + 3, y + 3, BJ_ANCHO_CARTA - 6, BJ_ALTO_CARTA - 6, SSD1306_WHITE);
    pantalla.drawLine(x + 4, y + 4, x + BJ_ANCHO_CARTA - 5, y + BJ_ALTO_CARTA - 5, SSD1306_WHITE);
    pantalla.drawLine(x + BJ_ANCHO_CARTA - 5, y + 4, x + 4, y + BJ_ALTO_CARTA - 5, SSD1306_WHITE);
    return;
  }

  pantalla.setTextColor(SSD1306_BLACK, SSD1306_WHITE);
  pantalla.setTextSize(1);
  pantalla.setCursor(x + 2, y + 2);

  if (carta.carta.valor == 1) pantalla.print("A");
  else if (carta.carta.valor == 11) pantalla.print("J");
  else if (carta.carta.valor == 12) pantalla.print("Q");
  else if (carta.carta.valor == 13) pantalla.print("K");
  else pantalla.print(carta.carta.valor);

  int centroX = x + BJ_ANCHO_CARTA / 2;
  int centroY = y + 11;
  if (carta.carta.palo == 0) {
    pantalla.fillCircle(centroX, centroY, 3, SSD1306_BLACK);
  } else if (carta.carta.palo == 1) {
    blackjackDibujarDiamante(centroX, centroY);
  } else if (carta.carta.palo == 2) {
    blackjackDibujarCorazon(centroX, centroY);
  } else {
    pantalla.drawLine(centroX - 3, centroY - 3, centroX + 3, centroY + 3, SSD1306_BLACK);
    pantalla.drawLine(centroX + 3, centroY - 3, centroX - 3, centroY + 3, SSD1306_BLACK);
  }
}

void blackjackResolver() {
  int jugador = blackjackPuntajeJugador();
  int casa = blackjackPuntajeCasaReal();

  if (jugador > 21) bjResultado = 2;
  else if (casa > 21) bjResultado = 1;
  else if (jugador > casa) bjResultado = 1;
  else if (casa > jugador) bjResultado = 2;
  else bjResultado = 3;
}

void blackjackActualizar() {
  blackjackActualizarAnimaciones();
  unsigned long ahora = millis();

  if (bjEstado == 0) {
    if (bjPasoReparto < 4 && ahora - bjUltimaCarta >= BJ_TIEMPO_ENTRE_CARTAS) {
      if (bjPasoReparto == 0) blackjackAgregarCarta(true, false);
      if (bjPasoReparto == 1) blackjackAgregarCarta(false, false);
      if (bjPasoReparto == 2) blackjackAgregarCarta(true, false);
      if (bjPasoReparto == 3) blackjackAgregarCarta(false, true);
      bjPasoReparto++;
      bjUltimaCarta = ahora;
    }

    if (bjPasoReparto >= 4 && ahora - bjUltimaCarta >= 700) {
      bjEstado = 1;
      bjUltimaCarta = ahora;
    }
  } else if (bjEstado == 1) {
    int puntaje = blackjackPuntajeJugador();
    if (puntaje < 17 && bjCartasJugador < BJ_MAX_CARTAS_JUGADOR && ahora - bjUltimaCarta >= 900) {
      blackjackAgregarCarta(true, false);
      bjUltimaCarta = ahora;
    } else if (puntaje >= 17 || bjCartasJugador >= BJ_MAX_CARTAS_JUGADOR || puntaje > 21) {
      bjEstado = 2;
      bjInicioEspera = ahora;
    }
  } else if (bjEstado == 2) {
    if (ahora - bjInicioEspera >= 600) {
      for (int i = 0; i < bjCantidadCartas; i++) {
        if (!bjCartas[i].jugador && bjCartas[i].oculta) bjCartas[i].oculta = false;
      }
      bjEstado = 3;
      bjUltimaCarta = ahora;
    }
  } else if (bjEstado == 3) {
    int puntaje = blackjackPuntajeCasaReal();
    if (puntaje < 17 && bjCartasCasa < BJ_MAX_CARTAS_CASA && ahora - bjUltimaCarta >= 900) {
      blackjackAgregarCarta(false, false);
      bjUltimaCarta = ahora;
    } else if (puntaje >= 17 || bjCartasCasa >= BJ_MAX_CARTAS_CASA) {
      bjEstado = 4;
      bjInicioEspera = ahora;
      blackjackResolver();
    }
  } else if (bjEstado == 4) {
    if (ahora - bjInicioEspera >= BJ_TIEMPO_ANTES_RESULTADO) {
      bjEstado = 5;
      bjInicioResultado = ahora;
    }
  }
}

void blackjackDibujar() {
  pantalla.clearDisplay();
  textoCentrado("CASA", BJ_Y_TEXTO_CASA, 1);
  textoCentrado("JUGADOR", BJ_Y_TEXTO_JUGADOR, 1);
  pantalla.drawFastHLine(0, BJ_Y_SEPARADOR, OLED_WIDTH, SSD1306_WHITE);

  for (int i = 0; i < bjCantidadCartas; i++) blackjackDibujarCarta(bjCartas[i]);

  if (bjCantidadCartas > 0) {
    String casaTexto = String(blackjackPuntajeCasaVisible());
    String jugadorTexto = String(blackjackPuntajeJugador());
    pantalla.setTextSize(1);
    pantalla.setTextColor(SSD1306_WHITE);
    pantalla.setCursor(112, 1);
    pantalla.print(casaTexto);
    pantalla.setCursor(112, 32);
    pantalla.print(jugadorTexto);
  }

  if (bjEstado == 5) {
    pantalla.fillRect(20, 24, 88, 17, SSD1306_BLACK);
    pantalla.drawRect(20, 24, 88, 17, SSD1306_WHITE);
    if (bjResultado == 1) textoCentrado("GANASTE", 29, 1);
    else if (bjResultado == 2) textoCentrado("PERDISTE", 29, 1);
    else textoCentrado("EMPATE", 29, 1);
  }

  pantalla.display();
}

// ============================================================
// MASCOTA - NO BLOQUEANTE
// Conserva la secuencia del codigo original, pero reemplaza
// todos los delay() por fases basadas en millis().
// ============================================================

const int MA_CENTRO_IZQ = 43;
const int MA_CENTRO_DER = 85;
const int MA_CENTRO_Y = 32;
const int MA_OJO_ANCHO = 25;
const int MA_OJO_ALTO = 31;

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

void mascotaIniciar() {
  mascotaFase = MASCOTA_NORMAL;
  mascotaInicioFase = millis();
}

void mascotaDibujarOjo(float x, float y, float ancho, float alto) {
  pantalla.fillRoundRect((int)(x - ancho / 2), (int)(y - alto / 2),
                         (int)ancho, (int)alto, 8, SSD1306_WHITE);
}

void mascotaDibujarOjoFeliz(float x, float y) {
  pantalla.drawLine((int)x - 9, (int)y + 2, (int)x, (int)y - 4, SSD1306_WHITE);
  pantalla.drawLine((int)x, (int)y - 4, (int)x + 9, (int)y + 2, SSD1306_WHITE);
}

void mascotaDibujarDosOjos(float desplazamiento, float ancho, float alto, float y) {
  pantalla.clearDisplay();
  mascotaDibujarOjo(MA_CENTRO_IZQ + desplazamiento, y, ancho, alto);
  mascotaDibujarOjo(MA_CENTRO_DER + desplazamiento, y, ancho, alto);
}

void mascotaDibujar() {
  unsigned long transcurrido = millis() - mascotaInicioFase;
  float p = (float)transcurrido / (float)mascotaDuracionFase(mascotaFase);
  if (p > 1) p = 1;
  float suave = mascotaSuave(p);

  pantalla.clearDisplay();

  float desplazamiento = 0;
  float ancho = MA_OJO_ANCHO;
  float alto = MA_OJO_ALTO;
  float y = MA_CENTRO_Y;

  if (mascotaFase == MASCOTA_MIRA_IZQ) desplazamiento = -10 * suave;
  else if (mascotaFase == MASCOTA_VUELVE_CENTRO_1) desplazamiento = -10 + 10 * suave;
  else if (mascotaFase == MASCOTA_MIRA_DER) desplazamiento = 10 * suave;
  else if (mascotaFase == MASCOTA_VUELVE_CENTRO_2) desplazamiento = 10 - 10 * suave;
  else if (mascotaFase == MASCOTA_MIRA_POCO_IZQ) desplazamiento = -8 * suave;
  else if (mascotaFase == MASCOTA_MIRA_POCO_CENTRO) desplazamiento = -8 + 8 * suave;

  if (mascotaFase == MASCOTA_PARPADEO_CIERRA || mascotaFase == MASCOTA_PARPADEO_ABRE ||
      mascotaFase == MASCOTA_DESPERTAR_PARPADEO_CIERRA || mascotaFase == MASCOTA_DESPERTAR_PARPADEO_ABRE) {
    float alturaActual = MA_OJO_ALTO;
    if (mascotaFase == MASCOTA_PARPADEO_CIERRA || mascotaFase == MASCOTA_DESPERTAR_PARPADEO_CIERRA) {
      alturaActual = MA_OJO_ALTO * (1.0 - suave);
    } else {
      alturaActual = MA_OJO_ALTO * suave;
    }
    if (alturaActual < 2) alturaActual = 2;
    alto = alturaActual;
  }

  if (mascotaFase == MASCOTA_FELIZ || mascotaFase == MASCOTA_FELIZ_ESPERA) {
    mascotaDibujarOjoFeliz(MA_CENTRO_IZQ, MA_CENTRO_Y);
    mascotaDibujarOjoFeliz(MA_CENTRO_DER, MA_CENTRO_Y);
    pantalla.display();
    return;
  }

  if (mascotaFase == MASCOTA_FELIZ_NORMAL) {
    alto = 6 + (MA_OJO_ALTO - 6) * suave;
    y = 35 - 3 * suave;
  }

  if (mascotaFase == MASCOTA_SORPRESA) {
    ancho = MA_OJO_ANCHO + 7 * suave;
    alto = MA_OJO_ALTO + 8 * suave;
  }

  if (mascotaFase == MASCOTA_SORPRESA_REBOTE) {
    float rebote = sin(p * PI * 3.0) * (1.0 - p) * 2.0;
    y = MA_CENTRO_Y + rebote;
    ancho = MA_OJO_ANCHO + 7;
    alto = MA_OJO_ALTO + 8;
  }

  if (mascotaFase == MASCOTA_SORPRESA_NORMAL) {
    ancho = MA_OJO_ANCHO + 7 - 7 * suave;
    alto = MA_OJO_ALTO + 8 - 8 * suave;
  }

  if (mascotaFase == MASCOTA_GUINO_CIERRA) {
    mascotaDibujarOjo(MA_CENTRO_IZQ, MA_CENTRO_Y, MA_OJO_ANCHO, MA_OJO_ALTO * (1.0 - suave));
    mascotaDibujarOjo(MA_CENTRO_DER, MA_CENTRO_Y, MA_OJO_ANCHO, MA_OJO_ALTO);
    pantalla.display();
    return;
  }

  if (mascotaFase == MASCOTA_GUINO_ESPERA) {
    mascotaDibujarOjo(MA_CENTRO_IZQ, MA_CENTRO_Y, MA_OJO_ANCHO, 3);
    mascotaDibujarOjo(MA_CENTRO_DER, MA_CENTRO_Y, MA_OJO_ANCHO, MA_OJO_ALTO);
    pantalla.display();
    return;
  }

  if (mascotaFase == MASCOTA_GUINO_ABRE) {
    mascotaDibujarOjo(MA_CENTRO_IZQ, MA_CENTRO_Y, MA_OJO_ANCHO, 3 + (MA_OJO_ALTO - 3) * suave);
    mascotaDibujarOjo(MA_CENTRO_DER, MA_CENTRO_Y, MA_OJO_ANCHO, MA_OJO_ALTO);
    pantalla.display();
    return;
  }

  if (mascotaFase == MASCOTA_DORMIR_CIERRA) {
    alto = MA_OJO_ALTO * (1.0 - suave);
    if (alto < 2) alto = 2;
  }

  if (mascotaFase == MASCOTA_DORMIR) {
    pantalla.drawFastHLine(MA_CENTRO_IZQ - 12, MA_CENTRO_Y, 25, SSD1306_WHITE);
    pantalla.drawFastHLine(MA_CENTRO_DER - 12, MA_CENTRO_Y, 25, SSD1306_WHITE);
    pantalla.display();
    return;
  }

  if (mascotaFase == MASCOTA_DESPERTAR) {
    alto = 2 + (MA_OJO_ALTO - 2) * suave;
  }

  mascotaDibujarDosOjos(desplazamiento, ancho, alto, y);
  pantalla.display();
}

void mascotaActualizar() {
  unsigned long ahora = millis();
  if (ahora - mascotaInicioFase >= mascotaDuracionFase(mascotaFase)) {
    mascotaSiguienteFase();
  }
  mascotaDibujar();
}

// ============================================================
// PANTALLAS DE MENU
// ============================================================

void dibujarInicio() {
  pantalla.clearDisplay();
  unsigned long t = millis() - inicioEstado;
  float p = (float)t / 1000.0;
  if (p > 1) p = 1;
  float suave = mascotaSuave(p);
  int y = -20 + (int)(35 * suave);
  textoCentrado("TinyPocket", y, 2);
  if (t >= 900) textoCentrado("Iniciando...", 48, 1);
  pantalla.display();
}

void dibujarHola() {
  pantalla.clearDisplay();
  unsigned long t = millis() - inicioEstado;
  float p = (float)t / 800.0;
  if (p > 1) p = 1;
  int y = 18 + (int)(16 * (1 - mascotaSuave(p)));
  textoCentrado("Hola!", y, 2);
  pantalla.display();
}

void dibujarBotones() {
  pantalla.clearDisplay();
  textoCentrado("CONTROLES", 4, 1);
  pantalla.drawRoundRect(17, 20, 22, 20, 4, SSD1306_WHITE);
  textoCentrado("<", 25, 1);
  pantalla.drawRoundRect(53, 20, 22, 20, 4, SSD1306_WHITE);
  textoCentrado("OK", 25, 1);
  pantalla.drawRoundRect(89, 20, 22, 20, 4, SSD1306_WHITE);
  textoCentrado(">", 25, 1);
  textoCentrado("IZQ / DER: mover", 46, 1);
  textoCentrado("OK: seleccionar", 56, 1);
  pantalla.display();
}

void dibujarMenuPrincipal() {
  const char *opciones[] = {"Ping Pong", "Snake", "Blackjack", "Mascota", "Informacion"};
  pantalla.clearDisplay();
  textoCentrado("TinyPocket", 1, 1);
  pantalla.drawFastHLine(10, 11, 108, SSD1306_WHITE);
  for (int i = 0; i < 5; i++) {
    int y = 17 + i * 9;
    if (i == seleccionMenu) dibujarFlecha(7, y + 1);
    pantalla.setTextSize(1);
    pantalla.setTextColor(SSD1306_WHITE);
    pantalla.setCursor(17, y);
    pantalla.print(opciones[i]);
  }
  pantalla.display();
}

void dibujarMenuPingPong() {
  pantalla.clearDisplay();
  textoCentrado("PING PONG", 3, 1);
  pantalla.drawFastHLine(15, 12, 98, SSD1306_WHITE);
  if (seleccionPingPong == 0) dibujarFlecha(20, 23);
  else dibujarFlecha(20, 37);
  pantalla.setTextSize(1);
  pantalla.setTextColor(SSD1306_WHITE);
  pantalla.setCursor(30, 21); pantalla.print("Normal");
  pantalla.setCursor(30, 35); pantalla.print("Breakout");
  pantalla.display();
}

void dibujarExplicacion() {
  pantalla.clearDisplay();
  if (juegoActual == JUEGO_NORMAL) {
    textoCentrado("Ping Pong", 3, 1);
    textoCentrado("NORMAL", 16, 1);
    textoCentrado("Mueve la paleta", 31, 1);
    textoCentrado("con IZQ / DER", 44, 1);
  } else if (juegoActual == JUEGO_BREAKOUT) {
    textoCentrado("Ping Pong", 3, 1);
    textoCentrado("BREAKOUT", 16, 1);
    textoCentrado("Rompe los bloques", 31, 1);
    textoCentrado("con la pelota", 44, 1);
  } else if (juegoActual == JUEGO_SNAKE) {
    textoCentrado("SNAKE", 8, 1);
    textoCentrado("Come la comida", 25, 1);
    textoCentrado("y crece", 39, 1);
  } else {
    textoCentrado("BLACKJACK", 8, 1);
    textoCentrado("Acercate a 21", 25, 1);
    textoCentrado("sin pasarte", 39, 1);
  }
  pantalla.display();
}

void dibujarInformacion() {
  pantalla.clearDisplay();
  textoCentrado("TinyPocket", 2, 1);
  textoCentrado("Mini consola", 17, 1);
  textoCentrado("OLED 128x64", 29, 1);
  textoCentrado("Ping Pong / Snake", 41, 1);
  textoCentrado("Blackjack / Mascota", 53, 1);
  pantalla.display();
}

void dibujarResultado() {
  pantalla.clearDisplay();
  if (juegoActual == JUEGO_NORMAL) textoCentrado("Ronda terminada", 24, 1);
  else if (juegoActual == JUEGO_BREAKOUT) textoCentrado("Breakout listo", 24, 1);
  else if (juegoActual == JUEGO_SNAKE) textoCentrado("Snake terminado", 24, 1);
  else textoCentrado("Blackjack terminado", 24, 1);
  pantalla.display();
}

// ============================================================
// FLUJO DE NAVEGACION AUTOMATICO
// ============================================================

void prepararSiguienteDespuesDelJuego() {
  if (juegoActual == JUEGO_NORMAL) {
    if (rondaActual < 2) {
      rondaActual++;
      cambiarEstado(ESTADO_JUEGO);
    } else {
      seleccionPingPong = 1;
      cambiarEstado(ESTADO_MENU_PINGPONG);
    }
  } else if (juegoActual == JUEGO_BREAKOUT) {
    seleccionMenu = 1;
    cambiarEstado(ESTADO_MENU_PRINCIPAL);
  } else if (juegoActual == JUEGO_SNAKE) {
    if (rondaActual < 2) {
      rondaActual++;
      cambiarEstado(ESTADO_JUEGO);
    } else {
      seleccionMenu = 2;
      cambiarEstado(ESTADO_MENU_PRINCIPAL);
    }
  } else if (juegoActual == JUEGO_BLACKJACK) {
    if (rondaActual < 2) {
      rondaActual++;
      cambiarEstado(ESTADO_JUEGO);
    } else {
      seleccionMenu = 3;
      cambiarEstado(ESTADO_MENU_PRINCIPAL);
    }
  }
}

void actualizarEstado() {
  switch (estadoActual) {
    case ESTADO_INICIO:
      if (pasoTiempo(TIEMPO_INICIO)) cambiarEstado(ESTADO_HOLA);
      break;

    case ESTADO_HOLA:
      if (pasoTiempo(TIEMPO_HOLA)) cambiarEstado(ESTADO_BOTONES);
      break;

    case ESTADO_BOTONES:
      if (pasoTiempo(TIEMPO_BOTONES)) cambiarEstado(ESTADO_MENU_PRINCIPAL);
      break;

    case ESTADO_MENU_PRINCIPAL:
      if (pasoTiempo(TIEMPO_OPCION_MENU)) {
        if (seleccionMenu == 0) {
          juegoActual = JUEGO_NORMAL;
          rondaActual = 1;
          cambiarEstado(ESTADO_MENU_PINGPONG);
        } else if (seleccionMenu == 1) {
          juegoActual = JUEGO_SNAKE;
          rondaActual = 1;
          cambiarEstado(ESTADO_EXPLICACION);
        } else if (seleccionMenu == 2) {
          juegoActual = JUEGO_BLACKJACK;
          rondaActual = 1;
          cambiarEstado(ESTADO_EXPLICACION);
        } else if (seleccionMenu == 3) {
          cambiarEstado(ESTADO_MASCOTA);
        } else {
          cambiarEstado(ESTADO_INFORMACION);
        }
      }
      break;

    case ESTADO_MENU_PINGPONG:
      if (pasoTiempo(TIEMPO_OPCION_PINGPONG)) {
        if (seleccionPingPong == 0) juegoActual = JUEGO_NORMAL;
        else juegoActual = JUEGO_BREAKOUT;
        rondaActual = 1;
        cambiarEstado(ESTADO_EXPLICACION);
      }
      break;

    case ESTADO_EXPLICACION:
      if (pasoTiempo(TIEMPO_EXPLICACION)) cambiarEstado(ESTADO_JUEGO);
      break;

    case ESTADO_JUEGO:
      if (juegoActual == JUEGO_NORMAL) {
        pingpongActualizarPaleta();
        pingpongActualizarPelota();
        if (pasoTiempo(6000)) {
          cambiarEstado(ESTADO_RESULTADO);
        }
      } else if (juegoActual == JUEGO_BREAKOUT) {
        breakoutActualizarPaleta();
        breakoutActualizarPelota();
        if (pasoTiempo(6000)) {
          cambiarEstado(ESTADO_RESULTADO);
        }
      } else if (juegoActual == JUEGO_SNAKE) {
        snakeActualizar();
        if (pasoTiempo(7000)) {
          cambiarEstado(ESTADO_RESULTADO);
        }
      } else {
        blackjackActualizar();
        if (bjEstado == 5 && millis() - bjInicioResultado >= BJ_TIEMPO_RESULTADO) {
          cambiarEstado(ESTADO_RESULTADO);
        }
        if (pasoTiempo(12000) && bjEstado != 5) {
          bjResultado = 3;
          bjEstado = 5;
          bjInicioResultado = millis();
        }
      }
      break;

    case ESTADO_RESULTADO:
      if (pasoTiempo(TIEMPO_RESULTADO)) prepararSiguienteDespuesDelJuego();
      break;

    case ESTADO_MASCOTA:
      mascotaActualizar();
      if (pasoTiempo(14000)) {
        seleccionMenu = 4;
        cambiarEstado(ESTADO_MENU_PRINCIPAL);
      }
      break;

    case ESTADO_INFORMACION:
      if (pasoTiempo(TIEMPO_INFORMACION)) {
        seleccionMenu = 0;
        cambiarEstado(ESTADO_MENU_PRINCIPAL);
      }
      break;
  }
}

void dibujarEstado() {
  switch (estadoActual) {
    case ESTADO_INICIO: dibujarInicio(); break;
    case ESTADO_HOLA: dibujarHola(); break;
    case ESTADO_BOTONES: dibujarBotones(); break;
    case ESTADO_MENU_PRINCIPAL: dibujarMenuPrincipal(); break;
    case ESTADO_MENU_PINGPONG: dibujarMenuPingPong(); break;
    case ESTADO_EXPLICACION: dibujarExplicacion(); break;
    case ESTADO_JUEGO:
      if (juegoActual == JUEGO_NORMAL) pingpongDibujar();
      else if (juegoActual == JUEGO_BREAKOUT) breakoutDibujar();
      else if (juegoActual == JUEGO_SNAKE) snakeDibujar();
      else blackjackDibujar();
      break;
    case ESTADO_RESULTADO: dibujarResultado(); break;
    case ESTADO_MASCOTA: break;
    case ESTADO_INFORMACION: dibujarInformacion(); break;
  }
}

// ============================================================
// SETUP / LOOP
// ============================================================

void setup() {
  Wire.begin();
  Wire.setClock(400000);

  if (!pantalla.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    while (true) {
      // Si el OLED no responde, queda detenido aqui.
    }
  }

  randomSeed(analogRead(A0) + micros());
  pantalla.clearDisplay();
  pantalla.display();

  inicioEstado = millis();
  mascotaIniciar();
  pingpongReiniciar();
  breakoutReiniciar();
  snakeReiniciar();
  blackjackIniciar();
}

void loop() {
  unsigned long ahora = millis();
  if (ahora - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = ahora;

  actualizarEstado();

  if (estadoActual != ESTADO_MASCOTA && estadoActual != ESTADO_JUEGO) {
    dibujarEstado();
  }
}
