# Explicación del código de TinyPocket — Arduino Uno

Este documento explica cómo está organizado `src/tinyPocket.ino` en la versión
para **Arduino Uno** del proyecto TinyPocket.

El microcontrolador es el **ATmega328P** (8 bits, **32 KB de flash y 2 KB de
SRAM**), conectado a un **OLED de 128×64 por I²C** y tres botones.

Esta versión tiene **las mismas seis funciones** que la del Mega (menú, Ping
Pong, Breakout, Mascota, Blackjack, Snake e Información) y los mismos botones,
pero reescrita para que quepa en 2 KB de RAM y 32 KB de flash.

## 1. Librerías

```cpp
#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>
```

Las mismas que en la versión Mega: **U8g2** para el OLED y `math.h` para las
interpolaciones.

## 2. Configuración de la pantalla: aquí está la diferencia clave

```cpp
U8G2_SH1106_128X64_NONAME_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306:
// U8G2_SSD1306_128X64_NONAME_1_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
```

El sufijo **`_1_`** es lo que hace posible que este programa exista en un Uno:
es el **búfer de una sola página**, de **128 bytes**, en lugar del búfer completo
(`_F_`) de **1024 bytes** que usa el Mega.

Se comprobó con el compilador: con el búfer completo, este mismo programa
**ocupaba 3169 bytes de SRAM y no entraba en los 2048 del ATmega328P**. Con el
búfer de una página se queda en **1753 bytes**.

U8g2 se encarga de volcar la pantalla por páginas, así que **el código de dibujo
no cambia ni una línea**: los dos programas usan `clearBuffer()`, `setFont()`,
`drawDisc()`, `sendBuffer()`… exactamente igual.

> Si algún día se cambia la placa por una con más RAM, se puede volver a `_F_`
> sin tocar nada más.

## 3. Conexiones

Según `Img/circuitoDiagrama.png`:

| Señal | Pin del Uno |
| --- | --- |
| OLED SDA | **A4** |
| OLED SCL | **A5** |
| Botón izquierda | D2 |
| Botón derecha | D3 |
| Botón OK | D4 |
| Alimentación | switch + batería al pin BAT-5V |

A4 y A5 son los pines I²C de hardware del ATmega328P, por eso la variante
`_HW_I2C` funciona sin indicar SDA ni SCL. **Ojo**: A4 y A5 son pines
analógicos, pero solo se usan como I²C, así que no estorban.

Los botones usan `INPUT_PULLUP` (sin pulsar = `HIGH`, pulsado = `LOW`), igual que
en las otras dos versiones.

## 4. Antirrebote de botones

Idéntico al del Mega: una función para los tres botones, estado estable tras
**25 ms** y dos eventos al soltar.

* **corto** → `corto = true`
* **800 ms o más** → `largo = true`, que es el comando de volver al menú

## 5. Adaptaciones de memoria, y por qué

El ATmega328P tiene 2 KB de RAM y 32 KB de flash. Estas son las seis cosas que
se cambiaron respecto a la versión Mega, todas por culpa de esas cifras:

| # | Adaptación | Ahorro |
| --- | --- | --- |
| 1 | Búfer de una página (`_1_`) en vez de completo (`_F_`) | 896 B de SRAM |
| 2 | Una sola fuente, la pequeña 4×6 | ~2,1 KB de flash |
| 3 | Mazo del blackjack con **1 byte por carta** (0..51) en vez de una estructura de 2 bytes | 52 B de SRAM |
| 4 | Coordenadas de la serpiente y de las cartas animadas como `int16_t` en vez de `float` | ~160 B de SRAM |
| 5 | Palos de las cartas con primitivas sencillas, sin triángulos rellenos | ~1 KB de flash |
| 6 | 24 segmentos de serpiente en vez de 40, y 6 puntos de estela en vez de 8 | ~130 B de SRAM |
| 7 | Sin `sqrt()` ni `sin()`: distancias comparadas al cuadrado y rebote triangular | ~0,5 KB de flash |
| 8 | **Todos los textos literales con `F()`**, copiados a un buffer de 32 bytes al dibujar | **518 B de RAM** |

### La fuente única

Medido con el compilador: **añadir una segunda fuente cuesta unos 2 KB**, el 6 %
del programa entero. Por eso el Uno usa solo la 4×6 y para que los títulos no se
vean débiles se dibujan **dos veces desplazados un píxel**, que produce un
efecto de negrita por dos bytes de código:

```cpp
void dibujarTitulo(const char *titulo, int y) {
  pantalla.setFont(u8g2_font_4x6_tf);
  pantalla.drawStr(0, y, titulo);
  pantalla.drawStr(1, y, titulo);
}
```

### Sin `sprintf()` ni `print()`

