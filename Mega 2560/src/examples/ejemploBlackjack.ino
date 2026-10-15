#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================================
// BLACKJACK - TinyPocket OLED 128x64
// ============================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 pantalla(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// ============================================================
// TIEMPOS
// ============================================================

const unsigned long FRAME_TIME = 16;

// Tiempo entre cada carta repartida
const unsigned long TIEMPO_ENTRE_CARTAS = 900;

// Tiempo que se muestra el resultado
const unsigned long TIEMPO_RESULTADO = 3000;

// Tiempo de espera entre terminar la ronda
// y mostrar el cartel de resultado
const unsigned long TIEMPO_ANTES_RESULTADO = 2000;

// ============================================================
// CARTAS
// ============================================================

const int MAX_CARTAS = 10;
const int MAX_CARTAS_JUGADOR = 5;
const int MAX_CARTAS_CASA = 5;

struct Carta {

  byte palo;
  byte valor;
};

Carta mazo[52];

int cartasEnMazo = 52;

// ============================================================
// CARTAS ANIMADAS
// ============================================================

struct CartaAnimada {

  Carta carta;

  float x;
  float y;

  float destinoX;
  float destinoY;

  bool jugador;
  bool oculta;
};

CartaAnimada cartas[MAX_CARTAS];

int cantidadCartas = 0;

int cartasJugador = 0;
int cartasCasa = 0;

// ============================================================
// ESTADOS DEL JUEGO
// ============================================================

enum EstadoJuego {

  ESTADO_REPARTO_INICIAL,
  ESTADO_JUGADOR,
  ESTADO_REVELAR_CASA,
  ESTADO_CASA,
  ESTADO_ESPERANDO_RESULTADO,
  ESTADO_RESULTADO
};

EstadoJuego estadoJuego =
  ESTADO_REPARTO_INICIAL;

// ============================================================
// PASOS DEL REPARTO INICIAL
// ============================================================

enum PasoInicial {

  INICIO_JUGADOR_1,
  INICIO_JUGADOR_2,
  INICIO_CASA_VISIBLE,
  INICIO_CASA_OCULTA,
  INICIO_TERMINADO
};

PasoInicial pasoInicial =
  INICIO_JUGADOR_1;

// ============================================================
// RESULTADOS
// ============================================================

const int RESULTADO_JUGADOR = 1;
const int RESULTADO_CASA = 2;
const int RESULTADO_EMPATE = 3;

int resultado = 0;

// ============================================================
// TIEMPOS
// ============================================================

unsigned long ultimoFrame = 0;

unsigned long ultimaCarta = 0;

unsigned long inicioResultado = 0;

unsigned long inicioEsperaResultado = 0;

// ============================================================
// DIMENSIONES DE CARTAS
// ============================================================

const int ANCHO_CARTA = 17;
const int ALTO_CARTA = 18;

const int ESPACIO_CARTAS = 3;

// ============================================================
// POSICIONES
// ============================================================

const int Y_TEXTO_CASA = 0;

const int Y_CARTAS_CASA = 8;

const int Y_SEPARADOR = 29;

const int Y_TEXTO_JUGADOR = 32;

const int Y_CARTAS_JUGADOR = 41;

// ============================================================
// ANIMACION
// ============================================================

const float VELOCIDAD_ANIMACION = 0.18;

// ============================================================
// CREAR MAZO
// ============================================================

void crearMazo() {

  int posicion = 0;

  // Cuatro palos
  for (byte palo = 0; palo < 4; palo++) {

    // Trece cartas por palo
    for (byte valor = 1; valor <= 13; valor++) {

      mazo[posicion].palo = palo;

      mazo[posicion].valor = valor;

      posicion++;
    }
  }

  cartasEnMazo = 52;
}

// ============================================================
// MEZCLAR MAZO
// ============================================================

