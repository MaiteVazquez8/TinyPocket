# Explicación del código de TinyPocket — Arduino Mega 2560

Este documento explica cómo está organizado `src/tinyPocket.ino` en la versión
para **Arduino Mega 2560** del proyecto TinyPocket.

El microcontrolador es el **ATmega2560** (8 bits, 253 KB de flash y **8 KB de
SRAM**), conectado a un **OLED de 128×64 por I²C** y tres botones.

Esta es la versión **de referencia** del proyecto: tiene todos los recursos, así
que es la que mantiene las animaciones con más detalle. Las versiones de
Arduino Uno y de ATtiny85 son adaptaciones suyas a placas con mucha menos
memoria, no copias recortadas al azar.

## 1. Librerías

```cpp
#include <Arduino.h>
#include <U8g2lib.h>
#include <math.h>
```

* **`Arduino.h`**: `pinMode()`, `digitalRead()`, `millis()`, `random()`.
* **`U8g2lib.h`**: control del OLED. Se eligió U8g2 y no Adafruit_SSD1306 porque
  el Mega puede permitirse el **búfer completo** de la pantalla (1 KB), que
  redibuja de una sola vez y sin parpadeos.
* **`math.h`**: `sin()`, `sqrt()`, `fabs()` para las interpolaciones.

## 2. Configuración de la pantalla

```cpp
U8G2_SH1106_128X64_NONAME_F_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
// Si tu OLED es SSD1306:
// U8G2_SSD1306_128X64_NONAME_F_HW_I2C pantalla(U8G2_R0, U8X8_PIN_NONE);
```

El sufijo `_F_` es el **búfer completo** (1024 bytes). El Mega tiene 8 KB de SRAM,
así que no hace falta ahorrar. La variante `_HW_I2C` usa los pines I²C de
hardware del Mega, que son D20 (SDA) y D21 (SCL).

## 3. Conexiones

Según `img/circuitoDiagrama.png`:

| Señal | Pin del Mega |
| --- | --- |
| OLED SDA | D20 (SDA de hardware) |
| OLED SCL | D21 (SCL de hardware) |
| Botón izquierda | D2 |
| Botón derecha | D3 |
| Botón OK | D4 |
| Alimentación | switch + batería al pin VIN |

Los botones usan `INPUT_PULLUP` (sin pulsar = `HIGH`, pulsado = `LOW`) y van
cada uno entre su pin y GND.

## 4. Antirrebote de botones

```cpp
struct Boton {
  byte pin; bool lecturaAnterior, estadoEstable;
  unsigned long cambioLectura, inicioPulsacion; bool corto, largo;
};
```

Una función, `actualizarBoton()`, atiende los tres botones. El estado solo
cambia si se mantiene **25 ms** estable. Al soltar hay dos eventos:

* **corto** → `corto = true`
* **800 ms o más** → `largo = true`

| Botón | Acción |
| --- | --- |
| Izquierda / Derecha | moverse, girar, pedir carta o plantarse |
| OK corto | confirmar, reiniciar, otra ronda |
| Mantener OK | volver al menú |

## 5. Tipos y prototipos al principio del archivo

Todos los `struct` y `enum`, y el bloque de prototipos, están **antes de la
primera función**. No es una manía: el Arduino IDE inserta los prototipos que
genera justo después de los `#include`, así que si un `enum` aparece más abajo
genera declaraciones rotas y la compilación falla con
`'FaseMascota' was not declared in this scope`. Es un error ya suffered en este
proyecto.

## 6. Pantalla

| Función | Qué hace |
| --- | --- |
| `clearBuffer()` + `sendBuffer()` | Vacía y vuelca el búfer |
| `setFont()` + `drawStr()` | Escribe texto |
| `drawDisc()`, `drawBox()`, `drawRBox()` | Círculos y rectángulos |
| `drawHLine()`, `drawVLine()`, `drawLine()` | Líneas |
| `setDrawColor(0)` | Escribe en negro (texto invertido) |

