#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ============================================================
// TinyPocket - ejemplo de animacion: PELOTA REBOTANDO
// Version ATtiny85.
//
// En el ATtiny85 no cabe un OLED de 128x64: su buffer de 1024 bytes
// no entra en los 512 bytes de RAM del chip. Este ejemplo muestra la
// MISMA animacion (pelota que rebota en las cuatro paredes, con
// estela y con el pixel extra al tocar) sobre el LCD de 16x2 que si
// cabe, usando los caracteres especiales del LCD.
//
// Pantalla : LCD 16x2 I2C, direccion 0x27
// I2C      : SDA -> PB0, SCL -> PB2
// Botones  : PB3, PB4, PB1 (este ejemplo no los usa)
//
// No hay ni un delay() en el bucle: el movimiento va con millis().
//
// Compilar con:
//   placa  : ATtiny25/45/85 (No bootloader), ATTinyCore, 8 MHz interno
//   memoria: 3162 bytes de flash (38%), 151 de SRAM (29%)
// ============================================================

#define LCD_ADDRESS 0x27
const char CH_PELOTA = 0;
const char CH_ESTELA = 1;

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
char buffer[33];

// La pelota recorre un tablero invisible de 16 columnas por 2 filas.
const unsigned long FRAME_TIME = 120;   // ~8 fps: la LCD va igual de rapido
const int LARGO_TRAIL = 4;

int8_t pelotaX = 8, pelotaY = 0;
int8_t velX = 1, velY = 1;
uint8_t rebote = 0;                      // cuenta hacia abajo tras tocar
uint8_t trailX[LARGO_TRAIL], trailY[LARGO_TRAIL];
uint8_t trailContador = 0;
unsigned long ultimoFrame = 0;

// Los tipos y los prototipos van antes de la primera funcion: el IDE
// mete los prototipos que genera justo tras los #include y si un enum
// aparece mas abajo genera declaraciones rotas.
void crearCaracteres();
void borrarPantalla();
void poner(uint8_t col, uint8_t fila, char c);
void pintar();
void actualizarPelota();

void crearCaracteres() {
  uint8_t pelota[8] = {0x00, 0x0E, 0x1F, 0x1F, 0x1F, 0x0E, 0x00, 0x00};
  uint8_t estela[8] = {0x00, 0x00, 0x1F, 0x1F, 0x1F, 0x00, 0x00, 0x00};
  lcd.createChar(CH_PELOTA, pelota);
  lcd.createChar(CH_ESTELA, estela);
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

void actualizarPelota() {
  // Mueve la estela antes de mover la pelota.
  for (int8_t i = LARGO_TRAIL - 1; i > 0; i--) {
    trailX[i] = trailX[i - 1];
    trailY[i] = trailY[i - 1];
  }
  trailX[0] = (uint8_t)pelotaX;
  trailY[0] = (uint8_t)pelotaY;

  pelotaX = (int8_t)(pelotaX + velX);
  pelotaY = (int8_t)(pelotaY + velY);

  // Rebote en las cuatro paredes.
  if (pelotaX <= 0) { pelotaX = 0; velX = 1; rebote = 8; }
  if (pelotaX >= 15) { pelotaX = 15; velX = -1; rebote = 8; }
  if (pelotaY <= 0) { pelotaY = 0; velY = 1; rebote = 8; }
  if (pelotaY >= 1) { pelotaY = 1; velY = -1; rebote = 8; }

  // El "latido" del rebote: cuenta hacia abajo y marca en el texto.
  if (rebote > 0) rebote--;

  // La estela se dibuja completa una vez que hay 4 puntos.
  if (++trailContador > LARGO_TRAIL) trailContador = LARGO_TRAIL;
}

void setup() {
  pinMode(PB3, INPUT_PULLUP);
  pinMode(PB4, INPUT_PULLUP);
  pinMode(PB1, INPUT_PULLUP);
  randomSeed(analogRead(A0) + micros());
  Wire.begin();
  Wire.setClock(100000);
  lcd.init();
  lcd.backlight();
  crearCaracteres();
  for (uint8_t i = 0; i < LARGO_TRAIL; i++) { trailX[i] = 8; trailY[i] = 0; }
}

void loop() {
  if (millis() - ultimoFrame < FRAME_TIME) return;
  ultimoFrame = millis();
  actualizarPelota();
  borrarPantalla();
  // Fila 0: la estela y la pelota. Fila 1: coordenadas y el latido.
  for (uint8_t i = (trailContador > 0 ? trailContador - 1 : 0); i < LARGO_TRAIL; i++) {
    if (i > 0) poner(trailX[i], trailY[i], CH_ESTELA);
  }
  poner((uint8_t)pelotaX, (uint8_t)pelotaY, CH_PELOTA);
  char fila[17];
  uint8_t n = 0;
  fila[n++] = 'X'; fila[n++] = '=';
  fila[n++] = (char)('0' + (pelotaX / 10));
  fila[n++] = (char)('0' + (pelotaX % 10));
  fila[n++] = ' ';
  fila[n++] = 'Y'; fila[n++] = '=';
  fila[n++] = (char)('0' + pelotaY);
  fila[n++] = ' ';
  fila[n++] = rebote > 0 ? '*' : '.';
  fila[n] = '\0';
  for (uint8_t i = 0; i < n; i++) buffer[16 + i] = fila[i];
  pintar();
}