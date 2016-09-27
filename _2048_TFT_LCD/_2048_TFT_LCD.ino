#include <Adafruit_GFX.h> // Libreria de graficos
#include <Adafruit_TFTLCD.h> // Libreria de LCD
#include <SparkFun_APDS9960.h> //Libreria del Sensor de Gestos

#define LCD_CS A3 // Definimos los pines del LCD
#define LCD_CD A2 // para poder visualizar elementos graficos
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define CFondo 0xbd74

static int Colores[13] = {
  0xce16,//0 -1
  0xFF00,//2048
  0xFF00,//8-3
  0xFF00,//16-
  0xFF00,//32
  0xFF00,//64
  0xFF00,//128
  0xFF00,//512
  0xFF00,//1024
  0xFF00,//2048
  0xFF00,//2048
  0xFF00,//2048
  0xFF00,//2048
};

//Enumeracion de los estados del automata finito de juego
enum ESTADO_JUEGO {
  EJ_INACTIVO, EJ_DESPLAZAR, EJ_COMBINAR, EJ_CREAR,
};

//Enumeracion de los comandos de control de juego
enum COMANDO_CONTROL {
  CC_IZQUIERDA, CC_DERECHA, CC_ARRIBA, CC_ABAJO, CC_REINICIAR,
};

enum DIR_MOVIMIENTO {
  DM_IZQ, DM_DER, DM_ARR, DM_ABA,
};

Adafruit_TFTLCD Pantalla(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); // Instancia LCD

//Instancia de objeto para la clase del sensor de gestos
SparkFun_APDS9960 Sensor = SparkFun_APDS9960();

static int GrosorX = Pantalla.width() / 4 ;
static int GrosorY = GrosorX;
//int GrosorY = Pantalla.height() / 4;
//x y
int ValorCelda[4][4] = {
  { 0,0,0,0},
  { 2,2,2,0},
  { 0,0,0,0},
  { 4, 8,  12,  0}
};

float Puntos = 100000;
float PPuntos = Puntos;

ESTADO_JUEGO EstadoJuego = EJ_INACTIVO;  //Estado actual del automata
DIR_MOVIMIENTO DirMovimiento;            //Direccion de movimiento cuando se desplaza


void setup() {
  //Se inicializa el puerto serie
  Serial.begin(9600);

  //La semilla del generador de numeros pseudo-aleatorios se alimenta con un pin de ADC abierto
  randomSeed(analogRead(A5));

  //Se inicializa el sensor de gestos
  Sensor.init();
  Sensor.enableGestureSensor(false);
  Sensor.setLEDDrive(LED_DRIVE_12_5MA);  //Este par de calibraciones se hace porque el sensor esta muy sensible
  Sensor.setGestureGain(GGAIN_1X);       //por default. Mermar la ganancia y la luz mejora la respuesta.

  //Iniciar la pantalla con el driver correcto
  Pantalla.begin(0x9325); // Iniciamos el LCD especificando el controlador ILI9341.
  Pantalla.setRotation(2);
  Pantalla.fillScreen(CFondo);
  //ReiniciarJuego();
  Dibujar();
}

void loop() {
  switch (EstadoJuego) {
    case EJ_INACTIVO:

      break;
    case EJ_DESPLAZAR:
      char i, i_ini, i_fin, i_inc;//i i_inicio i_fin i_incremento
      char j, j_ini, j_fin, j_inc;
      switch (DirMovimiento) {
        case DM_ARR:
          i_ini = 0; i_fin = 4; i_inc = 1;    //Se iteran todas las columnas
          j_ini = 1; j_fin = 4; j_inc = 1;    //Se iteran solo las 3 filas de abajo a arriba
          break;
        case DM_ABA:
          i_ini = 0; i_fin = 4; i_inc = 1;    //Se iteran todas las columnas
          j_ini = 2; j_fin = -1; j_inc = -1;  //Se iteran solo las 3 filas de arriba desde abajo
          break;

      }

      for (i = i_ini; i != i_fin; i += i_inc) {
        for (j = j_ini; j != j_fin; j += j_inc) {
          if (ValorCelda[i][j] > 0) {
            if ( ValorCelda[i][j - j_inc] > 0) {
              if ( ValorCelda[i][j - j_inc] == ValorCelda[i][j]) {
                ValorCelda[i][j] = 0;
                ValorCelda[i][j - j_inc]++;
              }
            }
            else {
              ValorCelda[i][j - j_inc] = ValorCelda[i][j];
              ValorCelda[i][j] = 0;
            }
          }
        }
      }
      EstadoJuego = EJ_INACTIVO;
      break;

  }
  DibujarPuntos();
  Puntos++;
  delay(100);
  Dibujar();
  LeerSerial();
  // LeerGestos(); problemas I2C
}

