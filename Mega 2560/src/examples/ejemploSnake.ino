#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// TINYPocket - SNAKE OLED
// 128x64 - I2C 0x3C
// IA automatica tipo jugador
// ============================================================

#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 pantalla(OLED_WIDTH, OLED_HEIGHT, &Wire, -1);

// ============================================================
// CONFIGURACION
// ============================================================

#define MAX_SEGMENTOS 40

const unsigned long FRAME_TIME = 20;

const float VELOCIDAD = 1.55;

const float DISTANCIA_COMIDA = 8.0;

const float LIMITE_IZQUIERDO = 4.0;
const float LIMITE_DERECHO = 124.0;
const float LIMITE_SUPERIOR = 4.0;
const float LIMITE_INFERIOR = 60.0;

// ============================================================
// SERPIENTE
// ============================================================

float snakeX[MAX_SEGMENTOS];
float snakeY[MAX_SEGMENTOS];

uint8_t snakeLongitud = 8;

// Direccion actual
int direccionX = 0;
int direccionY = -1;

// ============================================================
// COMIDA
// ============================================================

float comidaX;
float comidaY;

bool comidaVisible = true;

// ============================================================
// TIEMPOS
// ============================================================

unsigned long ultimoFrame = 0;
unsigned long ultimoPensamiento = 0;

const unsigned long TIEMPO_PENSAMIENTO = 80;

// ============================================================
// DISTANCIA
// ============================================================

float distanciaEntre(
  float x1,
  float y1,
  float x2,
  float y2
) {

  float dx = x2 - x1;
  float dy = y2 - y1;

  return sqrt(
    dx * dx +
    dy * dy
  );
}

// ============================================================
// GENERAR COMIDA
// ============================================================

void generarComida() {

  bool valida = false;

  while (!valida) {

    // La comida aparece en una cuadricula.
    // Esto hace que sea mucho mas facil alcanzarla.
    comidaX = random(12, 116);
    comidaY = random(8, 56);

    comidaX = ((int)comidaX / 4) * 4;
    comidaY = ((int)comidaY / 4) * 4;

    valida = true;

    // Evitar que aparezca dentro del cuerpo
    for (int i = 0; i < snakeLongitud; i++) {

      if (
        distanciaEntre(
          comidaX,
          comidaY,
          snakeX[i],
          snakeY[i]
        ) < 8
      ) {

        valida = false;
        break;
      }
    }
  }
}

// ============================================================
// PELIGRO INMEDIATO
// ============================================================

bool posicionPeligrosa(
  float x,
  float y
) {

  // ----------------------------------------------------------
  // PARED
  // ----------------------------------------------------------

  if (x <= LIMITE_IZQUIERDO) {
    return true;
  }

  if (x >= LIMITE_DERECHO) {
    return true;
  }

  if (y <= LIMITE_SUPERIOR) {
    return true;
  }

  if (y >= LIMITE_INFERIOR) {
    return true;
  }

  // ----------------------------------------------------------
  // CUERPO
  // ----------------------------------------------------------

  // No miramos toda la serpiente con el mismo peso.
  //
  // La cola esta avanzando y puede desaparecer del lugar,
  // por eso los ultimos segmentos son menos importantes.

  for (int i = 4; i < snakeLongitud - 2; i++) {

    float distanciaCuerpo =
      distanciaEntre(
        x,
        y,
        snakeX[i],
        snakeY[i]
      );

    if (distanciaCuerpo < 5.5) {
      return true;
    }
  }

  return false;
}

// ============================================================
// COMPROBAR UNA DIRECCION
// ============================================================

