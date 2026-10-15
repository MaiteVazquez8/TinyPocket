# Explicación del código de TinyPocket

Este documento explica cómo está organizado `tinyPocket.ino` y cuál es la función de cada una de sus partes.

El programa está desarrollado en **C/C++ para Arduino** y está diseñado para ejecutarse en un **ATtiny85** conectado a una pantalla LCD 16×2 mediante I²C y tres botones.

## 1. Librerías

El programa utiliza las librerías necesarias para controlar el microcontrolador, la comunicación I²C y la pantalla:

```cpp
#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
```

### `Arduino.h`

Proporciona las funciones principales utilizadas por Arduino, como:

* `pinMode()`
* `digitalRead()`
* `digitalWrite()`
* `millis()`
* `random()`

### `Wire.h`

Permite utilizar el protocolo de comunicación **I²C**.

Se utiliza para comunicarse con la pantalla LCD.

### `LiquidCrystal_I2C.h`

Permite controlar la pantalla LCD 16×2 utilizando el módulo I²C.

## 2. Configuración de la pantalla

Se define la dirección I²C y las dimensiones de la pantalla:

```cpp
#define LCD_ADDRESS 0x27
#define LCD_COLS 16
#define LCD_ROWS 2
```

Después se crea el objeto que representa la pantalla:

```cpp
LiquidCrystal_I2C lcd(
  LCD_ADDRESS,
  LCD_COLS,
  LCD_ROWS
);
```

La dirección `0x27` es una de las más utilizadas por los módulos I²C.

Si el módulo utiliza `0x3F`, se debe modificar:

```cpp
#define LCD_ADDRESS 0x27
```

por:

```cpp
#define LCD_ADDRESS 0x3F
```

## 3. Pines

Los tres botones están conectados a diferentes pines del ATtiny85:

```cpp
#define PIN_BTN_LEFT  PB3
#define PIN_BTN_RIGHT PB4
#define PIN_BTN_OK    PB1
```

La distribución es:

| Pin | Función |
| --- | ------- |
| PB3 | LEFT    |
| PB4 | RIGHT   |
| PB1 | OK      |
| PB0 | SDA     |
| PB2 | SCL     |

## 4. Configuración de los botones

Los botones utilizan las resistencias internas *pull-up* del ATtiny85:

```cpp
pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
pinMode(PIN_BTN_OK, INPUT_PULLUP);
```

Esto significa que el estado eléctrico del botón es:

```text
Sin pulsar → HIGH
Pulsado    → LOW
```

Los botones se conectan entre el pin correspondiente y GND.

## 5. Antirrebote

Los pulsadores mecánicos pueden generar pequeñas variaciones eléctricas al ser presionados. Este fenómeno se conoce como **rebote** o *bounce*.

Para evitar que una sola pulsación sea interpretada como varias, el programa implementa un sistema de *debounce*.

La información de cada botón se almacena en una estructura:

```cpp
struct Button {
  uint8_t pin;
  bool stableState;
  bool lastRead;
  uint32_t lastChange;
  bool pressed;
};
```

La estructura almacena:

* El pin del botón.
* El último estado estable.
* La última lectura.
* El momento del último cambio.
* Si existe una pulsación pendiente.

El tiempo de antirrebote se establece mediante:

```cpp
#define DEBOUNCE_MS 35UL
```

La función encargada de actualizar el estado es:

```cpp
updateButton()
```

Esta función comprueba que el estado permanezca estable durante el tiempo establecido antes de considerar válida una pulsación.

## 6. Eventos de pulsación

La función:

```cpp
consumePressed()
```

permite comprobar si un botón fue presionado.

Cuando detecta una pulsación, devuelve `true` y elimina el evento para que esa misma pulsación no vuelva a procesarse.

Esto permite utilizar los botones como eventos individuales en lugar de leerlos constantemente.

## 7. Estados de la aplicación

TinyPocket utiliza una máquina de estados para determinar qué parte del programa debe ejecutarse.

Los estados principales son:

```cpp
enum AppState : uint8_t {
  STATE_MENU,
  STATE_PINGPONG,
  STATE_MASCOTA,
  STATE_BLACKJACK,
  STATE_SNAKE
};
```

