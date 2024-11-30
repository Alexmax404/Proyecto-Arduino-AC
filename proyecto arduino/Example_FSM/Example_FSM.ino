/*!
\file   Example.ino
\date   2024-30-11
\author Glenn Ward , Jose Rodrigez, Camilo Benavidez
\brief  Maquina de estados

\par Copyright
La información contenida en este documento es propiedad y constituye valiosos 
secretos comerciales confidenciales de Unicauca, y está sujeta a restricciones 
de uso y divulgación.

\par
Copyright (c) Unicauca 2024. Todos los derechos reservados.

\par
Los avisos de derechos de autor anteriores no evidencian ninguna publicación 
real o prevista de este material.
*******************************************************************************/

#include <Keypad.h>
#include <string.h>

// Definición de pines para el buzzer y las luces
#define buzzer_pin 1 ///< Pin del buzzer
#define red_light 46 ///< Pin de la luz roja
#define green_light 48 ///< Pin de la luz verde
#define blue_light 50 ///< Pin de la luz azul
#include "pinout.h"

// Sensor DHT para temperatura y humedad
#include "DHT.h"
DHT dht(DHT11_PIN, DHTTYPE); ///< Inicialización del sensor DHT

// Biblioteca para el control del LCD
#include <LiquidCrystal.h>
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); ///< Configuración del LCD

// Configuración del teclado matricial
char password[5] = "2224"; ///< Contraseña correcta
char key1[5] = ""; ///< Buffer para almacenar la entrada del usuario
const byte ROWS = 4; ///< Número de filas del teclado
const byte COLS = 4; ///< Número de columnas del teclado
char keys[ROWS][COLS] = { ///< Mapeo de teclas
    {'1', '2', '3', 'A'},
    {'4', '5', '6', 'B'},
    {'7', '8', '9', 'C'},
    {'*', '0', '#', 'D'}
};
byte rowPins[ROWS] = {24, 26, 28, 30}; ///< Pines de las filas
byte colPins[COLS] = {32, 34, 36, 38}; ///< Pines de las columnas

Keypad keypad = Keypad(makeKeymap(keys), rowPins, colPins, ROWS, COLS);

unsigned char idx = 0; ///< Índice para la entrada de contraseña
unsigned char idx2 = 0; ///< Índice auxiliar para tareas varias
int c = 0; ///< Contador de intentos incorrectos
int intentos_alerta = 0; ///< Contador para alertas

// Macros para depuración
#define DEBUG(a) \
  Serial.print(millis()); \
  Serial.print(": "); \
  Serial.println(a);

// Tareas asíncronas
#include "AsyncTaskLib.h"

// Declaración de funciones y tareas
void timeout_T1();
AsyncTask tiempo_espera(5000, false, timeout_T1);

void measure_Temp();
AsyncTask Temperatura_Ambiente(500, true, measure_Temp);

void seguridad(void);
AsyncTask contrasenia(100, true, seguridad);

void lectura_sensores();
AsyncTask tarea_sensores(500, true, lectura_sensores);

void timeout_T2();
AsyncTask tiempo_espera_eventos(3000, false, timeout_T2);

void timeout_T3();
AsyncTask tiempo_espera_alerta_azul(3000, false, timeout_T3);

void reseteo(void);
AsyncTask tarea_reseteo(150, true, reseteo);

void bloqueado2();
AsyncTask tarea_bloqueado(150, true, bloqueado2);

AsyncTask tiempo_espera_bloqueado(7000, false, timeout_T2);

void buzzer_alerta_azul();
AsyncTask tarea_buzzer_azul(100, false, buzzer_alerta_azul);

void buzzer_alarma_roja();
AsyncTask tarea_buzzer_rojo(150, true, buzzer_alarma_roja);

// Máquina de estados
#include "StateMachineLib.h"

/*!
\enum State
Define los estados de la máquina de estados.
*/
enum State {
  Inicio = 0, ///< Estado inicial
  Monitoreo_ambiental = 1, ///< Estado de monitoreo
  Alarma_roja = 2, ///< Estado de alarma roja
  State_bloqueado = 3, ///< Estado bloqueado
  Monitor_eventos = 4, ///< Monitoreo de eventos
  Alerta_azul = 5, ///< Estado de alerta azul
};