void mezclarMazo() {

  for (
    int i = cartasEnMazo - 1;
    i > 0;
    i--
  ) {

    int posicionAleatoria =
      random(0, i + 1);

    Carta temporal =
      mazo[i];

    mazo[i] =
      mazo[posicionAleatoria];

    mazo[posicionAleatoria] =
      temporal;
  }
}

// ============================================================
// SACAR CARTA
// ============================================================

Carta sacarCarta() {

  // Si no quedan cartas,
  // crear y mezclar otro mazo
  if (cartasEnMazo <= 0) {

    crearMazo();

    mezclarMazo();
  }

  cartasEnMazo--;

  return mazo[cartasEnMazo];
}

// ============================================================
// VALOR DE CARTA
// ============================================================

int valorCarta(Carta carta) {

  // As
  if (carta.valor == 1) {

    return 11;
  }

  // 10, J, Q, K
  if (carta.valor >= 10) {

    return 10;
  }

  // 2 a 9
  return carta.valor;
}

// ============================================================
// CALCULAR PUNTAJE
// ============================================================

int calcularPuntaje(
  bool jugador,
  bool incluirOculta
) {

  int suma = 0;

  int cantidadAses = 0;

  for (
    int i = 0;
    i < cantidadCartas;
    i++
  ) {

    // Ignorar cartas del otro jugador
    if (
      cartas[i].jugador != jugador
    ) {

      continue;
    }

    // No contar la carta oculta de la casa
    if (
      !jugador &&
      cartas[i].oculta &&
      !incluirOculta
    ) {

      continue;
    }

    int valor =
      valorCarta(
        cartas[i].carta
      );

    suma += valor;

    if (
      cartas[i].carta.valor == 1
    ) {

      cantidadAses++;
    }
  }

  // Convertir As de 11 a 1
  // cuando el puntaje supera 21
  while (
    suma > 21 &&
    cantidadAses > 0
  ) {

    suma -= 10;

    cantidadAses--;
  }

  return suma;
}

// ============================================================
// PUNTAJE DEL JUGADOR
// ============================================================

int calcularPuntajeJugador() {

  return calcularPuntaje(
    true,
    true
  );
}

// ============================================================
// PUNTAJE VISIBLE DE LA CASA
// ============================================================

int calcularPuntajeCasaVisible() {

  return calcularPuntaje(
    false,
    false
  );
}

// ============================================================
// PUNTAJE REAL DE LA CASA
// ============================================================

int calcularPuntajeCasaReal() {

  return calcularPuntaje(
    false,
    true
  );
}

// ============================================================
// ANCHO DEL GRUPO DE CARTAS
// ============================================================

int calcularAnchoGrupo(
  int cantidad
) {

  if (cantidad <= 0) {

    return 0;
  }

  return
    cantidad * ANCHO_CARTA +
    (cantidad - 1) *
    ESPACIO_CARTAS;
}

// ============================================================
// ACTUALIZAR POSICIONES
// ============================================================

void actualizarPosicionesCartas() {

  int anchoJugador =
    calcularAnchoGrupo(
      cartasJugador
    );

  int anchoCasa =
    calcularAnchoGrupo(
      cartasCasa
    );

  int inicioJugador =
    (SCREEN_WIDTH -
     anchoJugador) / 2;

  int inicioCasa =
    (SCREEN_WIDTH -
     anchoCasa) / 2;

  int indiceJugador = 0;

  int indiceCasa = 0;

  for (
    int i = 0;
    i < cantidadCartas;
    i++
  ) {

    if (cartas[i].jugador) {

      cartas[i].destinoX =
        inicioJugador +
        indiceJugador *
        (
          ANCHO_CARTA +
          ESPACIO_CARTAS
        );

      cartas[i].destinoY =
        Y_CARTAS_JUGADOR;

      indiceJugador++;

    } else {

      cartas[i].destinoX =
        inicioCasa +
        indiceCasa *
        (
          ANCHO_CARTA +
          ESPACIO_CARTAS
        );

      cartas[i].destinoY =
        Y_CARTAS_CASA;

      indiceCasa++;
    }
  }
}

