#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - ejemplo de animacion: OJOS DE LA MASCOTA
// Version ATtiny85.
//
// El OLED dibuja la cara con formas de 25x31 pixeles. En el LCD de
// 16x2 no hay pixeles libres, asi que la misma expresion se cuenta con
// las fases y los mismos tiempos, pero dibujada con caracteres:
// parentesis para la cara, guion bajo como ojo abierto, un caracter
// especial como ojo cerrado y un corazon como boca sonriente.
//
// Las fases son las mismas que en el ejemplo del Mega y las que usa
// tinyPocket.ino: mirar, volver al centro, parpadear, feliz,
// sorpresa, guino y dormir.
//
// Pantalla : LCD 16x2 I2C, direccion 0x27
// I2C      : SDA -> PB0, SCL -> PB2
// Botones  : PB3 (izq), PB4 (der), PB1 (OK) - no usados aqui
//
// Sin delay(): las fases avanzan con millis().
//
// Compilar con:
//   placa  : ATtiny25/45/85 (No bootloader), ATTinyCore, 8 MHz interno
// ============================================================

#define LCD_ADDRESS 0x27
const char CH_OJO_CERRADO = 0;
const char CH_CORAZON = 1;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
char buffer[33];

// Las fases y sus duraciones son las del ejemplo del OLED, en centi-
// segundos para que la tabla quepa en PROGMEM sin gastar RAM.
enum Fase {
  NORMAL, MIRA_IZQ, PAUSA_IZQ, VUELVE_CENTRO_1, PAUSA_CENTRO_1,
  MIRA_DER, PAUSA_DER, VUELVE_CENTRO_2, PAUSA_CENTRO_2,
  PARPADEO_CIERRA, PARPADEO_ESPERA, PARPADEO_ABRE, PARPADEO_PAUSA,
  FELIZ, FELIZ_ESPERA, FELIZ_NORMAL, FELIZ_PAUSA,
  SORPRESA, SORPRESA_REBOTE, SORPRESA_ESPERA, SORPRESA_NORMAL, SORPRESA_PAUSA,
  GUINO_CIERRA, GUINO_ESPERA, GUINO_ABRE, GUINO_PAUSA,
  MIRA_POCO_IZQ, MIRA_POCO_PAUSA, MIRA_POCO_CENTRO, MIRA_POCO_PAUSA_2,
  DORMIR_CIERRA, DORMIR, DESPERTAR, DESPERTAR_PAUSA,
  DESPERTAR_PARPADEO_CIERRA, DESPERTAR_PARPADEO_ESPERA, DESPERTAR_PARPADEO_ABRE
};

const uint16_t duracionFase[] PROGMEM = {
    80, 60, 50, 55, 30, 65, 50, 55, 50,
    24,  9, 24, 60,
    24, 100, 50, 40,
    30, 21, 70, 45, 40,
    20, 15, 20, 50,
    40, 30, 40, 50,
    36, 120, 36, 20, 24, 9, 24
};

Fase fase = NORMAL;
unsigned long inicioFase = 0;