/*!
\enum Input
Define las entradas de la máquina de estados.
*/
enum Input { 
  Sign_R = 0, ///< Entrada basura
  Sign_T = 1, ///< Entrada por tiempo
  Sign_D = 2, ///< Entrada por sensores
  Sign_P = 3, ///< Entrada por intentos fallidos
  Unknown = 4, ///< Entrada desconocida
  clavecorrecta = 5, ///< Entrada por clave correcta
  bloqueado = 6, ///< Entrada de estado bloqueado
  Sign_T2 = 7, ///< Entrada adicional de tiempo
  Sign_T3 = 8, ///< Otra entrada por tiempo
  Sign_A = 9, ///< Entrada para reinicio
};

Input currentInput, input; ///< Variables para la entrada actual

StateMachine stateMachine(6, 10); ///< Máquina de estados con 6 estados y 10 transiciones

/********************************************
* Configuración inicial del programa
*********************************************/
void setup() {
  Serial.begin(115200); ///< Inicialización de la comunicación serial

  // Configuración de pines
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(LED_BLUE_PIN, OUTPUT);
  pinMode(SENSOR_IR, INPUT);
  pinMode(SENSOR_SH, INPUT);

  pinMode(buzzer_pin, OUTPUT);
  pinMode(red_light, OUTPUT);
  pinMode(green_light, OUTPUT);
  pinMode(blue_light, OUTPUT);

  lcd.begin(16, 2); ///< Inicialización del LCD
  dht.begin(); ///< Inicialización del sensor DHT

  // Valores iniciales de los dispositivos
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_BLUE_PIN, LOW);
  digitalWrite(buzzer_pin, LOW);

  setup_State_Machine(); ///< Configuración de la máquina de estados
  stateMachine.SetState(State::Inicio, true, true); ///< Estado inicial
}

/*!
\brief Función principal del programa.
Se actualizan las tareas y la máquina de estados.
*/
void loop() {
  tiempo_espera.Update();
  Temperatura_Ambiente.Update();
  contrasenia.Update();
  tarea_sensores.Update();
  tiempo_espera_eventos.Update();
  tiempo_espera_alerta_azul.Update();
  tarea_reseteo.Update();
  tarea_bloqueado.Update();
  tiempo_espera_bloqueado.Update();
  tarea_buzzer_azul.Update();
  tarea_buzzer_rojo.Update();

  stateMachine.Update(); ///< Actualización de la máquina de estados
}

/*!
\brief Función que gestiona la seguridad.
Maneja la validación de la clave ingresada por el usuario.
*/
void seguridad(void) {
  char key = keypad.getKey();

  if (key) {
    if (idx < 4) {
      lcd.setCursor(idx, 1);
      lcd.print("*");
      key1[idx] = key;
      idx++;
    }
  }

  if (idx == 4) {
    if (strncmp(password, key1, 4) == 0) {
      lcd.clear();
      lcd.print("Clave correcta");
      digitalWrite(LED_GREEN_PIN, HIGH);
      delay(1000);
      digitalWrite(LED_GREEN_PIN, LOW);
      c = 0;
      currentInput = Input::clavecorrecta;
      contrasenia.Stop();
    } else {
      c++;
      lcd.clear();
      if (c < 3) {
        lcd.print("Clave incorrecta");
        digitalWrite(LED_BLUE_PIN, HIGH);
        delay(1000);
        digitalWrite(LED_BLUE_PIN, LOW);
      } else {
        c = 0;
        currentInput = Input::bloqueado;
      }
    }
    idx = 0;
    memset(key1, 0, sizeof(key1));
    delay(1500);
    lcd.clear();
    lcd.print("Digite la clave");
  }
}


/*!
\brief Función que maneja el timeout de T1.
Cambia la entrada actual a Sign_T2 tras el tiempo de espera.
*/
void timeout_T1() {
  currentInput = Input::Sign_T2;
}

/*!
\brief Función que maneja el timeout de T2.
Cambia la entrada actual a Sign_T tras el tiempo de espera.
*/
void timeout_T2() {
  currentInput = Input::Sign_T;
}