// ============================================================
// DIBUJAR CORAZON
// ============================================================

void dibujarCorazon(
  int x,
  int y
) {

  pantalla.fillCircle(
    x + 4,
    y + 4,
    3,
    SSD1306_BLACK
  );

  pantalla.fillCircle(
    x + 9,
    y + 4,
    3,
    SSD1306_BLACK
  );

  pantalla.fillTriangle(
    x + 1,
    y + 5,
    x + 12,
    y + 5,
    x + 6,
    y + 12,
    SSD1306_BLACK
  );
}

// ============================================================
// DIBUJAR DIAMANTE
// ============================================================

void dibujarDiamante(
  int x,
  int y
) {

  pantalla.fillTriangle(
    x + 6,
    y,
    x + 12,
    y + 6,
    x + 6,
    y + 12,
    SSD1306_BLACK
  );

  pantalla.fillTriangle(
    x + 6,
    y,
    x,
    y + 6,
    x + 6,
    y + 12,
    SSD1306_BLACK
  );
}

// ============================================================
// DIBUJAR TREBOL
// ============================================================

void dibujarTrebol(
  int x,
  int y
) {

  pantalla.fillCircle(
    x + 3,
    y + 4,
    3,
    SSD1306_BLACK
  );

  pantalla.fillCircle(
    x + 9,
    y + 4,
    3,
    SSD1306_BLACK
  );

  pantalla.fillCircle(
    x + 6,
    y + 8,
    3,
    SSD1306_BLACK
  );

  pantalla.fillTriangle(
    x + 6,
    y + 7,
    x + 3,
    y + 13,
    x + 9,
    y + 13,
    SSD1306_BLACK
  );
}

// ============================================================
// DIBUJAR PICAS
// ============================================================

void dibujarPicas(
  int x,
  int y
) {

  pantalla.fillTriangle(
    x + 6,
    y,
    x,
    y + 8,
    x + 12,
    y + 8,
    SSD1306_BLACK
  );

  pantalla.fillCircle(
    x + 3,
    y + 8,
    3,
    SSD1306_BLACK
  );

  pantalla.fillCircle(
    x + 9,
    y + 8,
    3,
    SSD1306_BLACK
  );

  pantalla.fillRect(
    x + 5,
    y + 7,
    3,
    6,
    SSD1306_BLACK
  );
}

// ============================================================
// DIBUJAR VALOR
// ============================================================

void dibujarValor(
  int x,
  int y,
  byte valor
) {

  if (valor == 1) {

    pantalla.setCursor(
      x,
      y
    );

    pantalla.print("A");

  } else if (
    valor >= 2 &&
    valor <= 9
  ) {

    pantalla.setCursor(
      x,
      y
    );

    pantalla.print(
      valor
    );

  } else if (valor == 10) {

    pantalla.setCursor(
      x,
      y
    );

    pantalla.print("10");

  } else if (valor == 11) {

    pantalla.setCursor(
      x,
      y
    );

    pantalla.print("J");

  } else if (valor == 12) {

    pantalla.setCursor(
      x,
      y
    );

    pantalla.print("Q");

  } else if (valor == 13) {

    pantalla.setCursor(
      x,
      y
    );

    pantalla.print("K");
  }
}

// ============================================================
// CARTA OCULTA
// ============================================================

void dibujarCartaOculta(
  int x,
  int y
) {

  pantalla.fillRoundRect(
    x,
    y,
    ANCHO_CARTA,
    ALTO_CARTA,
    2,
    SSD1306_WHITE
  );
}

// ============================================================
// DIBUJAR CARTA
// ============================================================

