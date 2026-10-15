# TinyPocket

TinyPocket es una mini consola portátil con **menú, cinco minijuegos, una
mascota virtual y tres botones**. El proyecto mantiene **tres versiones**,
una por microcontrolador, cada una adaptada a los recursos reales de su placa.

## Las tres versiones

Cada carpeta del proyecto corresponde a un microcontrolador distinto, según el
nombre de la carpeta. **No son tres copias del mismo código**: son tres
adaptaciones, con pantalla, pines, memoria y animaciones distintas.

| Carpeta | Microcontrolador | Pantalla | I2C | Botones |
| --- | --- | --- | --- | --- |
| `Mega 2560/` | ATmega2560 (256 KB flash, 8 KB SRAM) | OLED 128×64, búfer completo | D20 / D21 | D2 / D3 / D4 |
| `Arduino Uno/` | ATmega328P (32 KB flash, 2 KB SRAM) | OLED 128×64, búfer de 1 página | A4 / A5 | D2 / D3 / D4 |
| `ATtiny85/` | ATtiny85 (8 KB flash, **512 B SRAM**) | **LCD 16×2** de caracteres | PB0 / PB2 | PB3 / PB4 / PB1 |

### Código final de TinyPocket

Uno en cada carpeta, en `src/tinyPocket.ino`. Los tres tienen las mismas seis
funciones:

| Menú | Función |
| --- | --- |
| Ping Pong | Pelota, paleta y rebote en las cuatro paredes |
| Breakout | Ladrillos, colisiones en cuatro direcciones y vidas |
| Mascota | Estadísticas, cuatro acciones y cara animada |
| Blackjack | Mazo, reparto, banca hasta 17 y partida a 3 victorias |
| Snake | Controlado por el jugador |
| Info | Pantalla de información |

### Ejemplos de animación

Seis ejemplos independientes en `src/examples/` de cada carpeta, pensados para
ver una animación sola, sin botones y sin bloquear el programa:

| Ejemplo | Qué muestra |
| --- | --- |
| `ejemploPelotaRebote.ino` | Pelota rebotando, con estela y latido en el impacto |
| `ejemploPingPong.ino` | Rebote, estela y ángulo de salida según dónde golpea |
| `ejemploBreakout.ino` | Colisión con ladrillos desde las cuatro direcciones |
| `ejemploSnake.ino` | Movimiento suave, cabeza con ojos y comida que late (**con IA**) |
| `ejemploBlackjack.ino` | Reparto de cartas volando desde los laterales |
| `ejemploOjosMascota.ino` | Las 37 fases de la cara de la mascota |

> `ejemploSnake.ino` lleva IA a propósito: su función es ver la animación. El
> Snake jugable, sin IA y sin giros de 180°, está en `tinyPocket.ino`.

## Por qué el ATtiny85 no usa OLED

El búfer de un OLED de 128×64 ocupa **1024 bytes**, y el ATtiny85 tiene
**512 bytes de SRAM en total**: no cabe ni de lejos. Se comprobó compilando y
el programa no enlaza. Por eso esa versión usa el **LCD 16×2 de caracteres**, que
no necesita búfer, y representa las animaciones con caracteres especiales
(`createChar`). Es, además, la versión más portable: 8 KB de flash, 512 B de RAM y
5 V.

## Memoria de los códigos finales

Números reales dados por el compilador:

| Versión | Flash | % | SRAM | % |
| --- | --- | --- | --- | --- |
| Mega 2560 | 42112 B | 17 % | 3220 B | 39 % |
| Arduino Uno | 31218 B | **97 %** | 1240 B | 61 % |
| ATtiny85 | 5622 B | 69 % | 267 B | 52 % |

El Mega va sobrado. El ATtiny85 tiene margen de sobra. El Uno es el
apretado: casi sin flash, aunque su RAM se sacó de aprietos pasando todos los
textos a flash con `F()`. Antes de añadir una función nueva hay que mirar estas
filas.