Entre los dos se llevan más de 2 KB de flash. En su lugar hay un conversor a
texto de veinte líneas:

```cpp
void dibujarNumero(int x, int y, int valor);
```

### Los textos se quedan en FLASH (el ahorro más grande)

En AVR **un literal escrito a pelo no vive en la flash**. El enlazador mete
`.rodata` dentro de `.data`, es decir dentro de la RAM, y además lo copia al
arrancar. Medido con `avr-size` sobre este mismo programa:

| Sección del ELF | Bytes |
| --- | --- |
| `.data` (textos + búfer de U8g2) | 806 |
| `.bss` (variables a cero) | 947 |
| **Total de RAM** | **1753** |

Los 60 textos del programa sumaban **692 bytes de RAM**, el 34 % de toda la
memoria del Uno. La solución es pasarlos todos con `F()` (que los manda a
PROGMEM) y copiarlos en el momento de dibujar:

```cpp
void dibujarTexto(int x, int y, const __FlashStringHelper *txt) {
  const char *p = (const char *)txt;
  uint8_t n = 0;
  while (n < TEXTOS_MAX - 1 && pgm_read_byte(p + n) != 0) {
    textoTemporal[n] = (char)pgm_read_byte(p + n);
    n++;
  }
  textoTemporal[n] = '\0';
  pantalla.drawStr(x, y, textoTemporal);
}
```

Con un buffer temporal de 32 bytes en la pila. Lo mismo se hizo con las cuatro
listas de textos (`LISTA_MENU`, `LISTA_MODOS`, `LISTA_ACCIONES`, `LISTA_STATS`),
que antes eran arrays de punteros: con eso se recuperaron **518 bytes de RAM** y
el programa incluso bajó de tamaño en flash.

> Si alguna vez se cambia la placa por una con más RAM, esta técnica sigue
> siendo buena: los textos en flash no consumen RAM en ninguna placa AVR.

## 6. Tipos y prototipos al principio del archivo

Todos los `struct` y `enum`, y el bloque de prototipos, están **antes de la
primera función**. El Arduino IDE inserta los prototipos que genera justo
después de los `#include`; si un `enum` aparece más abajo genera declaraciones
rotas y la compilación falla con `'FaseMascota' was not declared in this scope`.
Ese error ya ocurrió en este proyecto al mover las animaciones.

## 7. Estados de la aplicación

```cpp
enum Estado { MENU, SUBMENU_PINGPONG, JUEGO_PINGPONG, JUEGO_BREAKOUT,
              JUEGO_MASCOTA, JUEGO_BLACKJACK, JUEGO_SNAKE, INFORMACION };
```

Los mismos ocho estados que en el Mega, y el mismo `switch` en `loop()`.

## 8. Menú

Igual que en el Mega: marcador animado con interpolación suave y opción
seleccionada en negro sobre el recuadro. Con la fuente pequeña, los títulos usan
el doble dibujo en negrita.

## 9. Ping Pong

La misma física y el mismo aspecto que en el Mega, con el campo reducido para
dejar sitio al marcador y a la línea de controles:

* Paleta abajo (y=52), 24 px de ancho.
* Pelota de radio 2 que **crece a 3 px durante el rebote**.
* **Estela de 6 puntos** decrecientes.
* El ángulo de salida depende de dónde golpea, con mínimo de 0,35 para que la
  pelota no se quede pegada moviéndose en vertical.

## 10. Breakout

Los mismos bloques de **11×4 px** (8 columnas × 4 filas) que en el Mega. La
colisión se resuelve por la posición de la pelota en el fotograma anterior y, si
entra por una esquina, por la distancia mínima a cada cara.

Cada ladrillo roto acelera la pelota un 1,5 %, con tope en 2,8. Hay **3 vidas**.

## 11. Mascota

Las mismas **37 fases** que en el Mega y el Mega es quien las referencia, pero
las duraciones están en una **tabla en PROGMEM** en vez de un `switch` de 37
casos:

```cpp
const uint16_t duracionesFase[] PROGMEM = { 80, 60, 50, 55, 30, ... };
unsigned long mascotaDuracionFase(FaseMascota fase) {
  return (unsigned long)pgm_read_word(&duracionesFase[(uint8_t)fase]) * 10;
}
```

El rebote de la sorpresa usa un **pico triangular** en vez de `sin()`, para no
arrastrar la librería matemática.

Las cuatro estadísticas y las cuatro acciones se mantienen igual.

## 12. Blackjack

* Mazo real de 52 cartas, pero **guardado con un byte por carta**. El palo y el
  valor se deducen del índice:

```cpp
carta.carta.palo = nuevaCarta / 13;
carta.carta.valor = (nuevaCarta % 13) + 1;
```