void dibujarCarta(
  CartaAnimada &carta
) {

  int x =
    (int)carta.x;

  int y =
    (int)carta.y;

  // ----------------------------------------------------------
  // CARTA OCULTA
  // ----------------------------------------------------------

  if (carta.oculta) {

    dibujarCartaOculta(
      x,
      y
    );

    return;
  }

  // ----------------------------------------------------------
  // FONDO
  // ----------------------------------------------------------

  pantalla.fillRoundRect(
    x,
    y,
    ANCHO_CARTA,
    ALTO_CARTA,
    2,
    SSD1306_WHITE
  );

  // ----------------------------------------------------------
  // TEXTO
  // ----------------------------------------------------------

  pantalla.setTextColor(
    SSD1306_BLACK
  );

  pantalla.setTextSize(1);

  // Valor
  dibujarValor(
    x + 2,
    y + 2,
    carta.carta.valor
  );

  // ----------------------------------------------------------
  // PALO
  // ----------------------------------------------------------

  int paloX =
    x + 2;

  int paloY =
    y + 8;

  if (
    carta.carta.palo == 0
  ) {

    dibujarCorazon(
      paloX,
      paloY
    );

  } else if (
    carta.carta.palo == 1
  ) {

    dibujarDiamante(
      paloX,
      paloY
    );

  } else if (
    carta.carta.palo == 2
  ) {

    dibujarTrebol(
      paloX,
      paloY
    );

  } else {

    dibujarPicas(
      paloX,
      paloY
    );
  }
}

// ============================================================
// AGREGAR CARTA
// ============================================================

void agregarCarta(
  bool jugador,
  bool oculta
) {

  if (
    cantidadCartas >= MAX_CARTAS
  ) {

    return;
  }

  if (
    jugador &&
    cartasJugador >=
    MAX_CARTAS_JUGADOR
  ) {

    return;
  }

  if (
    !jugador &&
    cartasCasa >=
    MAX_CARTAS_CASA
  ) {

    return;
  }

  Carta nuevaCarta =
    sacarCarta();

  cartas[cantidadCartas].carta =
    nuevaCarta;

  cartas[cantidadCartas].jugador =
    jugador;

  cartas[cantidadCartas].oculta =
    oculta;

  // ----------------------------------------------------------
  // POSICION INICIAL DE LA CARTA
  // ----------------------------------------------------------

  if (
    (jugador &&
     cartasJugador % 2 == 0) ||
    (!jugador &&
     cartasCasa % 2 == 0)
  ) {

    cartas[cantidadCartas].x =
      -20;

  } else {

    cartas[cantidadCartas].x =
      SCREEN_WIDTH + 20;
  }

  // ----------------------------------------------------------
  // ALTURA
  // ----------------------------------------------------------

  if (jugador) {

    cartas[cantidadCartas].y =
      Y_CARTAS_JUGADOR;

  } else {

    cartas[cantidadCartas].y =
      Y_CARTAS_CASA;
  }

  cartas[cantidadCartas].destinoX =
    cartas[cantidadCartas].x;

  cartas[cantidadCartas].destinoY =
    cartas[cantidadCartas].y;

  cantidadCartas++;

  if (jugador) {

    cartasJugador++;

  } else {

    cartasCasa++;
  }

  actualizarPosicionesCartas();
}

// ============================================================
// REVELAR CARTA DE LA CASA
// ============================================================

void revelarCartaCasa() {

  for (
    int i = 0;
    i < cantidadCartas;
    i++
  ) {

    if (
      !cartas[i].jugador &&
      cartas[i].oculta
    ) {

      cartas[i].oculta =
        false;

      break;
    }
  }
}

// ============================================================
// NUEVA PARTIDA
// ============================================================

void nuevaPartida() {

  cantidadCartas = 0;

  cartasJugador = 0;

  cartasCasa = 0;

  resultado = 0;

  crearMazo();

  mezclarMazo();

  estadoJuego =
    ESTADO_REPARTO_INICIAL;

  pasoInicial =
    INICIO_JUGADOR_1;

  ultimaCarta =
    millis();
}

// ============================================================
// ACTUALIZAR ANIMACIONES
// ============================================================