bool direccionEsSegura(
  int nuevaDireccionX,
  int nuevaDireccionY
) {

  float cabezaX = snakeX[0];
  float cabezaY = snakeY[0];

  // Miramos solamente lo suficiente para evitar
  // chocarnos inmediatamente.
  for (int distanciaMira = 5; distanciaMira <= 15; distanciaMira += 5) {

    float pruebaX =
      cabezaX +
      nuevaDireccionX *
      distanciaMira;

    float pruebaY =
      cabezaY +
      nuevaDireccionY *
      distanciaMira;

    if (
      posicionPeligrosa(
        pruebaX,
        pruebaY
      )
    ) {

      return false;
    }
  }

  return true;
}

// ============================================================
// CALCULAR QUE TAN BUENA ES UNA DIRECCION
// ============================================================

float evaluarDireccion(
  int nuevaDireccionX,
  int nuevaDireccionY
) {

  float cabezaX = snakeX[0];
  float cabezaY = snakeY[0];

  // ----------------------------------------------------------
  // POSICION FUTURA
  // ----------------------------------------------------------

  float futuroX =
    cabezaX +
    nuevaDireccionX *
    25;

  float futuroY =
    cabezaY +
    nuevaDireccionY *
    25;

  // ----------------------------------------------------------
  // DISTANCIA A LA COMIDA
  // ----------------------------------------------------------

  float distanciaActual =
    distanciaEntre(
      cabezaX,
      cabezaY,
      comidaX,
      comidaY
    );

  float distanciaFutura =
    distanciaEntre(
      futuroX,
      futuroY,
      comidaX,
      comidaY
    );

  float puntaje = 0;

  // ==========================================================
  // PRIORIDAD PRINCIPAL: COMIDA
  // ==========================================================

  // Cuanto mas cerca quede la comida,
  // mejor es la direccion.
  puntaje += distanciaFutura * 2.5;

  // Si realmente nos acercamos, damos un bonus enorme.
  if (
    distanciaFutura <
    distanciaActual
  ) {

    puntaje -= 45;
  }

  // Si nos alejamos, penalizamos.
  else {

    puntaje += 25;
  }

  // ==========================================================
  // SI LA COMIDA ESTA A UN COSTADO
  // ==========================================================

  float diferenciaX =
    abs(comidaX - cabezaX);

  float diferenciaY =
    abs(comidaY - cabezaY);

  // Si la comida esta claramente a la derecha
  if (
    comidaX > cabezaX + 5 &&
    diferenciaX > diferenciaY
  ) {

    if (nuevaDireccionX == 1) {
      puntaje -= 70;
    }
  }

  // Si la comida esta claramente a la izquierda
  if (
    comidaX < cabezaX - 5 &&
    diferenciaX > diferenciaY
  ) {

    if (nuevaDireccionX == -1) {
      puntaje -= 70;
    }
  }

  // Si la comida esta claramente abajo
  if (
    comidaY > cabezaY + 5 &&
    diferenciaY > diferenciaX
  ) {

    if (nuevaDireccionY == 1) {
      puntaje -= 70;
    }
  }

  // Si la comida esta claramente arriba
  if (
    comidaY < cabezaY - 5 &&
    diferenciaY > diferenciaX
  ) {

    if (nuevaDireccionY == -1) {
      puntaje -= 70;
    }
  }

  // ==========================================================
  // SEGURIDAD
  // ==========================================================

  if (
    !direccionEsSegura(
      nuevaDireccionX,
      nuevaDireccionY
    )
  ) {

    // Sigue pudiendo elegirlo si es necesario,
    // pero es MUY mala opcion.
    puntaje += 1000;
  }

  // ==========================================================
  // NO DAR MEDIA VUELTA
  // ==========================================================

  if (
    nuevaDireccionX == -direccionX &&
    nuevaDireccionY == -direccionY
  ) {

    puntaje += 500;
  }

  // ==========================================================
  // PEQUENA IMPERFECCION
  // ==========================================================

  puntaje += random(0, 6);

  return puntaje;
}

// ============================================================
// DECIDIR MOVIMIENTO
// ============================================================