Cada estado representa una sección diferente de la aplicación.

Cuando el usuario selecciona un juego, `appState` cambia al estado correspondiente.

Cuando vuelve al menú:

```cpp
appState = STATE_MENU;
```

## 8. Funciones de la pantalla

El programa utiliza funciones auxiliares para simplificar la escritura de información en la LCD.

Por ejemplo:

```cpp
clearLine()
```

permite limpiar una línea de la pantalla.

Otra función puede utilizarse para mostrar texto:

```cpp
printLine()
```

También puede existir una función para centrar texto:

```cpp
printCentered()
```

Estas funciones evitan repetir continuamente las mismas operaciones de la pantalla en cada juego.

## 9. Menú principal

El menú contiene los cuatro modos disponibles:

```cpp
const char *menuItems[] = {
  "PingPong",
  "Mascota",
  "Blackjack",
  "Snake"
};
```

La variable:

```cpp
uint8_t menuIndex = 0;
```

indica qué opción está seleccionada.

LEFT y RIGHT modifican `menuIndex`.

Cuando se presiona OK se inicia el modo seleccionado.

La función principal del menú es:

```cpp
handleMenu()
```

## 10. Ping Pong

Ping Pong utiliza diferentes variables para almacenar el estado de la partida:

* Posición de la pelota.
* Dirección de la pelota.
* Posición del jugador.
* Posición de la IA.
* Puntuación del jugador.
* Puntuación de la IA.

La puntuación se almacena en:

```cpp
uint8_t ppPlayerScore = 0;
uint8_t ppAiScore = 0;
```

La posición vertical de las paletas tiene dos posibilidades:

```text
0 → arriba
1 → abajo
```

La función:

```cpp
updatePingPong()
```

actualiza la posición de la pelota y comprueba las colisiones.

## 11. Puntuación de Ping Pong

El límite para ganar está definido por:

```cpp
#define PP_WIN_SCORE 5
```

Cuando el jugador llega a 5 puntos, gana.

Si la IA llega a 5 puntos, la partida termina con derrota.

## 12. Mascota

La mascota tiene dos estadísticas:

```cpp
uint8_t petHunger = 9;
uint8_t petHappy = 9;
```

Ambas comienzan con el valor máximo de `9`.

La función:

```cpp
handleMascota()
```

se encarga de controlar la mascota.

Entre sus tareas se encuentran:

* Leer los botones.
* Aumentar hambre.
* Aumentar felicidad.
* Reducir las estadísticas con el paso del tiempo.
* Comprobar la derrota.
* Comprobar la victoria.

## 13. Alimentar

Al presionar LEFT se alimenta a la mascota.

El valor de hambre aumenta y no puede superar `9`.

Por ejemplo:

```cpp
petHunger += 2;
```

Si el valor ya está cerca del máximo, se mantiene dentro del límite establecido.

## 14. Jugar con la mascota

Al presionar RIGHT aumenta la felicidad.

La estadística también tiene un máximo de `9`.

Esto permite mantener la mascota con vida durante más tiempo.

## 15. Pérdida de la mascota

La mascota pierde cuando alguna de sus estadísticas llega a `0`.

La condición utilizada es:

```cpp
if (petHunger == 0 || petHappy == 0)
```

En ese momento el estado de la mascota cambia a `GAME_OVER`.

## 16. Victoria de la mascota

La mascota puede ganar si permanece con vida durante el tiempo establecido.

El tiempo se define mediante:

```cpp
#define PET_SURVIVE_WIN_MS 60000UL
```

El valor corresponde inicialmente a **60 segundos**.

## 17. Blackjack

Blackjack utiliza una segunda máquina de estados para controlar las diferentes etapas de una ronda:

```cpp
enum BJPhase : uint8_t {
  BJ_PLAYER_TURN,
  BJ_DEALER_TURN,
  BJ_ROUND_OVER,
  BJ_MATCH_WIN,
  BJ_MATCH_LOSE
};
```

Los estados representan:

* Turno del jugador.
* Turno del dealer.
* Fin de una ronda.
* Victoria de la partida.
* Derrota de la partida.