void crearCaracteres() {
  uint8_t ojoCerrado[8] = {0x00, 0x00, 0x00, 0x1F, 0x00, 0x00, 0x00, 0x00};
  uint8_t corazon[8] = {0x00, 0x0A, 0x1F, 0x1F, 0x0E, 0x04, 0x00, 0x00};
  lcd.createChar(CH_OJO_CERRADO, ojoCerrado);
  lcd.createChar(CH_CORAZON, corazon);
}
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
// Progreso de la fase, de 0 a 100.
uint8_t progreso() {
  unsigned long dur = (unsigned long)pgm_read_word(&duracionFase[fase]) * 10;
  unsigned long t = millis() - inicioFase;
  if (t >= dur) return 100;
  return (uint8_t)((t * 100) / dur);
}
void setup() {
  randomSeed(analogRead(A0) + micros());
  Wire.begin();
  Wire.setClock(100000);
  lcd.init();
  lcd.backlight();
  crearCaracteres();
  inicioFase = millis();
}
void loop() {
  unsigned long dur = (unsigned long)pgm_read_word(&duracionFase[fase]) * 10;
  if (millis() - inicioFase >= dur) {
    fase = (Fase)((fase + 1) % (DESPERTAR_PARPADEO_ABRE + 1));
    inicioFase = millis();
  }
  uint8_t p = progreso();

  borrarPantalla();

  // Fila 0: la cara. Se mueve con la mirada y cambia segun la fase.
  uint8_t desplazamiento = 0;
  if (fase == MIRA_IZQ) desplazamiento = (uint8_t)(15 * p / 100);
  else if (fase == VUELVE_CENTRO_1) desplazamiento = 15 - (uint8_t)(15 * p / 100);
  else if (fase == MIRA_DER) desplazamiento = 0;
  else if (fase == VUELVE_CENTRO_2) desplazamiento = (uint8_t)(15 * p / 100);
  else if (fase == MIRA_POCO_IZQ) desplazamiento = (uint8_t)(8 * p / 100);
  else if (fase == MIRA_POCO_CENTRO) desplazamiento = 8 - (uint8_t)(8 * p / 100);

  // Parpadeo: los dos ojos se cierran y se abren.
  bool cerrado = (fase == PARPADEO_CIERRA && p > 60) ||
                 (fase == PARPADEO_ESPERA) ||
                 (fase == DESPERTAR_PARPADEO_ESPERA) ||
                 (fase == DORMIR) ||
                 (fase == GUINO_ESPERA);
  bool feliz = (fase == FELIZ_ESPERA);

  char ojo = cerrado ? CH_OJO_CERRADO : '_';
  if (feliz) {
    buffer[4] = '<'; buffer[5] = '^'; buffer[6] = '>';
    buffer[9] = '<'; buffer[10] = '^'; buffer[11] = '>';
    buffer[7] = CH_CORAZON;
  } else {
    buffer[4] = '(';
    buffer[5] = ojo;
    buffer[6] = ')';
    buffer[9] = '(';
    buffer[10] = ojo;
    buffer[11] = ')';
  }
  // La mirada desplaza los ojos hacia un lado.
  if (desplazamiento > 0 && !cerrado) {
    buffer[5] = (char)(5 + (desplazamiento > 3 ? 1 : 0));
    buffer[10] = (char)(10 + (desplazamiento > 3 ? 1 : 0));
  }

  // Fila 1: que fase es, para poder seguirla con el debugger.
  const char *nombre = "normal";
  switch (fase) {
    case NORMAL: nombre = "normal"; break;
    case MIRA_IZQ: case PAUSA_IZQ: case VUELVE_CENTRO_1: case PAUSA_CENTRO_1:
      nombre = "mirando izq"; break;
    case MIRA_DER: case PAUSA_DER: case VUELVE_CENTRO_2: case PAUSA_CENTRO_2:
      nombre = "mirando der"; break;
    case PARPADEO_CIERRA: case PARPADEO_ESPERA: case PARPADEO_ABRE: case PARPADEO_PAUSA:
      nombre = "parpadeo"; break;
    case FELIZ: case FELIZ_ESPERA: case FELIZ_NORMAL: case FELIZ_PAUSA:
      nombre = "feliz"; break;
    case SORPRESA: case SORPRESA_REBOTE: case SORPRESA_ESPERA:
    case SORPRESA_NORMAL: case SORPRESA_PAUSA:
      nombre = "sorpresa"; break;
    case GUINO_CIERRA: case GUINO_ESPERA: case GUINO_ABRE: case GUINO_PAUSA:
      nombre = "guino"; break;
    case DORMIR_CIERRA: case DORMIR: nombre = "durmiendo"; break;
    default: nombre = "despierta"; break;
  }
  uint8_t i = 0;
  while (nombre[i] != '\0' && i < 16) { buffer[16 + i] = nombre[i]; i++; }

  pintar();
}