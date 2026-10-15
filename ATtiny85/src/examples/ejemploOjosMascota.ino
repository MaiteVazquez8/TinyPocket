#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define ANCHO_PANTALLA 128
#define ALTO_PANTALLA 64

Adafruit_SSD1306 pantalla(ANCHO_PANTALLA, ALTO_PANTALLA, &Wire, -1);


// =====================================================
// POSICIONES
// =====================================================

const int CENTRO_IZQ = 43;
const int CENTRO_DER = 85;

const int CENTRO_Y = 32;


// =====================================================
// TAMAÑO DE LOS OJOS
// =====================================================

const float OJO_ANCHO = 25;
const float OJO_ALTO = 31;


// =====================================================
// VELOCIDAD DE ANIMACION
// =====================================================

const int FRAME = 20;


// =====================================================
// SETUP
// =====================================================

void setup() {

  if (!pantalla.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {

    while (true) {
    }
  }

  // I2C rapido
  Wire.setClock(400000);

  pantalla.clearDisplay();
  pantalla.display();
}


// =====================================================
// LOOP
// =====================================================

void loop() {

  // -----------------------------------------
  // ESTADO NORMAL
  // -----------------------------------------

  ojosNormales();

  delay(800);


  // -----------------------------------------
  // MIRA LENTAMENTE A LA IZQUIERDA
  // -----------------------------------------

  moverOjos(0, -15, 600);

  delay(500);


  // -----------------------------------------
  // VUELVE AL CENTRO
  // -----------------------------------------

  moverOjos(-15, 0, 550);

  delay(300);


  // -----------------------------------------
  // MIRA A LA DERECHA
  // -----------------------------------------

  moverOjos(0, 15, 650);

  delay(500);


  // -----------------------------------------
  // VUELVE AL CENTRO
  // -----------------------------------------

  moverOjos(15, 0, 550);

  delay(500);


  // -----------------------------------------
  // PARPADEO
  // -----------------------------------------

  parpadear();

  delay(600);


  // -----------------------------------------
  // FELIZ
  // -----------------------------------------

  feliz();

  delay(1000);


  // -----------------------------------------
  // VUELVE A NORMAL
  // -----------------------------------------

  volverNormal(500);

  delay(400);


  // -----------------------------------------
  // SORPRESA
  // -----------------------------------------

  sorpresa();

  delay(700);


  // -----------------------------------------
  // VUELVE A NORMAL
  // -----------------------------------------

  volverNormal(450);

  delay(400);


  // -----------------------------------------
  // GUIÑO
  // -----------------------------------------

  guino();

  delay(500);


  // -----------------------------------------
  // MIRA UN POQUITO
  // -----------------------------------------

  moverOjos(0, -8, 400);

  delay(300);

  moverOjos(-8, 0, 400);

  delay(500);


  // -----------------------------------------
  // DORMIR
  // -----------------------------------------

  dormir();

  delay(1200);


  // -----------------------------------------
  // DESPERTAR
  // -----------------------------------------

  despertar();

  delay(800);
}


// =====================================================
// OJO REDONDEADO
// =====================================================

void dibujarOjo(float x, float y, float ancho, float alto) {

  pantalla.fillRoundRect(
    (int)(x - ancho / 2),
    (int)(y - alto / 2),
    (int)ancho,
    (int)alto,
    8,
    SSD1306_WHITE
  );
}


// =====================================================
// DIBUJAR DOS OJOS
// =====================================================

void dibujarDosOjos(
  float desplazamiento,
  float ancho,
  float alto,
  float y
) {

  pantalla.clearDisplay();

  dibujarOjo(
    CENTRO_IZQ + desplazamiento,
    y,
    ancho,
    alto
  );

  dibujarOjo(
    CENTRO_DER + desplazamiento,
    y,
    ancho,
    alto
  );

  pantalla.display();
}


// =====================================================
// OJOS NORMALES
// =====================================================

void ojosNormales() {

  dibujarDosOjos(
    0,
    OJO_ANCHO,
    OJO_ALTO,
    CENTRO_Y
  );
}


// =====================================================
// MOVIMIENTO SUAVE
// =====================================================

void moverOjos(float desde, float hasta, int duracion) {

  int frames = duracion / FRAME;

  for (int i = 0; i <= frames; i++) {

    float progreso = (float)i / frames;

    // Smoothstep
    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float posicion =
      desde +
      (hasta - desde) * suave;

    dibujarDosOjos(
      posicion,
      OJO_ANCHO,
      OJO_ALTO,
      CENTRO_Y
    );

    delay(FRAME);
  }
}


// =====================================================
// PARPADEO MUY SUAVE
// =====================================================

void parpadear() {

  // CERRAR

  for (int i = 0; i <= 12; i++) {

    float progreso = (float)i / 12.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      OJO_ALTO *
      (1.0 - suave);

    if (alto < 2) {
      alto = 2;
    }

    dibujarDosOjos(
      0,
      OJO_ANCHO,
      alto,
      CENTRO_Y
    );

    delay(FRAME);
  }


  // OJOS CERRADOS

  pantalla.clearDisplay();

  pantalla.drawLine(
    CENTRO_IZQ - 10,
    CENTRO_Y,
    CENTRO_IZQ + 10,
    CENTRO_Y,
    SSD1306_WHITE
  );

  pantalla.drawLine(
    CENTRO_DER - 10,
    CENTRO_Y,
    CENTRO_DER + 10,
    CENTRO_Y,
    SSD1306_WHITE
  );

  pantalla.display();

  delay(90);


  // ABRIR

  for (int i = 0; i <= 12; i++) {

    float progreso = (float)i / 12.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      2 +
      (OJO_ALTO - 2) * suave;

    dibujarDosOjos(
      0,
      OJO_ANCHO,
      alto,
      CENTRO_Y
    );

    delay(FRAME);
  }
}


// =====================================================
// FELIZ
// =====================================================

void feliz() {

  // Los ojos se achican suavemente

  for (int i = 0; i <= 12; i++) {

    float progreso = (float)i / 12.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      OJO_ALTO -
      (OJO_ALTO - 6) * suave;

    dibujarDosOjos(
      0,
      OJO_ANCHO,
      alto,
      CENTRO_Y + 3
    );

    delay(FRAME);
  }


  // OJOS FELICES

  pantalla.clearDisplay();

  dibujarOjoFeliz(CENTRO_IZQ);
  dibujarOjoFeliz(CENTRO_DER);

  pantalla.display();
}


// =====================================================
// OJO FELIZ
// =====================================================

void dibujarOjoFeliz(int x) {

  pantalla.drawLine(
    x - 11,
    CENTRO_Y + 2,
    x - 6,
    CENTRO_Y - 3,
    SSD1306_WHITE
  );

  pantalla.drawLine(
    x - 6,
    CENTRO_Y - 3,
    x,
    CENTRO_Y - 5,
    SSD1306_WHITE
  );

  pantalla.drawLine(
    x,
    CENTRO_Y - 5,
    x + 6,
    CENTRO_Y - 3,
    SSD1306_WHITE
  );

  pantalla.drawLine(
    x + 6,
    CENTRO_Y - 3,
    x + 11,
    CENTRO_Y + 2,
    SSD1306_WHITE
  );
}


// =====================================================
// VOLVER A NORMAL
// =====================================================

void volverNormal(int duracion) {

  int frames = duracion / FRAME;

  for (int i = 0; i <= frames; i++) {

    float progreso = (float)i / frames;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      6 +
      (OJO_ALTO - 6) * suave;

    dibujarDosOjos(
      0,
      OJO_ANCHO,
      alto,
      CENTRO_Y + 3 - (3 * suave)
    );

    delay(FRAME);
  }
}


// =====================================================
// SORPRESA
// =====================================================

void sorpresa() {

  // Los ojos crecen

  for (int i = 0; i <= 15; i++) {

    float progreso = (float)i / 15.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float ancho =
      OJO_ANCHO +
      7 * suave;

    float alto =
      OJO_ALTO +
      8 * suave;

    dibujarDosOjos(
      0,
      ancho,
      alto,
      CENTRO_Y
    );

    delay(FRAME);
  }


  // Pequeño rebote

  for (int i = 0; i < 3; i++) {

    dibujarDosOjos(
      0,
      OJO_ANCHO + 7,
      OJO_ALTO + 8,
      CENTRO_Y - 1
    );

    delay(35);

    dibujarDosOjos(
      0,
      OJO_ANCHO + 7,
      OJO_ALTO + 8,
      CENTRO_Y + 1
    );

    delay(35);
  }
}


// =====================================================
// GUIÑO
// =====================================================

void guino() {

  // Cerrar ojo izquierdo

  for (int i = 0; i <= 10; i++) {

    float progreso = (float)i / 10.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      OJO_ALTO -
      (OJO_ALTO - 3) * suave;

    pantalla.clearDisplay();

    dibujarOjo(
      CENTRO_IZQ,
      CENTRO_Y,
      OJO_ANCHO,
      alto
    );

    dibujarOjo(
      CENTRO_DER,
      CENTRO_Y,
      OJO_ANCHO,
      OJO_ALTO
    );

    pantalla.display();

    delay(FRAME);
  }


  delay(150);


  // Abrir

  for (int i = 0; i <= 10; i++) {

    float progreso = (float)i / 10.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      3 +
      (OJO_ALTO - 3) * suave;

    pantalla.clearDisplay();

    dibujarOjo(
      CENTRO_IZQ,
      CENTRO_Y,
      OJO_ANCHO,
      alto
    );

    dibujarOjo(
      CENTRO_DER,
      CENTRO_Y,
      OJO_ANCHO,
      OJO_ALTO
    );

    pantalla.display();

    delay(FRAME);
  }
}


// =====================================================
// DORMIR
// =====================================================

void dormir() {

  for (int i = 0; i <= 18; i++) {

    float progreso = (float)i / 18.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      OJO_ALTO -
      (OJO_ALTO - 2) * suave;

    dibujarDosOjos(
      0,
      OJO_ANCHO,
      alto,
      CENTRO_Y
    );

    delay(FRAME);
  }


  // Ojos cerrados

  pantalla.clearDisplay();

  pantalla.drawLine(
    CENTRO_IZQ - 10,
    CENTRO_Y,
    CENTRO_IZQ + 10,
    CENTRO_Y,
    SSD1306_WHITE
  );

  pantalla.drawLine(
    CENTRO_DER - 10,
    CENTRO_Y,
    CENTRO_DER + 10,
    CENTRO_Y,
    SSD1306_WHITE
  );

  pantalla.display();
}


// =====================================================
// DESPERTAR
// =====================================================

void despertar() {

  // Abrir lentamente

  for (int i = 0; i <= 18; i++) {

    float progreso = (float)i / 18.0;

    float suave =
      progreso *
      progreso *
      (3.0 - 2.0 * progreso);

    float alto =
      2 +
      (OJO_ALTO - 2) * suave;

    dibujarDosOjos(
      0,
      OJO_ANCHO,
      alto,
      CENTRO_Y
    );

    delay(FRAME);
  }


  delay(200);

  // Parpadeo de recién despertado

  parpadear();
}