## 18. Generación de cartas

Las cartas se generan mediante:

```cpp
uint8_t randomCard()
```

La función devuelve un número aleatorio entre `1` y `10`.

No se utiliza una baraja física, sino una generación aleatoria simplificada.

## 19. Turno del jugador

Durante:

```cpp
BJ_PLAYER_TURN
```

el jugador puede elegir entre pedir otra carta o plantarse.

### Hit

LEFT agrega una nueva carta:

```cpp
bjPlayer += randomCard();
```

Si el jugador supera `21`, pierde la ronda.

### Stand

RIGHT termina el turno del jugador:

```cpp
bjPhase = BJ_DEALER_TURN;
```

A partir de ese momento comienza el turno del dealer.

## 20. Turno del dealer

El dealer continúa recibiendo cartas mientras su puntuación sea menor que `17`.

Cuando alcanza `17` o más, se compara su puntuación con la del jugador.

Si el dealer supera `21`, el jugador gana la ronda.

## 21. Resultado del Blackjack

El número de victorias y derrotas se almacena en:

```cpp
uint8_t bjWins = 0;
uint8_t bjLosses = 0;
```

La partida se configura para terminar cuando uno de los dos valores alcanza `3`:

```cpp
#define BJ_TARGET_WINS 3
#define BJ_TARGET_LOSSES 3
```

Por lo tanto:

* 3 victorias → Blackjack ganado.
* 3 derrotas → Blackjack perdido.

## 22. Snake

Snake utiliza las 32 posiciones disponibles de la pantalla:

```text
16 columnas × 2 filas = 32 posiciones
```

Cada posición se representa mediante un número entre `0` y `31`.

La serpiente se almacena en:

```cpp
uint8_t snakeBody[32];
```

La longitud inicial es:

```cpp
uint8_t snakeLen = 3;
```

## 23. Dirección de Snake

La dirección se almacena en:

```cpp
uint8_t snakeDir = 1;
```

Los valores representan:

| Valor | Dirección |
| ----: | --------- |
|     0 | Arriba    |
|     1 | Derecha   |
|     2 | Abajo     |
|     3 | Izquierda |

LEFT gira hacia la izquierda y RIGHT hacia la derecha.

## 24. Movimiento de Snake

La función:

```cpp
updateSnake()
```

calcula la nueva posición de la cabeza.

Después comprueba:

* Si la cabeza salió del tablero.
* Si chocó contra el cuerpo.
* Si encontró la comida.

Si la cabeza sale del tablero o choca contra el cuerpo, se produce un `GAME OVER`.

## 25. Comida

La comida se coloca en una posición aleatoria:

```cpp
snakeFood = random(0, 32);
```

Antes de colocarla, el programa verifica que la posición no esté ocupada por la serpiente.

Para esto se utiliza:

```cpp
snakeCellUsed()
```

Cuando la serpiente come la comida, aumenta su longitud.

## 26. Victoria de Snake

El tamaño necesario para ganar se define mediante:

```cpp
#define SNAKE_WIN_LENGTH 12
```

Cuando la serpiente alcanza esa longitud, el estado cambia a victoria.

## 27. Velocidad de Snake

La velocidad depende de la longitud de la serpiente.

La función:

```cpp
snakeStepTime()
```

calcula cuánto tiempo debe transcurrir entre movimientos.

A medida que la serpiente crece, el intervalo entre movimientos disminuye.

Esto hace que el juego aumente progresivamente su dificultad.

## 28. `millis()`

Los diferentes juegos utilizan:

```cpp
millis()
```

para controlar el tiempo.

Por ejemplo:

```cpp
uint32_t now = millis();

if ((now - lastStep) >= stepTime) {
  updateSnake();
}
```

Esto permite ejecutar acciones periódicamente sin utilizar `delay()`.

El uso de `millis()` también permite continuar leyendo los botones mientras transcurre el tiempo.

## 29. `setup()`

`setup()` se ejecuta una sola vez al encender el ATtiny85.

Se encarga de inicializar:

* Los botones.
* La comunicación I²C.
* La pantalla LCD.
* El generador de números aleatorios.
* El menú inicial.