void DibujarPuntos() {
  if (Puntos != PPuntos) {
    PPuntos = Puntos;
    Pantalla.fillRect(GrosorX * 2 + 3, GrosorY * 4 + 3, GrosorX * 2 - 3 * 2, GrosorY - 3 * 2 , CFondo);
    Pantalla.setCursor(GrosorX * 2.5, GrosorY * 4 + 10);
    Pantalla.setTextSize(2);
    Pantalla.print("SCORE");
    Pantalla.setCursor(GrosorX * 2 + 3, GrosorY * 4.5);
    Pantalla.setTextSize(3); // Definimos tamaño del texto.
    Pantalla.setTextColor(0x00FFFF);
    Pantalla.print(Puntos, 0);
  }
}

void DibujarCuadro(int x, int y, int potencia) {
  Pantalla.fillRect(x * GrosorX + 3 , y * GrosorY + 3 , GrosorX - 3 * 2  , GrosorY  - 3 * 2, Colores[potencia]);
  if (potencia > 9) {
    Pantalla.setCursor(x * GrosorX + 6, y * GrosorY + 25);
    Pantalla.setTextSize(2); // Definimos tamaño del texto.
    Pantalla.setTextColor(0x00FFFF);
    Pantalla.print(pow(2, potencia), 0);
  }
  else if (potencia > 6) {
    Pantalla.setCursor(x * GrosorX + 5, y * GrosorY + 20);
    Pantalla.setTextSize(3); // Definimos tamaño del texto.
    Pantalla.setTextColor(0x00FFFF);
    Pantalla.print(pow(2, potencia), 0);
  }
  else  if (potencia > 3) {
    Pantalla.setCursor(x * GrosorX + 8, y * GrosorY + 15);
    Pantalla.setTextSize(4); // Definimos tamaño del texto.
    Pantalla.setTextColor(0x00FFFF);
    Pantalla.print(pow(2, potencia), 0);
  }
  else  if (potencia > 0) {
    Pantalla.setCursor(x * GrosorX + 15, y * GrosorY + 10);
    Pantalla.setTextSize(6); // Definimos tamaño del texto.
    Pantalla.setTextColor(0x00FFFF);
    Pantalla.print(pow(2, potencia), 0);
  }
}

void Dibujar() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      DibujarCuadro(i, j, ValorCelda[i][j]);
    }
  }
}

void LeerGestos() {
  //Se procede a leer el sensor y se genera un comando a la logica de juego
  //segun el gesto detectado (si se detecto uno)
  if (Sensor.isGestureAvailable()) {
    switch (Sensor.readGesture()) {
      case DIR_LEFT:
        Accion(CC_IZQUIERDA);
        break;
      case DIR_RIGHT:
        Accion(CC_DERECHA);
        break;
      case DIR_UP:
        Accion(CC_ARRIBA);
        break;
      case DIR_DOWN:
        Accion(CC_ABAJO);
      case DIR_FAR:
        Accion(CC_REINICIAR);
        break;
    }
  }
}

void LeerSerial() {
  if (Serial.available()) {
    char caracter = Serial.read();
    switch (caracter) {
      case 'w':
        Accion(CC_ARRIBA);
        Serial.println("Ariba");
        break;
      case 'a':
        Accion(CC_IZQUIERDA);
        Serial.println("Izquierda");
        break;
      case 's':
        Accion(CC_ABAJO);
        Serial.println("Abajo");
        break;
      case 'd':
        Accion(CC_DERECHA);
        Serial.println("Derecha");
        break;
      case 'r':
        Accion(CC_REINICIAR);
        Serial.println("Reiniciar");
        break;
    }
  }
}

void Accion(COMANDO_CONTROL comando) {
  if (EstadoJuego != EJ_INACTIVO) return;
  switch (comando) {
    case CC_IZQUIERDA:
      EstadoJuego = EJ_DESPLAZAR;
      DirMovimiento = DM_IZQ;
      break;
    case CC_DERECHA:
      EstadoJuego = EJ_DESPLAZAR;
      DirMovimiento = DM_DER;
      break;
    case CC_ARRIBA:
      EstadoJuego = EJ_DESPLAZAR;
      DirMovimiento = DM_ARR;
      break;
    case CC_ABAJO:
      EstadoJuego = EJ_DESPLAZAR;
      DirMovimiento = DM_ABA;
      break;
    case CC_REINICIAR:
      ReiniciarJuego();
      break;
  }
}

void ReiniciarJuego() {
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      ValorCelda[i][j] = 0;
    }
  }
  Puntos = 0;
  PPuntos = 0;
  EstadoJuego = EJ_INACTIVO;
  Pantalla.fillScreen(CFondo);
  crearCelda();
  crearCelda();
}

void crearCelda() {
  bool Encontrado = false;
  do {
    int i = random(4);
    int j = random(4);
    if (ValorCelda[i][j] == 0) {
      ValorCelda[i][j] = 1;
      Encontrado = true;
    }
  } while (!Encontrado);
}