void actualizarAnimaciones() {

  for (
    int i = 0;
    i < cantidadCartas;
    i++
  ) {

    float diferenciaX =
      cartas[i].destinoX -
      cartas[i].x;

    float diferenciaY =
      cartas[i].destinoY -
      cartas[i].y;

    cartas[i].x +=
      diferenciaX *
      VELOCIDAD_ANIMACION;

    cartas[i].y +=
      diferenciaY *
      VELOCIDAD_ANIMACION;

    // Evitar movimiento infinito
    if (
      abs(diferenciaX) < 0.1
    ) {

      cartas[i].x =
        cartas[i].destinoX;
    }

    if (
      abs(diferenciaY) < 0.1
    ) {

      cartas[i].y =
        cartas[i].destinoY;
    }
  }
}

// ============================================================
// DIBUJAR JUEGO
// ============================================================

void dibujarJuego() {

  pantalla.clearDisplay();

  // ----------------------------------------------------------
  // CARTAS
  // ----------------------------------------------------------

  for (
    int i = 0;
    i < cantidadCartas;
    i++
  ) {

    dibujarCarta(
      cartas[i]
    );
  }

  // ----------------------------------------------------------
  // SEPARADOR
  // ----------------------------------------------------------

  pantalla.drawFastHLine(
    0,
    Y_SEPARADOR,
    SCREEN_WIDTH,
    SSD1306_WHITE
  );

  // ----------------------------------------------------------
  // CASA
  // ----------------------------------------------------------

  pantalla.setTextColor(
    SSD1306_WHITE
  );

  pantalla.setTextSize(1);

  pantalla.setCursor(
    2,
    Y_TEXTO_CASA
  );

  pantalla.print(
    "CASA "
  );

  int puntajeCasa =
    calcularPuntajeCasaVisible();

  pantalla.print(
    puntajeCasa
  );

  // ----------------------------------------------------------
  // JUGADOR
  // ----------------------------------------------------------

  pantalla.setCursor(
    2,
    Y_TEXTO_JUGADOR
  );

  pantalla.print(
    "JUGADOR "
  );

  int puntajeJugador =
    calcularPuntajeJugador();

  pantalla.print(
    puntajeJugador
  );

  pantalla.display();
}

// ============================================================
// REPARTO INICIAL
// ============================================================

void actualizarRepartoInicial() {

  if (
    millis() - ultimaCarta <
    TIEMPO_ENTRE_CARTAS
  ) {

    return;
  }

  switch (pasoInicial) {

    // --------------------------------------------------------
    // JUGADOR CARTA 1
    // --------------------------------------------------------

    case INICIO_JUGADOR_1:

      agregarCarta(
        true,
        false
      );

      pasoInicial =
        INICIO_JUGADOR_2;

      ultimaCarta =
        millis();

      break;

    // --------------------------------------------------------
    // JUGADOR CARTA 2
    // --------------------------------------------------------

    case INICIO_JUGADOR_2:

      agregarCarta(
        true,
        false
      );

      pasoInicial =
        INICIO_CASA_VISIBLE;

      ultimaCarta =
        millis();

      break;

    // --------------------------------------------------------
    // CASA CARTA VISIBLE
    // --------------------------------------------------------

    case INICIO_CASA_VISIBLE:

      agregarCarta(
        false,
        false
      );

      pasoInicial =
        INICIO_CASA_OCULTA;

      ultimaCarta =
        millis();

      break;

    // --------------------------------------------------------
    // CASA CARTA OCULTA
    // --------------------------------------------------------

    case INICIO_CASA_OCULTA:

      agregarCarta(
        false,
        true
      );

      pasoInicial =
        INICIO_TERMINADO;

      ultimaCarta =
        millis();

      break;

    // --------------------------------------------------------
    // TERMINO REPARTO
    // --------------------------------------------------------

    case INICIO_TERMINADO:

      estadoJuego =
        ESTADO_JUGADOR;

      ultimaCarta =
        millis();

      break;
  }
}

// ============================================================
// TURNO DEL JUGADOR
// ============================================================