void pensarMovimiento() {

  int posiblesX[4] = {
    1,
    -1,
    0,
    0
  };

  int posiblesY[4] = {
    0,
    0,
    1,
    -1
  };

  float mejorPuntaje = 999999;
  int mejorOpcion = -1;

  // ----------------------------------------------------------
  // EVALUAR LAS 4 DIRECCIONES
  // ----------------------------------------------------------

  for (int i = 0; i < 4; i++) {

    int nuevaX = posiblesX[i];
    int nuevaY = posiblesY[i];

    // Nunca girar directamente hacia atras.
    if (
      nuevaX == -direccionX &&
      nuevaY == -direccionY
    ) {

      continue;
    }

    float puntaje =
      evaluarDireccion(
        nuevaX,
        nuevaY
      );

    if (
      puntaje <
      mejorPuntaje
    ) {

      mejorPuntaje = puntaje;
      mejorOpcion = i;
    }
  }

  // ----------------------------------------------------------
  // APLICAR DIRECCION
  // ----------------------------------------------------------

  if (mejorOpcion >= 0) {

    direccionX =
      posiblesX[mejorOpcion];

    direccionY =
      posiblesY[mejorOpcion];
  }
}

// ============================================================
// COMPROBAR COLISION CON EL CUERPO
// ============================================================

bool chocoConSuCuerpo() {

  // Empezamos desde el segmento 4.
  // Los primeros segmentos estan demasiado cerca de la cabeza.
  for (int i = 4; i < snakeLongitud; i++) {

    float d =
      distanciaEntre(
        snakeX[0],
        snakeY[0],
        snakeX[i],
        snakeY[i]
      );

    if (d < 4.0) {
      return true;
    }
  }

  return false;
}

// ============================================================
// COMPROBAR COMIDA
// ============================================================

bool estaComiendo() {

  float d =
    distanciaEntre(
      snakeX[0],
      snakeY[0],
      comidaX,
      comidaY
    );

  return d < DISTANCIA_COMIDA;
}

// ============================================================
// REINICIAR
// ============================================================

void reiniciarJuego() {

  snakeLongitud = 8;

  float inicioX = 64;
  float inicioY = 40;

  // Cuerpo horizontal
  for (int i = 0; i < snakeLongitud; i++) {

    snakeX[i] =
      inicioX -
      i * 4;

    snakeY[i] =
      inicioY;
  }

  // IMPORTANTE:
  // Arrancar hacia arriba.
  direccionX = 0;
  direccionY = -1;

  generarComida();

  ultimoPensamiento = 0;
}

// ============================================================
// ACTUALIZAR SERPIENTE
// ============================================================

void actualizarSerpiente() {

  // ----------------------------------------------------------
  // MOVER CUERPO
  // ----------------------------------------------------------

  for (
    int i = snakeLongitud - 1;
    i > 0;
    i--
  ) {

    snakeX[i] =
      snakeX[i - 1];

    snakeY[i] =
      snakeY[i - 1];
  }

  // ----------------------------------------------------------
  // MOVER CABEZA
  // ----------------------------------------------------------

  snakeX[0] +=
    direccionX *
    VELOCIDAD;

  snakeY[0] +=
    direccionY *
    VELOCIDAD;

  // ----------------------------------------------------------
  // PARED
  // ----------------------------------------------------------

  if (
    snakeX[0] <= 1 ||
    snakeX[0] >= 127 ||
    snakeY[0] <= 1 ||
    snakeY[0] >= 63
  ) {

    reiniciarJuego();
    return;
  }

  // ----------------------------------------------------------
  // CUERPO
  // ----------------------------------------------------------

  if (chocoConSuCuerpo()) {

    reiniciarJuego();
    return;
  }

  // ----------------------------------------------------------
  // COMIDA
  // ----------------------------------------------------------

  if (
    comidaVisible &&
    estaComiendo()
  ) {

    if (
      snakeLongitud <
      MAX_SEGMENTOS
    ) {

      snakeX[snakeLongitud] =
        snakeX[snakeLongitud - 1];

      snakeY[snakeLongitud] =
        snakeY[snakeLongitud - 1];

      snakeLongitud++;
    }

    generarComida();
  }
}