Las fuentes que se usan, y su coste aproximado en flash:

| Fuente | Uso | Tamaño |
| --- | --- | --- |
| `u8g2_font_6x10_tf` | Títulos, cabeceras, resultado del blackjack | ~0,9 KB |
| `u8g2_font_5x8_tf` | Marcadores y datos | ~0,7 KB |
| `u8g2_font_4x6_tf` | Línea de controles y textos pequeños | ~0,4 KB |

## 7. Estados de la aplicación

```cpp
enum Estado { MENU, SUBMENU_PINGPONG, JUEGO_PINGPONG, JUEGO_BREAKOUT,
              JUEGO_MASCOTA, JUEGO_BLACKJACK, JUEGO_SNAKE, INFORMACION };
```

`loop()` reparte con un `switch` y cada juego vuelve al menú con
`volverAlMenu()`.

## 8. Menú

Menú con **marcador animado**: un recuadro que se desliza hacia la opción
elegida con una interpolación suave (`marcadorMenuY`), no de un salto.

```cpp
marcadorMenuY += (destino - marcadorMenuY) * 0.38;
```

La opción seleccionada se dibuja en negro sobre el recuadro
(`setDrawColor(0)`), lo que hace el texto legible sin marcos extra.

## 9. Ping Pong

* Paleta abajo (y=52), 24 px de ancho, 3 de alto.
* Pelota de radio 2,5 que **crece a 3 px mientras dura el rebote**.
* **Estela de 6 puntos**, decreciendo de radio.
* El ángulo de salida depende de **dónde golpea**: en el borde sale más
  paralela, en el centro sube casi vertical.

```cpp
velocidadX = (pelotaX - paletaX) * 0.08;
if (fabs(velocidadX) < 0.35) velocidadX = (velocidadX >= 0) ? 0.35 : -0.35;
```

El mínimo de 0,35 es lo que evita que la pelota se quede Werner «pegada»
moviéndose en vertical de arriba abajo.

## 10. Breakout

* Bloques de **11×4 px**, 8 columnas × 4 filas, separados 2 px y centrados.
* La colisión se resuelve mirando **de dónde venía la pelota** en el fotograma
  anterior (`boPelotaAnteriorX/Y`), así que el rebote sale por la cara correcta.
  Si entra por una esquina, se decide por la distancia mínima.
* Cada ladrillo roto acelera la pelota un 1,5 %, con tope en 2,8.
* 3 vidas; al perderlas sale el cartel de fin de partida.

## 11. Mascota

37 fases de animación encadenadas con `millis()`, sin un solo `delay()`:

mirar izquierda → pausa → volver al centro → pausa → mirar derecha → pausa →
volver → pausa → **parpadeo** (cierre, espera, apertura) → **feliz** → volver a
normal → **sorpresa** (crece y da un bote) → volver a normal → **guiño** →
mirar un poco → **dormir** → **despertar** con parpadeo.

Las interpolaciones usan *smoothstep*:

```cpp
float suave = t * t * (3.0 - 2.0 * t);
```

que es lo que hace que los ojos se muevan con arranque y frenado suaves en vez
de a tirones.

Las cuatro estadísticas (hambre, higiene, felicidad, energía) bajan cada 10 s y se
pueden recuperar con las cuatro acciones. La acción de dormir fuerza las fases de
sueño; OK despierta.

## 12. Blackjack

* Mazo real de **52 cartas** (4 palos × 13 valores) barajado con Fisher-Yates.
* Las cartas **entran volando desde los laterales**, alternando el lado por el
  que llega cada una, y se colocan en el centro con una interpolación:

```cpp
cartas[i].x += (cartas[i].destinoX - cartas[i].x) * 0.18;
```

* La segunda carta de la banca sale tapada hasta el final.
* **Palos dibujados**: corazón, diamante, trébol y picas. Como U8g2 solo trae el
  triángulo de contorno, `llenarTriangulo()` rellena el triángulo a mano
  recorriendo filas.
