# Explicación del código de TinyPocket — ATtiny85

Este documento explica cómo está organizado `src/tinyPocket.ino` en la
versión para **ATtiny85** del proyecto TinyPocket.

El microcontrolador es el **ATtiny85** (8 bits, 8 KB de flash y **512 bytes de
SRAM**), conectado a una pantalla **LCD 16×2 con módulo I²C** y tres botones.

## 1. Por qué esta versión NO usa el OLED

Las otras dos versiones del proyecto (Arduino Mega 2560 y Arduino Uno) usan un
OLED de 128×64. En el ATtiny85 eso es físicamente imposible:

| Elemento | Tamaño | ¿Cabe en 512 B de SRAM? |
| --- | --- | --- |
| Buffer de U8g2 128×64 | 1024 bytes | **No** (el doble de toda la RAM) |
| Buffer de Adafruit_SSD1306 | 1024 bytes | **No** |
| Buffer de LiquidCrystal_I2C (16×2) | 0 bytes (la librería no guarda copia) | **Sí** |

Se comprobó intentándolo: el sketch con U8g2 **no enlaza** en el ATtiny85
(`ld returned 1 exit status`). Por eso esta versión usa el LCD de caracteres de
16×2, que es lo que describe el diseño original de esta carpeta, y que además
aprovecha mejor el hardware: un LCD de caracteres se lee mucho mejor de cerca
que un OLED diminuto.

## 2. Librerías

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
```

* **`Arduino.h`**: `pinMode()`, `digitalRead()`, `millis()`, `random()`.
* **`Wire.h`**: I²C por hardware (USI del ATtiny85), SDA en PB0 y SCL en PB2.
* **`LiquidCrystal_I2C`**: controlador del LCD 16×2. Necesaria porque U8g2 y
  Adafruit_SSD1306 no caben en este microcontrolador.

## 3. Configuración de la pantalla

```cpp
#define LCD_ADDRESS 0x27
const char CH_PELOTA   = 0;   // y 7 caracteres especiales más
LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);
```

Dirección `0x27`; si tu módulo usa `0x3F`, cambia esa constante.

Los caracteres especiales se crean una vez en `crearCaracteres()` con
`createChar()`: pelota, paleta, cuerpo de serpiente, comida, cabeza, ladrillo,
corazón y ojo cerrado. Son los que permiten representar las animaciones en un
LCD que solo sabe pintar texto.

## 4. Conexiones

| Pin del ATtiny85 | Función |
| --- | --- |
| PB0 | SDA |
| PB1 | Botón OK |
| PB2 | SCL |
| PB3 | Botón izquierda |
| PB4 | Botón derecha |

Los botones usan las resistencias *pull-up* internas:

```cpp
pinMode(PIN_IZQUIERDA, INPUT_PULLUP);   // PB3
pinMode(PIN_DERECHA,   INPUT_PULLUP);   // PB4
pinMode(PIN_OK,        INPUT_PULLUP);   // PB1
```

Sin pulsar = `HIGH`, pulsado = `LOW`. Cada botón va entre su pin y GND.

> PB0 y PB2 están dedicados al I²C, y PB3/PB4/PB1 a los botones: **no queda
> ningún pin libre** en el ATtiny85. Si se quisiera añadir un pulsador más
> habría que recurrente a un pin por software o a un expansor I/O.

## 5. Antirrebote de botones

```cpp
struct Boton {
  uint8_t pin;
  bool lecturaAnterior, estadoEstable;
  uint16_t cambioLectura, inicioPulsacion;
  bool corto, largo;
};
```

Una sola función, `actualizarBoton()`, atiende los tres botones por parámetro
(un puntero), así que el código del antirrebote existe una sola vez. El estado
solo cambia si se mantiene **35 ms** estable. Al soltar:

* pulsación **corta** → `corto = true`
* pulsación de **800 ms o más** → `largo = true`

`largo` se usa para volver al menú desde cualquier juego.

## 6. Pantalla: un único buffer de 32 caracteres

```cpp
char buffer[33];
void poner(uint8_t columna, uint8_t fila, char c);
void pintar();
```

`pintar()` manda las dos filas de golpe:

```cpp
buffer[16] = '\0';
lcd.setCursor(0, 0); lcd.print(buffer);
lcd.setCursor(0, 1); lcd.print(buffer + 16);
```

Todos los juegos escriben en ese mismo buffer, así que **solo hay una función
de pintado en todo el programa**. Es el ahorro de memoria y de código que
permite que quepan cinco juegos.

### Textos en flash

```cpp
ponerTextoF(1, F("OK entra  mant: menu"));
```

Los literales van siempre con `F()`. Sin `F()`, cada texto literal se copia a la
SRAM al arrancar, y en esta placa cada byte cuenta: solo con los textos del
programa eran unos 240 bytes de RAM que no había.

## 7. Estados de la aplicación

```cpp
enum Estado { MENU, SUBMENU_PINGPONG, JUEGO_PINGPONG, JUEGO_BREAKOUT,
              JUEGO_MASCOTA, JUEGO_BLACKJACK, JUEGO_SNAKE, INFORMACION };
