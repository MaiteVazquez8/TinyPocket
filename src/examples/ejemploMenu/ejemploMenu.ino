
#include <Arduino.h>
#include <U8g2lib.h>

// I2C por software: SCL en D3 y SDA en D2
U8G2_SH1106_128X64_NONAME_F_SW_I2C pantalla(
  U8G2_R0,
  3,
  2,
  U8X8_PIN_NONE
);

void setup() {
  pantalla.begin();

  pantalla.clearBuffer();
  pantalla.setFont(u8g2_font_ncenB14_tr);
  pantalla.drawStr(0, 25, "Hola!");

  pantalla.setFont(u8g2_font_6x10_tr);
  pantalla.drawStr(0, 45, "TinyPocket");
  pantalla.drawStr(0, 60, "OLED funcionando");

  pantalla.sendBuffer();
}

void loop() {
}