void actualizarJugador() {

  int puntajeJugador =
    calcularPuntajeJugador();

  // ----------------------------------------------------------
  // MAS DE 21
  // ----------------------------------------------------------

  if (
    puntajeJugador > 21
  ) {

    resultado =
      RESULTADO_CASA;

    estadoJuego =
      ESTADO_ESPERANDO_RESULTADO;

    inicioEsperaResultado =
      millis();

    return;
  }

  // ----------------------------------------------------------
  // 20 O 21
  // SE PLANTA
  // ----------------------------------------------------------

  if (
    puntajeJugador >= 20
  ) {

    estadoJuego =
      ESTADO_REVELAR_CASA;

    ultimaCarta =
      millis();

    return;
  }

  // ----------------------------------------------------------
  // 18 O 19
  // DECISION ALEATORIA
  // ----------------------------------------------------------

  if (
    puntajeJugador == 18 ||
    puntajeJugador == 19
  ) {

    if (
      millis() - ultimaCarta >=
      TIEMPO_ENTRE_CARTAS
    ) {

      int decision =
        random(0, 2);

      // Pedir otra
      if (
        decision == 1
      ) {

        agregarCarta(
          true,
          false
        );

      }

      // Plantarse
      else {

        estadoJuego =
          ESTADO_REVELAR_CASA;
      }

      ultimaCarta =
        millis();
    }

    return;
  }

  // ----------------------------------------------------------
  // 17 O MENOS
  // SIEMPRE PEDIR
  // ----------------------------------------------------------

  if (
    puntajeJugador <= 17
  ) {

    if (
      millis() - ultimaCarta >=
      TIEMPO_ENTRE_CARTAS
    ) {

      agregarCarta(
        true,
        false
      );

      ultimaCarta =
        millis();
    }

    return;
  }
}

// ============================================================
// REVELAR CASA
// ============================================================

void actualizarRevelarCasa() {

  if (
    millis() - ultimaCarta <
    TIEMPO_ENTRE_CARTAS
  ) {

    return;
  }

  revelarCartaCasa();

  estadoJuego =
    ESTADO_CASA;

  ultimaCarta =
    millis();
}

// ============================================================
// TURNO DE LA CASA
// ============================================================

void actualizarCasa() {

  int puntajeCasa =
    calcularPuntajeCasaReal();

  int puntajeJugador =
    calcularPuntajeJugador();

  // ----------------------------------------------------------
  // CASA SE PASA
  // ----------------------------------------------------------

  if (
    puntajeCasa > 21
  ) {

    resultado =
      RESULTADO_JUGADOR;

    estadoJuego =
      ESTADO_ESPERANDO_RESULTADO;

    inicioEsperaResultado =
      millis();

    return;
  }

  // ----------------------------------------------------------
  // CASA LLEGA A 17 O MAS
  // ----------------------------------------------------------

  if (
    puntajeCasa >= 17
  ) {

    if (
      puntajeJugador > puntajeCasa
    ) {

      resultado =
        RESULTADO_JUGADOR;

    } else if (
      puntajeCasa > puntajeJugador
    ) {

      resultado =
        RESULTADO_CASA;

    } else {

      resultado =
        RESULTADO_EMPATE;
    }

    // --------------------------------------------------------
    // IMPORTANTE:
    // NO MOSTRAR EL RESULTADO TODAVIA
    // --------------------------------------------------------

    estadoJuego =
      ESTADO_ESPERANDO_RESULTADO;

    inicioEsperaResultado =
      millis();

    return;
  }

  // ----------------------------------------------------------
  // CASA TIENE MENOS DE 17
  // ----------------------------------------------------------

  if (
    millis() - ultimaCarta >=
    TIEMPO_ENTRE_CARTAS
  ) {

    agregarCarta(
      false,
      false
    );

    ultimaCarta =
      millis();
  }
}

// ============================================================
// ESPERA ANTES DEL RESULTADO
// ============================================================