* Se reparten cada 900 ms. Izquierda = pedir carta, derecha = plantarse.
* Gana quien más se acerque a 21 sin pasarse.

## 13. Snake

* Tablero de 116×40 px dentro de un marco, con 40 segmentos como máximo.
* Movimiento **continuo** (no a saltos de celda): la cabeza se mueve 1,35 px por
  fotograma y el cuerpo la sigue.
* La cabeza se dibuja como un círculo de 3 px con **dos ojos que miran hacia
  donde avanza**.
* La comida late (radio 2 y 3 cada 180 ms) y lleva un pixel encima.
* **Controlado por el jugador**, no por IA. Izquierda gira a la izquierda,
  derecha a la derecha, y **no se permite el giro de 180°**:

```cpp
if (nuevaX == -snDireccionX && nuevaY == -snDireccionY) return;
```

* Al comer aparece un doble marco alrededor de la comida como destello.
* La velocidad sube al pasar de 8 puntos.

## 14. `millis()` en lugar de `delay()`

Ningún juego usa `delay()`. Cada uno lleva su propio reloj:

```cpp
if (ahora - ultimoFrame >= 16) { ultimoFrame = ahora; ... }
```

y solo redibuja cuando toca. La pantalla nunca bloquea la lectura de botones,
porque `actualizarBotones()` se llama lo primero en cada vuelta de `loop()`.

## 15. Cómo compilar

| Parámetro | Valor |
| --- | --- |
| Placa | `Arduino Mega or Mega 2560` |
| Core | `arduino:avr` (1.8.8) |

```bash
arduino-cli compile -b arduino:avr:mega src
```

## 16. Memoria: números medidos

| Programa | Flash | % | SRAM | % |
| --- | --- | --- | --- | --- |
| `src/tinyPocket.ino` | 41816 B | 16 % | 3169 B | 39 % |
| `src/examples/ejemploPelotaRebote.ino` | 14116 B | 6 % | 1664 B | 20 % |
| `src/examples/ejemploPingPong.ino` | 15952 B | 6 % | 1663 B | 20 % |
| `src/examples/ejemploBreakout.ino` | 17840 B | 7 % | 1694 B | 21 % |
| `src/examples/ejemploSnake.ino` | 14224 B | 5 % | 1914 B | 23 % |
| `src/examples/ejemploBlackjack.ino` | 15652 B | 6 % | 1794 B | 22 % |
| `src/examples/ejemploOjosMascota.ino` | 13914 B | 5 % | 1562 B | 19 % |

## 17. Diferencias con las otras dos versiones

| | Mega 2560 | Arduino Uno | ATtiny85 |
| --- | --- | --- | --- |
| Pantalla | OLED 128×64, búfer completo | OLED 128×64, búfer de 1 página | **LCD 16×2** |
| SRAM usada | 3169 B | 1753 B | 287 B |
| Fuentes | 3 | **1** (4×6) | 1 (caracteres) |
| Mazo del blackjack | 52 cartas, 2 bytes/carta | 52 cartas, 1 byte/carta | cartas de 1 a 10 |
| Serpiente | 40 segmentos, float | 24 segmentos, `int16_t` | 32 celdas, 1 byte/celda |
| Palos de las cartas | 4 figuras rellenas | 4 figuras sencillas | No hay cartas dibujadas |

## 18. Limitaciones conocidas

* Solo 6 pines usados de los muchos disponibles; se podrían añadir más botones.
* Los botones van con cable hasta el pin: con cables largos el antirrebote de
  25 ms puede no ser suficiente.
* La placa necesita alimentación externa (batería al VIN); el OLED va a 3,3 V.

## Resumen

`tinyPocket.ino` en esta carpeta es la versión completa de TinyPocket para
Arduino Mega 2560: menú con marcador animado, Ping Pong, Breakout con colisiones
en cuatro direcciones, mascota con 37 fases de animación, Blackjack con mazo
real y cartas animadas, Snake con control por botones e información. Todo sin
`delay()` en el bucle y con el búfer de pantalla completo.