Estado estado = MENU;
```

`loop()` decide con un `switch` qué parte se ejecuta, y cualquier juego vuelve
al menú con `volverAlMenu()`.

## 8. Menú

En 16×2 solo cabe un nombre por fila, así que el menú muestra **la opción
seleccionada centrada con un `>`**. Se avanza con izquierda/derecha y se entra
con OK.

Los nombres están en **PROGMEM** como una sola cadena separada por comas:

```cpp
const char nombresMenu[] PROGMEM = "PingPong,Breakout,Mascota,Blackjack,Snake,Info";
```

Un array de punteros a textos habría ocupado 12 bytes de punteros más unos 60 de
textos; así el menú completo no cuesta **ni un byte de RAM**.

## 9. Ping Pong

La pelota rebota en diagonal por las dos filas del LCD:

```cpp
int8_t ppPelotaX = 8, ppPelotaY = 0, ppVelX = 1, ppVelY = 1;
```

* La **paleta del jugador** va abajo (fila 1) y la mueve el jugador con
  izquierda/derecha.
* La **paleta de la máquina** va arriba (fila 1 → fila 0) y persigue la pelota.
* La pelota solo rebota si cae en la fila donde está la paleta que la alcanza
  (`abs(pelotaX - paleta) <= 1`).
* Si no la alcanza, punto para el otro. **Gana el primero en llegar a 5.**

La máquina solo persigue la pelota cuando está en la fila de arriba, así que se
puede ganar Acercándose por un lado.

## 10. Breakout compacto

El OLED tiene sitio para 8×4 = 32 ladrillos. **En 16×2 no caben**, así que esta
versión es compacta:

* La fila de arriba es una **muralla de 16 ladrillos**, un carácter cada uno.
* La paleta se mueve en la fila de abajo.
* La pelota rebota en diagonal; al llegar arriba **destruye el ladrillo** de esa
  columna y vuelve a bajar.
* Hay **3 vidas**; se gana cuando no queda ningún ladrillo.

La muralla se guarda como una **máscara de 16 bits** (un bit por ladrillo):

```cpp
uint16_t boMuralla = 0xFFFF;
```

Eso son **2 bytes en lugar de los 16** de un array de booleanos, y no hay que
recorrerlo para saber si queda alguno.

## 11. Mascota

Dos estadísticas, como en el diseño original de esta carpeta:

```cpp
uint8_t maHambre = 9, maFelicidad = 9;
```

* Cada 4 s baja un punto cada estadística.
* **LEFT** alimenta (+2 de hambre), **RIGHT** juega (+2 de felicidad).
* Se **pierde** si alguna llega a 0; se **gana** si se sobreviven 60 s.

La cara se dibuja en la fila de arriba con caracteres y cambia sola cada 700 ms
(normal, guiño, feliz, cansado y durmiendo), usando el carácter especial de ojo
cerrado. Las barras de estado se dibujan en la fila de abajo.

## 12. Blackjack

Cartas de 1 a 10, con el as valiendo 11 (y bajando a 1 si el total pasa de 21):

```cpp
uint8_t bjValorCarta(uint8_t valor) { return valor == 1 ? 11 : valor; }
```

* **LEFT** = pedir carta, **RIGHT** = plantarse.
* Hasta 5 cartas por jugador. La segunda carta de la banca sale como `?` hasta
  el final.
* Se reparten **de una en una cada 600 ms**, sin bloquear el programa.
* Al plantarse (o al pasarse) **la banca pide cartas hasta llegar a 17**, que es
  la regla del blackjack. Antes la banca se quedaba con sus dos cartas: eso se
  corrigió.
* La partida termina al ganar o perder **3 rondas**. El marcador `victorias-derrotas`
  se ve junto al resultado de cada ronda; al llegar a 3, OK arranca una
  **revancha** que pone el marcador a cero, y mientras la partida sigue viva OK
  reparte cartas nuevas conservando el marcador.

## 13. Snake

El tablero son las **32 celdas del LCD**: 16 columnas × 2 filas. Cada celda se
número del 0 al 31 y el cuerpo se guarda con **un byte por segmento**:

```cpp
uint8_t snCuerpo[24];
```

* **LEFT** gira a la izquierda, **RIGHT** gira a la derecha.
* **OK** pausa, **mantener OK** vuelve al menú.
* **No se permite el giro de 180°** (`girarSnake()` lo descarta): en un tablero
  de dos filas, dar media vuelta mete la cabeza dentro del propio cuerpo y
  termina la partida al instante.
* La colisión con el cuerpo se busca desde el segmento 2, porque los dos
  primeros están pegados a la cabeza por diseño.
* La velocidad sube cuando la serpiente pasa de 8 segmentos.
* Se **gana** al llegar a 12 de longitud.

## 14. `millis()` en lugar de `delay()`

Nada de `delay()` en el bucle. Cada juego decide cuándo moverse con su propio
`millis()`:

```cpp
if (millis() - ultimoFrame < FRAME_TIME) return;
```

El único `delay()` del programa está en `setup()`, para mostrar la pantalla de
inicio unos 600 ms.

### Las marcas de tiempo son de 32 bits

Todas las variables que guardan un instante (`ppUltimoPaso`, `boUltimoPaso`,
`maUltimoTic`, `bjUltimoPaso`, `snUltimoPaso` y las dos del antirrebote) son
`unsigned long`, no `uint16_t`.

No es un detalle menor: con `uint16_t`, en cuanto `millis()` pasa de 65535 ms
(unos 65 segundos de funcionamiento) el valor guardado se recorta, y la resta
`ahora - marca` da un número enorme en vez del intervalo real. El resultado es
que **todos los juegos perdían el límite de fotogramas** y se movían a la
velocidad del bucle. Cuesta 24 bytes de SRAM y unos 180 de flash, y con el
margen que hay se paga sin problema.

## 15. Cómo compilar

| Parámetro | Valor |
| --- | --- |
| Placa | `ATtiny25/45/85 (No bootloader)` |
| Core | `ATTinyCore:avr` (1.5.2) de Spence Konde |
| Reloj | 8 MHz interno (opción por defecto de la placa) |
| Programador | ISP, o micronucleus/DigiSpark con la variante `attinyx5micr` |

Con `arduino-cli`:

```bash
arduino-cli compile -b "ATTinyCore:avr:attinyx5:chip=85" src
```

## 16. Memoria: números medidos

Los que da el compilador, no estimaciones:

| Programa | Flash | % | SRAM | % |
| --- | --- | --- | --- | --- |
| `src/tinyPocket.ino` | 5622 B | 69 % | 267 B | 52 % |
| `src/examples/ejemploPelotaRebote.ino` | 3162 B | 39 % | 151 B | 29 % |
| `src/examples/ejemploPingPong.ino` | 3074 B | 38 % | 145 B | 28 % |
| `src/examples/ejemploBreakout.ino` | 3250 B | 40 % | 154 B | 30 % |
| `src/examples/ejemploSnake.ino` | 3364 B | 41 % | 171 B | 33 % |
| `src/examples/ejemploBlackjack.ino` | 3298 B | 40 % | 137 B | 27 % |
| `src/examples/ejemploOjosMascota.ino` | 3500 B | 43 % | 276 B | 54 % |

El programa principal está al **69 % de la flash** y al **52 % de la SRAM**, así que
todavía queda margen para añadir cosas.

## 17. Reglas para modificar este archivo

1. **Todos los tipos y prototipos van antes de la primera función.** El Arduino
   IDE inserta los prototipos que genera justo después de los `#include`; si un
   `enum` o un `struct` aparece más abajo, genera declaraciones rotas y la
   compilación falla con `'X' was not declared in this scope`.
