# TinyPocket

TinyPocket es una mini consola portátil desarrollada con un **ATtiny85**, una pantalla **LCD 16×2 con comunicación I²C** y tres botones.

El proyecto reúne diferentes minijuegos y una mascota virtual en un dispositivo de pequeño tamaño y bajo consumo, utilizando los recursos limitados del ATtiny85.

## Simulación en Tinkered

Podés acceder al proyecto y consultar la simulación desde el siguiente enlace:

**[Abrir TinyPocket en Tinkered](https://app.tinkered.ai/p/172675c8-5722-4fdb-a319-6ffd4de13691)**

## Objetivo

El objetivo de TinyPocket es desarrollar un dispositivo electrónico que combine programación embebida y electrónica mediante un sistema sencillo e interactivo.

El proyecto permite trabajar con:

- Microcontroladores.
- Programación en C/C++.
- Comunicación I²C.
- Pantallas LCD.
- Lectura de botones.
- Antirrebote de pulsadores.
- Máquinas de estados.
- Temporizadores mediante `millis()`.
- Generación de números aleatorios.
- Desarrollo de minijuegos.

## Modos de juego

### Ping Pong

Minijuego en el que el jugador debe devolver una pelota utilizando una paleta.

Controles:

- **LEFT:** mover la paleta hacia arriba.
- **RIGHT:** mover la paleta hacia abajo.
- **OK:** volver al menú.

El jugador gana cuando consigue **5 puntos**.

### Mascota

Una mascota virtual cuenta con dos estadísticas:

- Hambre.
- Felicidad.

Ambas estadísticas disminuyen con el tiempo y el jugador debe mantenerlas utilizando los botones.

Controles:

- **LEFT:** alimentar.
- **RIGHT:** jugar.
- **OK:** volver al menú.

La partida termina si alguna estadística llega a `0`.

La mascota gana si consigue sobrevivir durante el tiempo establecido.

### Blackjack

Versión simplificada del juego Blackjack.

Controles:

- **LEFT:** pedir otra carta (`Hit`).
- **RIGHT:** plantarse (`Stand`).
- **OK:** continuar la ronda o volver al menú.

El jugador gana la partida al conseguir **3 victorias** y pierde al alcanzar **3 derrotas**.

### Snake

Versión de Snake adaptada a las dimensiones de una pantalla LCD 16×2.

El tablero utiliza:

- 16 columnas.
- 2 filas.
- 32 posiciones disponibles.

Controles:

- **LEFT:** girar hacia la izquierda.
- **RIGHT:** girar hacia la derecha.
- **OK:** volver al menú.

La serpiente aumenta de tamaño al comer la comida y su velocidad aumenta progresivamente.

## Componentes

| Componente | Cantidad |
|---|---:|
| ATtiny85 | 1 |
| LCD 16×2 con módulo I²C | 1 |
| Pulsadores | 3 |
| Switch | 1 |
| Fuente de alimentación/batería | 1 |
| Cables | Según necesidad |

> **Importante:** verificar la tensión de funcionamiento de la LCD y del módulo I²C antes de realizar la conexión. Muchos módulos LCD 16×2 están diseñados para trabajar a 5 V.

## Conexiones

### ATtiny85 y LCD I²C

| ATtiny85 | LCD I²C |
|---|---|
| PB0 | SDA |
| PB2 | SCL |
| VCC | VCC |
| GND | GND |

### ATtiny85 y botones

| Botón | Pin del ATtiny85 |
|---|---|
| LEFT | PB3 |
| RIGHT | PB4 |
| OK | PB1 |

Cada botón se conecta entre su pin correspondiente y **GND**.

El programa utiliza `INPUT_PULLUP`, por lo que el estado de los botones es:

- Sin pulsar: `HIGH`.
- Pulsado: `LOW`.

El switch funciona como interruptor general de alimentación.

## Diagrama del circuito

El diagrama completo de conexiones se encuentra en:

`Img/circuitoDiagrama.png`

También se incluyen imágenes de la simulación dentro de:

`Img/Simulacion/`

## Pantalla LCD

TinyPocket utiliza una pantalla LCD 16×2 conectada mediante I²C.

La dirección I²C depende del módulo utilizado. Una dirección común es:

```cpp
#define LCD_ADDRESS 0x27
```

Algunos módulos utilizan:

```cpp
#define LCD_ADDRESS 0x3F
```

Si la pantalla no funciona, se debe comprobar la dirección I²C del módulo y modificarla en el código.

## Funcionamiento

Al encender TinyPocket se inicializa el ATtiny85, la comunicación I²C, la pantalla y los botones.

Luego se muestra el menú principal, desde donde se puede seleccionar uno de los cuatro modos:

- Ping Pong.
- Mascota.
- Blackjack.
- Snake.

LEFT y RIGHT permiten desplazarse por las opciones y OK permite seleccionar el modo.

Cuando termina una partida, el usuario puede volver al menú y seleccionar otro modo.

## Software

El proyecto está desarrollado utilizando:

- **Arduino IDE**
- **C/C++**
- **ATtiny85**
- **Wire**
- **LiquidCrystal_I2C**

## Cargar el programa

Para programar el ATtiny85:

1. Instalar Arduino IDE.
2. Instalar un core compatible con ATtiny85.
3. Seleccionar el ATtiny85 como placa.
4. Instalar las librerías necesarias.
5. Abrir `src/tinyPocket.ino`.
6. Comprobar los pines definidos en el código.
7. Comprobar la dirección I²C de la pantalla.
8. Seleccionar el programador utilizado.
9. Compilar el programa.
10. Cargarlo en el ATtiny85.

## Controles

| Modo | LEFT | RIGHT | OK |
|---|---|---|---|
| Menú | Anterior | Siguiente | Entrar |
| Ping Pong | Arriba | Abajo | Menú |
| Mascota | Alimentar | Jugar | Menú |
| Blackjack | Hit | Stand | Continuar / Menú |
| Snake | Girar izquierda | Girar derecha | Menú |

## Estructura del repositorio

```text
TinyPocket/
│
├── README.md
│
├── Docs/
│
├── Img/
│   ├── circuitoDiagrama.png
│   └── Simulacion/
│       ├── circuitoSimulado.png
│       └── circuitoSimulado2.png
│
└── src/
    ├── explicacion_codigo.md
    └── tinyPocket.ino
```

### `Docs/`

Contiene documentación adicional relacionada con el proyecto.

Puede utilizarse para almacenar información técnica, investigaciones, especificaciones o documentación del desarrollo.

### `Img/`

Contiene las imágenes utilizadas para documentar el proyecto.

`circuitoDiagrama.png` muestra el circuito y sus conexiones.

La carpeta `Simulacion` contiene imágenes del circuito simulado.

### `src/`

Contiene el código fuente y la documentación específica del programa.

#### `tinyPocket.ino`

Es el programa principal de TinyPocket.

Contiene la implementación de:

- Menú.
- Botones.
- Pantalla LCD.
- Ping Pong.
- Mascota.
- Blackjack.
- Snake.
- Estados de la aplicación.
- Temporizadores.
- Generación aleatoria.
- Lógica de los juegos.

#### `explicacion_codigo.md`

Contiene una explicación de las diferentes partes de `tinyPocket.ino`, incluyendo la inicialización, botones, menú y funcionamiento de cada modo.

## Documentación del código

Para conocer cómo está organizado y cómo funciona el código, consultá:

`src/explicacion_codigo.md`

## TinyPocket

**Mini consola portátil basada en ATtiny85.**