// ============================================================
// DIBUJAR SERPIENTE
// ============================================================

void dibujarSerpiente() {

  for (
    int i = snakeLongitud - 1;
    i >= 0;
    i--
  ) {

    int x =
      (int)snakeX[i];

    int y =
      (int)snakeY[i];

    // --------------------------------------------------------
    // CABEZA
    // --------------------------------------------------------

    if (i == 0) {

      pantalla.fillCircle(
        x,
        y,
        3,
        SSD1306_WHITE
      );

      // Ojos
      int ojo1X = x;
      int ojo1Y = y;

      int ojo2X = x;
      int ojo2Y = y;

      if (direccionX > 0) {

        ojo1X += 1;
        ojo1Y -= 1;

        ojo2X += 1;
        ojo2Y += 1;
      }

      else if (direccionX < 0) {

        ojo1X -= 1;
        ojo1Y -= 1;

        ojo2X -= 1;
        ojo2Y += 1;
      }

      else if (direccionY < 0) {

        ojo1X -= 1;
        ojo1Y -= 1;

        ojo2X += 1;
        ojo2Y -= 1;
      }

      else {

        ojo1X -= 1;
        ojo1Y += 1;

        ojo2X += 1;
        ojo2Y += 1;
      }

      pantalla.drawPixel(
        ojo1X,
        ojo1Y,
        SSD1306_BLACK
      );

      pantalla.drawPixel(
        ojo2X,
        ojo2Y,
        SSD1306_BLACK
      );
    }

    // --------------------------------------------------------
    // CUERPO
    // --------------------------------------------------------

    else {

      pantalla.fillCircle(
        x,
        y,
        2,
        SSD1306_WHITE
      );
    }
  }
}

// ============================================================
// DIBUJAR COMIDA
// ============================================================

void dibujarComida() {

  if (!comidaVisible) {
    return;
  }

  int x =
    (int)comidaX;

  int y =
    (int)comidaY;

  // Animacion suave
  int radio = 2;

  if (
    ((millis() / 180) % 2) == 0
  ) {

    radio = 3;
  }

  pantalla.fillCircle(
    x,
    y,
    radio,
    SSD1306_WHITE
  );

  pantalla.drawPixel(
    x,
    y - radio - 1,
    SSD1306_WHITE
  );
}

// ============================================================
// BORDE
// ============================================================

void dibujarBorde() {

  pantalla.drawRect(
    0,
    0,
    OLED_WIDTH,
    OLED_HEIGHT,
    SSD1306_WHITE
  );
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  Wire.begin();

  Wire.setClock(400000);

  if (
    !pantalla.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    while (true) {
    }
  }

  pantalla.clearDisplay();
  pantalla.display();

  randomSeed(
    analogRead(A0) +
    micros()
  );

  reiniciarJuego();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long ahora =
    millis();

  // ----------------------------------------------------------
  // FRAME
  // ----------------------------------------------------------

  if (
    ahora - ultimoFrame <
    FRAME_TIME
  ) {

    return;
  }

  ultimoFrame =
    ahora;

  // ----------------------------------------------------------
  // IA
  // ----------------------------------------------------------

  if (
    ultimoPensamiento == 0 ||
    ahora - ultimoPensamiento >=
    TIEMPO_PENSAMIENTO
  ) {

    pensarMovimiento();

    ultimoPensamiento =
      ahora;
  }

  // ----------------------------------------------------------
  // ACTUALIZAR
  // ----------------------------------------------------------

  actualizarSerpiente();

  // ----------------------------------------------------------
  // DIBUJAR
  // ----------------------------------------------------------

  pantalla.clearDisplay();

  dibujarBorde();

  dibujarComida();

  dibujarSerpiente();

  pantalla.display();
}