* Las cartas **entran volando desde los laterales**, alternando el lado, con la
  misma interpolación que en el Mega.
* La segunda carta de la banca sale tapada hasta el final.
* Los palos se dibujan con círculos, líneas y cajas: se pierde el relleno del
  triángulo, pero se distinguen los cuatro.
* Se reparten cada 900 ms. Izquierda = pedir carta, derecha = plantarse.
* La banca pide cartas hasta llegar a 17 y después se resuelve la ronda.
* **La partida se gana al llegar a 3 victorias y se pierde al llegar a 3
  derrotas**, igual que en el Mega y en el ATtiny85. El marcador de rondas se
  ve en la pantalla de resultado; mientras la partida sigue viva, OK reparte
  cartas nuevas conservando el marcador, y al cerrarse la partida OK arranca
  una revancha que lo pone a cero.

## 13. Snake

* 24 segmentos como máximo, con coordenadas **`int16_t`**.
* Movimiento continuo de 1,35 px por fotograma dentro de un marco interior de
  116×40 px.
* Cabeza con **ojos que miran hacia donde avanza**.
* Comida que late cada 180 ms y doble marco al comer.
* **Controlado por el jugador** y **sin giros de 180°**:
  `izquierda` gira a la izquierda, `derecha` a la derecha, OK pausa y mantener OK
  vuelve al menú.

## 14. `millis()` en lugar de `delay()`

Ningún juego usa `delay()`. Cada uno lleva su propio reloj de 16 ms y solo
redibuja cuando toca, así que la pantalla nunca impide registrar las pulsaciones
del jugador.

## 15. Cómo compilar

| Parámetro | Valor |
| --- | --- |
| Placa | `Arduino UNO` |
| Core | `arduino:avr` (1.8.8) |

```bash
arduino-cli compile -b arduino:avr:uno src
```

## 16. Memoria: números medidos

| Programa | Flash | % | SRAM | % |
| --- | --- | --- | --- | --- |
| `src/tinyPocket.ino` | 31218 B | **97 %** | 1240 B | **61 %** |
| `src/examples/ejemploPelotaRebote.ino` | 13384 B | 41 % | 768 B | 38 % |
| `src/examples/ejemploPingPong.ino` | 15216 B | 47 % | 767 B | 37 % |
| `src/examples/ejemploBreakout.ino` | 17110 B | 53 % | 798 B | 39 % |
| `src/examples/ejemploSnake.ino` | 13498 B | 42 % | 1018 B | 50 % |
| `src/examples/ejemploBlackjack.ino` | 14918 B | 46 % | 898 B | 44 % |
| `src/examples/ejemploOjosMascota.ino` | 13192 B | 41 % | 666 B | 33 % |

**El programa principal está al 97 % de la flash y al 61 % de la RAM.** La
RAM se dejó en un 86 % al principio de la revisión y se bajó a este 61 %
sacando los textos a flash; en flash, en cambio, sigue justo: quedan unos
1 KB. Si hay que añadir algo nuevo, lo primero que hay que mirar es la tabla
de fuentes.

## 17. Diferencias con las otras dos versiones

| | Mega 2560 | Arduino Uno | ATtiny85 |
| --- | --- | --- | --- |
| Pantalla | OLED 128×64, búfer completo | OLED 128×64, búfer de 1 página | **LCD 16×2** |
| I2C | D20 / D21 | **A4 / A5** | PB0 / PB2 |
| Flash del programa | 42112 B (17 %) | 31218 B (97 %) | 5622 B (69 %) |
| SRAM del programa | 3220 B (39 %) | 1240 B (61 %) | 267 B (52 %) |
| Fuentes | 3 | 1 (4×6) | 1 (caracteres) |
| Interfaz | Títulos grandes | Títulos "en negrita" con una fuente | Una opción por fila |

## 18. Limitaciones conocidas

* **La flash está casi llena (97 %).** No hay sitio para un juego nuevo sin
  quitar algo: el primer candidato a irse sería el Breakout, que es el bloque
  más grande del programa.
* **Solo una fuente de texto.** Todos los rótulos son pequeños; el precio de la
  legibilidad fue una decisión de memoria.
* El ATmega328P no tiene más pines I²C: A4 y A5 están ocupados por el OLED, así
  que no se le puede añadir un segundo dispositivo I²C.

## Resumen

`tinyPocket.ino` en esta carpeta es TinyPocket completo para Arduino Uno: las
mismas seis funciones, los mismos botones y las mismas reglas de juego que en
el Mega, con el búfer de pantalla reducido a una página, una sola fuente, un mazo
de un byte por carta y coordenadas enteras. Cabe en 2 KB de RAM y 32 KB de flash,
pero sin mucho margen para crecer.