void actualizarEsperaResultado() {

  if (
    millis() -
    inicioEsperaResultado >=
    TIEMPO_ANTES_RESULTADO
  ) {

    estadoJuego =
      ESTADO_RESULTADO;

    inicioResultado =
      millis();
  }
}

// ============================================================
// DIBUJAR RESULTADO
// ============================================================

void dibujarResultado() {

  pantalla.clearDisplay();

  pantalla.setTextColor(
    SSD1306_WHITE
  );

  pantalla.setTextSize(2);

  // ----------------------------------------------------------
  // JUGADOR GANA
  // ----------------------------------------------------------

  if (
    resultado ==
    RESULTADO_JUGADOR
  ) {

    pantalla.setCursor(
      19,
      23
    );

    pantalla.print(
      "Ganaste!"
    );
  }

  // ----------------------------------------------------------
  // CASA GANA
  // ----------------------------------------------------------

  else if (
    resultado ==
    RESULTADO_CASA
  ) {

    pantalla.setCursor(
      28,
      15
    );

    pantalla.print(
      "Gana"
    );

    pantalla.setCursor(
      22,
      36
    );

    pantalla.print(
      "la casa"
    );
  }

  // ----------------------------------------------------------
  // EMPATE
  // ----------------------------------------------------------

  else {

    pantalla.setCursor(
      35,
      25
    );

    pantalla.print(
      "Empate"
    );
  }

  pantalla.display();
}

// ============================================================
// SETUP
// ============================================================

void setup() {

  // ----------------------------------------------------------
  // I2C
  // ----------------------------------------------------------

  Wire.begin();

  Wire.setClock(400000);

  // ----------------------------------------------------------
  // OLED
  // ----------------------------------------------------------

  if (
    !pantalla.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    while (true) {
    }
  }

  // ----------------------------------------------------------
  // PANTALLA DE INICIO
  // ----------------------------------------------------------

  pantalla.clearDisplay();

  pantalla.setTextColor(
    SSD1306_WHITE
  );

  pantalla.setTextSize(2);

  pantalla.setCursor(
    18,
    24
  );

  pantalla.print(
    "BLACKJACK"
  );

  pantalla.display();

  delay(1000);

  // ----------------------------------------------------------
  // SEMILLA ALEATORIA
  // ----------------------------------------------------------

  randomSeed(
    analogRead(A0)
  );

  // ----------------------------------------------------------
  // COMENZAR PARTIDA
  // ----------------------------------------------------------

  nuevaPartida();
}

// ============================================================
// LOOP
// ============================================================

void loop() {

  unsigned long ahora =
    millis();

  // ----------------------------------------------------------
  // CONTROL DE FPS
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
  // ANIMACIONES
  // ----------------------------------------------------------

  actualizarAnimaciones();

  // ----------------------------------------------------------
  // ESTADOS DEL JUEGO
  // ----------------------------------------------------------

  if (
    estadoJuego ==
    ESTADO_REPARTO_INICIAL
  ) {

    actualizarRepartoInicial();

  } else if (
    estadoJuego ==
    ESTADO_JUGADOR
  ) {

    actualizarJugador();

  } else if (
    estadoJuego ==
    ESTADO_REVELAR_CASA
  ) {

    actualizarRevelarCasa();

  } else if (
    estadoJuego ==
    ESTADO_CASA
  ) {

    actualizarCasa();

  } else if (
    estadoJuego ==
    ESTADO_ESPERANDO_RESULTADO
  ) {

    actualizarEsperaResultado();

  } else if (
    estadoJuego ==
    ESTADO_RESULTADO
  ) {

    // Mantener el cartel durante 3 segundos
    if (
      ahora - inicioResultado >=
      TIEMPO_RESULTADO
    ) {

      nuevaPartida();

      return;
    }
  }

  // ----------------------------------------------------------
  // DIBUJAR
  // ----------------------------------------------------------

  if (
    estadoJuego ==
    ESTADO_RESULTADO
  ) {

    dibujarResultado();

  } else {

    dibujarJuego();
  }
}