/*!
\brief Función que maneja el timeout de T3.
Incrementa el contador de intentos de alerta y cambia la entrada a Sign_T3.
*/
void timeout_T3() {
  intentos_alerta++;
  currentInput = Input::Sign_T3;
}

/*!
\brief Función para medir y mostrar la temperatura, humedad y luz.
Evalúa las condiciones ambientales y actualiza la entrada de la máquina de estados si es necesario.
*/
void measure_Temp() {
  float value_Hum = dht.readHumidity(); ///< Lee la humedad
  float value_Temp = dht.readTemperature(); ///< Lee la temperatura en Celsius

  // Lee el valor del fotocelda
  outputValue = analogRead(photocellPin);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Pht:");
  lcd.print(outputValue);
  lcd.setCursor(8, 0);
  lcd.print("Hm:");
  lcd.print(value_Hum);

  if (value_Temp > 30) {
    currentInput = Input::Sign_D; ///< Actualiza la entrada si la temperatura supera los 30 °C
  }

  lcd.setCursor(0, 1);
  lcd.print("TMP:");
  lcd.print(value_Temp);
  lcd.print("C");
}

/*!
\brief Función para la lectura de sensores.
Detecta cambios en los sensores IR y SH y actualiza la entrada de la máquina de estados.
*/
void lectura_sensores() {
  if (digitalRead(SENSOR_IR) == 0) {
    currentInput = Input::Sign_D;
  } else if (digitalRead(SENSOR_SH) == 1) {
    //currentInput = Input::Sign_D; // Este código está comentado pero puede activarse según el diseño
  } else {
    currentInput = Input::Unknown;
  }
}

/*!
\brief Función para reiniciar el sistema.
Detecta el ingreso del carácter `*` y reinicia el sistema.
*/
void reseteo(void) {
  char key = keypad.getKey(); ///< Lee una tecla del teclado

  if (key) {
    if (idx < 1) {
      idx++;
    }
  }

  if (idx == 1 && key == '*') {
    lcd.clear();
    lcd.print("Reiniciando");
    currentInput = Input::Sign_A; ///< Actualiza la entrada para reinicio
    tarea_reseteo.Stop(); ///< Detiene la tarea
    idx = 0; ///< Reinicia el índice
    intentos_alerta = 0; ///< Reinicia el contador de alertas
  }
}

/*!
\brief Función que maneja el estado bloqueado.
Enciende una luz roja y reproduce una melodía de bloqueo.
*/
void bloqueado2() {
  digitalWrite(LED_RED_PIN, HIGH);
  int size = sizeof(durations2) / sizeof(int);

  for (int note = 0; note < size; note++) {
    int duration = 1000 / durations2[note];
    tone(BUZZER_PIN, melody2[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
  }

  digitalWrite(LED_RED_PIN, LOW);
}

/*!
\brief Función que gestiona la alerta azul.
Enciende una luz azul y reproduce un sonido de alerta si hay intentos de acceso fallidos.
*/
void buzzer_alerta_azul() {
  if (intentos_alerta >= 2) {
    currentInput = Input::Sign_P;
    return;
  }

  int size = sizeof(durations) / sizeof(int);
  digitalWrite(LED_BLUE_PIN, HIGH);

  for (int note = 0; note < size; note++) {
    int duration = 1000 / durations[note];
    tone(BUZZER_PIN, melody[note], duration);
    int pauseBetweenNotes = duration * 1.30;
    delay(pauseBetweenNotes);
    noTone(BUZZER_PIN);
  }

  digitalWrite(LED_BLUE_PIN, LOW);
}

/*!
\brief Función que gestiona la alarma roja.
Enciende una luz roja y reproduce un sonido alternante simulando una alarma.
*/
void buzzer_alarma_roja() {
  digitalWrite(LED_RED_PIN, HIGH);

  tone(BUZZER_PIN, 1000);  // Sonido inicial de 1000 Hz
  delay(500);  // Pausa de 500 ms

  noTone(BUZZER_PIN);
  delay(500);

  tone(BUZZER_PIN, 1000);  // Reproduce el tono nuevamente
  delay(500);

  noTone(BUZZER_PIN);
  delay(500);

  digitalWrite(LED_RED_PIN, LOW);
}