## Cómo compilar

```bash
# Mega 2560
arduino-cli compile -b arduino:avr:mega "Mega 2560/src"

# Arduino Uno
arduino-cli compile -b arduino:avr:uno "Arduino Uno/src"

# ATtiny85 (core ATTinyCore de Spence Konde, 8 MHz interno)
arduino-cli compile -b "ATTinyCore:avr:attinyx5:chip=85" "ATtiny85/src"
```

Para compilar un ejemplo concreto hay que abrir **solo ese `.ino`**: en Arduino
IDE, *Archivo → Abrir* (o *Sketchbook* en arduino-cli), porque un sketch solo
puede tener un `.ino` principal.

### Librerías necesarias

| Librería | Para qué |
| --- | --- |
| **U8g2** (2.36.19) | OLED 128×64 en el Mega y el Uno |
| **LiquidCrystal_I2C** (1.1.2) | LCD 16×2 en el ATtiny85 |

Núcleo: `arduino:avr` para el Mega y el Uno; `ATTinyCore:avr` para el ATtiny85.

## Controles

| Modo | Izquierda | Derecha | OK corto | Mantener OK |
| --- | --- | --- | --- | --- |
| Menú | Anterior | Siguiente | Entrar | — |
| Submenú | Cambiar | Cambiar | Entrar | Volver al menú |
| Ping Pong | Mover paleta izq. | Mover paleta der. | — | Volver al menú |
| Breakout | Mover paleta izq. | Mover paleta der. | Reiniciar al acabar | Volver al menú |
| Mascota | Comer | Jugar | Actuar | Volver al menú |
| Blackjack | Pedir carta | Plantarse | Otra ronda | Volver al menú |
| Snake | Girar a la izquierda | Girar a la derecha | Pausa / reiniciar | Volver al menú |

Todos los botones usan las resistencias *pull-up* internas: **sin pulsar = HIGH,
pulsado = LOW**, y cada botón va entre su pin y GND.

## Estructura del repositorio

```text
TinyPocket/
├── README.md
│
├── Mega 2560/
│   ├── img/circuitoDiagrama.png
│   ├── img/Simulacion/
│   └── src/
│       ├── tinyPocket.ino          <- código final
│       ├── explicacion_codigo.md   <- cómo funciona
│       └── examples/               <- 6 ejemplos de animación
│
├── Arduino Uno/                     <- misma estructura
│   ├── Img/
│   └── src/
│       ├── tinyPocket.ino
│       ├── explicacion_codigo.md
│       └── examples/
│
└── ATtiny85/
    ├── explicacion_codigo.md
    └── src/
        ├── tinyPocket.ino
        └── examples/
```

## Documentación

Cada versión tiene su propio `explicacion_codigo.md` con el detalle de sus
adaptaciones:

* `Mega 2560/src/explicacion_codigo.md`
* `Arduino Uno/src/explicacion_codigo.md`
* `ATtiny85/explicacion_codigo.md`

## Reglas de mantenimiento

1. **Tipos y prototipos antes de la primera función.** El Arduino IDE inserta
   los prototipos que genera justo después de los `#include`. Si un `enum` o un
   `struct` aparece más abajo, genera declaraciones rotas y la compilación falla
   con `'X' was not declared in this scope`. Ya ha pasado en este proyecto.
2. **Ningún `delay()` en el bucle.** Las animaciones van con `millis()`; los
   `delay()` solo en `setup()`.
3. **Los ejemplos no dependen del código final.** Cada uno se compila por
   separado.
4. **Medir la memoria después de cada cambio**, sobre todo en el Uno y el
   ATtiny85, que van al 96 % y al 97 % de su flash.

## TinyPocket

**Mini consola portátil.** Una versión para Arduino Mega 2560, otra para Arduino
Uno y otra para ATtiny85, con las mismas funciones adaptadas a cada placa.