2. **Sin `float`.** El ATtiny85 no tiene FPU: cada operación en coma flotante
   arrastra código y ralentiza. Todo el programa usa enteros.
3. **Sin `String` ni concatenaciones dinámicas.** Se reserva `char[]` en la
   pila y se reutilizan en cada fotograma.
4. **Todo texto literal con `F()`.**
5. Un solo buffer de pantalla y una sola función de pintado.

## 18. Limitaciones conocidas

* **No hay OLED en esta versión** por falta de RAM. Ver el apartado 1.
* El **Breakout es compacto** (16 ladrillos, no 32) porque no caben más celdas.
* El **Snake juega en 32 celdas**, que es el tablero completo del LCD; con
  Buffers del OLED el mismo juego tiene cientos de casillas.
* No quedan pines libres para más de tres botones.
* La flash quedó al 69 % tras las correcciones: hay margen, pero sigue siendo la
  versión más apretada por número de bytes.

## 19. Organizaciones y recursos

| Sección | Función |
| --- | --- |
| Librerías y configuración | Includes, pines, pantalla |
| Tipos | `Boton`, `Estado`, caracteres especiales |
| Prototipos | Declaraciones de todas las funciones |
| Botones | Antirrebote y pulsaciones cortas/largas |
| Pantalla | Buffer, `poner()`, `pintar()`, `createChar()` |
| Menú | Navegación con y sin scroll |
| Ping Pong | Rebote diagonal con máquina |
| Breakout | Muralla de 16 bits y vidas |
| Mascota | Estadísticas y cara animada |
| Blackjack | Reparto temporizado y partida |
| Snake | 32 celdas controlado por botones |
| `setup()` / `loop()` | Inicialización y máquina de estados |

## Resumen

`tinyPocket.ino` en esta carpeta es la versión **más compacta** de TinyPocket:
las mismas seis funciones (menú, Ping Pong, Breakout, Mascota, Blackjack, Snake
e Información) adaptadas a lo que de verdad caben en 512 bytes de RAM y 8 KB de
flash, con los mismos tres botones y los mismos comandos.