Una estructura simplificada es:

```cpp
void setup() {
  pinMode(PIN_BTN_LEFT, INPUT_PULLUP);
  pinMode(PIN_BTN_RIGHT, INPUT_PULLUP);
  pinMode(PIN_BTN_OK, INPUT_PULLUP);

  Wire.begin();

  lcd.init();
  lcd.backlight();

  randomSeed(...);

  gotoMenu();
}
```

## 30. `loop()`

`loop()` se ejecuta continuamente mientras el ATtiny85 está encendido.

Primero obtiene el tiempo actual:

```cpp
uint32_t now = millis();
```

Después actualiza el estado de los botones:

```cpp
updateButtons(now);
```

Finalmente comprueba el estado actual de la aplicación y ejecuta la función correspondiente.

La selección se realiza mediante:

```cpp
switch (appState)
```

De esta manera, solamente se ejecuta la lógica correspondiente al modo activo.

## 31. Volver al menú

Para regresar al menú se utiliza:

```cpp
gotoMenu();
```

Esta función cambia el estado actual:

```cpp
appState = STATE_MENU;
```

y prepara nuevamente la pantalla para mostrar las opciones disponibles.

## 32. Constantes de configuración

Los valores principales de los juegos se encuentran definidos mediante constantes.

Por ejemplo:

```cpp
#define PP_WIN_SCORE 5

#define PET_SURVIVE_WIN_MS 60000UL

#define BJ_TARGET_WINS 3
#define BJ_TARGET_LOSSES 3

#define SNAKE_WIN_LENGTH 12
```

Esto permite modificar la dificultad o duración de los juegos sin tener que cambiar la lógica principal.

Por ejemplo, para que Ping Pong termine al conseguir 10 puntos:

```cpp
#define PP_WIN_SCORE 10
```

## 33. Organización del código

El archivo `tinyPocket.ino` está dividido en diferentes secciones:

| Sección       | Función                           |
| ------------- | --------------------------------- |
| Librerías     | Importa las dependencias          |
| Configuración | Define pines y parámetros         |
| Botones       | Lee y procesa los pulsadores      |
| Pantalla      | Muestra información en la LCD     |
| Estados       | Controla el modo actual           |
| Menú          | Permite seleccionar un juego      |
| Ping Pong     | Implementa el juego               |
| Mascota       | Implementa la mascota virtual     |
| Blackjack     | Implementa el juego de cartas     |
| Snake         | Implementa el juego               |
| `setup()`     | Inicializa el hardware            |
| `loop()`      | Ejecuta continuamente el programa |

## 34. Conceptos utilizados

El proyecto utiliza varios conceptos importantes de programación embebida:

* Variables y constantes.
* Estructuras (`struct`).
* Enumeraciones (`enum`).
* Funciones.
* Condicionales.
* Bucles.
* `switch`.
* Máquinas de estados.
* Lectura digital.
* Comunicación I²C.
* Temporización mediante `millis()`.
* Generación de números aleatorios.
* Antirrebote de botones.

## 35. Modificación y ampliación

El programa está diseñado para poder agregar nuevos modos.

Para incorporar un nuevo juego sería necesario:

1. Crear un nuevo estado en `AppState`.
2. Crear las variables necesarias.
3. Crear una función para iniciar el juego.
4. Crear una función para actualizar su lógica.
5. Crear una función para mostrar su información.
6. Crear una función para manejar los botones.
7. Agregar el juego al menú.
8. Incorporar su estado al `switch` principal de `loop()`.

De esta forma, TinyPocket puede seguir creciendo sin tener que modificar completamente la estructura existente.

## Resumen

`tinyPocket.ino` controla todo el funcionamiento de la mini consola.

El programa utiliza una **máquina de estados** para administrar el menú y los diferentes modos, mientras que `millis()` permite controlar los tiempos de los juegos sin bloquear la ejecución.

Los tres botones permiten controlar toda la interfaz y la pantalla LCD 16×2 funciona como sistema de visualización.

Los cuatro modos disponibles son:

* Ping Pong.
* Mascota.
* Blackjack.
* Snake.