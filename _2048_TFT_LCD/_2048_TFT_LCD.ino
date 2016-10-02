#include <Adafruit_GFX.h> // Libreria de graficos
#include <Adafruit_TFTLCD.h> // Libreria de LCD
#include <SparkFun_APDS9960.h> //Libreria del Sensor de Gestos

#define LCD_CS A3 // Definimos los pines del LCD
#define LCD_CD A2 // para poder visualizar elementos graficos
#define LCD_WR A1
#define LCD_RD A0
//OJO OJO Se modifico el escujo cambiando el pin fisico de A4 hacia 12
#define LCD_RESET 12

#define CFondo 0xbd74

static int Colores[14] = {
  0xce16,//0 0
  0xED20,
  0x1631,
  0xF900,
  0xF81F,
  0x07F7,
  0x6006,
  0x01BF,
  0xF900,
  0x87FB,
  0x6EF7,
  0xED20,
  0x6EF7,
  0xA44E
};

//Enumeracion de los estados del automata finito de juego
enum ESTADO_JUEGO {
  Ey_INACTIVO, Ey_DESPLAZAR, Ey_COMBINAR, Ey_CREAR,
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
  { 1, 0, 0, 0},
  { 1, 0, 0, 0},
  { 1, 0, 0, 0},
  { 1, 0,  0,  0}
};

int ValorCeldaPasada[4][4] = {
  { 0, 0, 0, 0},
  { 4, 2, 2, 0},
  { 0, 0, 0, 0},
  { 4, 8,  12,  0}
};

int ValorUnion[4][4] = {
  { 0, 0, 0, 0},
  { 0, 0, 0, 0},
  { 0, 0, 0, 0},
  { 0, 0, 0,  0}
};

float Puntos = 0;
float PPuntos = -1;

ESTADO_JUEGO EstadoJuego = Ey_INACTIVO;  //Estado actual del automata
DIR_MOVIMIENTO DirMovimiento;            //Direccion de movimiento cuando se desplaza


void setup() {
  //Se inicializa el puerto serie
  Serial.begin(9600);
  Serial.println("Iniciando");

  //La semilla del generador de numeros pseudo-aleatorios se alimenta con un pin de ADC abierto
  randomSeed(analogRead(A5));
  Serial.println("Bajarando el mazo");

  //Se inicializa el sensor de gestos
  Sensor.init();
  Sensor.enableGestureSensor(false);
  Sensor.setLEDDrive(LED_DRIVE_12_5MA);  //Este par de calibraciones se hace porque el sensor esta muy sensible
  Sensor.setGestureGain(GGAIN_1X);       //por default. Mermar la ganancia y la luz mejora la respuesta.
  Serial.println("Activando el sensor de Gestos");


  //Iniciar la pantalla con el driver correcto
  Pantalla.begin(0x9325); // Iniciamos el LCD especificando el controlador ILI9341.
  Pantalla.setRotation(2);
  Pantalla.fillScreen(CFondo);
  Serial.println("Activando la pantalla");
  Serial.print(Pantalla.width());
  Serial.print("x");
  Serial.println(Pantalla.height());

  // ReiniciarJuego();
  Dibujar();
}

void loop() {
  LeerSerial();//Buscas comando por el puero Serial
//  LeerGestos();//Busca comandos por el sensor de gestos

  switch (EstadoJuego) {
    case Ey_INACTIVO:

      break;
    case Ey_DESPLAZAR:
      Serial.println("Empezando..");
      char x, x_ini, x_fin, x_inc, x_bus;
      char y, y_ini, y_fin, y_inc, y_bus;
      boolean Continuar;
      Continuar = false;
      boolean SeMovio;
      Continuar = false;
      switch (DirMovimiento) {
        case DM_ARR:
          x_ini = 0; x_fin = 4; x_inc = 1; x_bus = 0;
          y_ini = 1; y_fin = 4; y_inc = 1; y_bus = -1;
          break;
        case DM_ABA:
          x_ini = 0; x_fin = 4; x_inc = 1; x_bus = 0;
          y_ini = 2; y_fin = -1; y_inc = -1; y_bus = 1;
          break;
        case DM_IZQ:
          x_ini = 1; x_fin = 4; x_inc = 1; x_bus = -1;
          y_ini = 0; y_fin = 4; y_inc = 1; y_bus = 0;
          break;
        case DM_DER:
          x_ini = 2; x_fin = -1; x_inc = -1; x_bus = 1;
          y_ini = 0; y_fin = 4; y_inc = 1; y_bus = 0;
          break;
      }
      for (x = x_ini; x != x_fin; x += x_inc) {
        for (y = y_ini; y != y_fin; y += y_inc) {
          if (ValorCelda[x][y] > 0 && ValorUnion[x][y] == 0 ) {
            if ( ValorCelda[x + x_bus][y + y_bus] > 0) {
              if ( ValorCelda[x + x_bus][y + y_bus] == ValorCelda[x][y] ) {
                ValorCelda[x][y] = 0;
                ValorCelda[x + x_bus][y + y_bus]++;
                Puntos += pow(2, ValorCelda[x + x_bus][y + y_bus]);
                SeMovio = true;
                ValorUnion[x + x_bus][y + y_bus] = 1;
              }
            }
            else {
              ValorCelda[x + x_bus][y + y_bus] = ValorCelda[x][y];
              ValorCelda[x][y] = 0;
              SeMovio = true;
            }
            DibujarCuadro(x, y, ValorCelda[x][y]);
            DibujarCuadro(x + x_bus, y + y_bus, ValorCelda[x + x_bus][y + y_bus]);
          }
        }
      }
      for (x = x_ini; x != x_fin; x += x_inc) {
        for (y = y_ini; y != y_fin; y += y_inc) {
          if (ValorUnion[x][y] == 0 && ValorCelda[x][y] > 0 && (ValorCelda[x + x_bus][y + y_bus] == 0 || ValorCelda[x + x_bus][y + y_bus] == ValorCelda[x][y]) ) {
            Continuar = true;
            Serial.println("Segir Moviendo");
            continue;
          }
        }
      }
      if (!Continuar) {
        if (SeMovio) {
          EstadoJuego = Ey_CREAR;
        }
        else {
          EstadoJuego = Ey_INACTIVO;
        }
      }
      break;
    case  Ey_CREAR:
      Serial.println("Creando");
      crearCelda();
      EstadoJuego = Ey_INACTIVO;
      for (x = x_ini; x != x_fin; x += x_inc) {
        for (y = y_ini; y != y_fin; y += y_inc) {
          ValorUnion[x][y] == 0;
        }
      }
      DibujarPuntos();//Dibuja el Marcador
      break;
  }
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
  for (int x = 0; x < 4; x++) {
    for (int y = 0; y < 4; y++) {
      DibujarCuadro(x, y, ValorCelda[x][y]);
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
    Serial.print("Mover...");
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
  if (EstadoJuego != Ey_INACTIVO) return;
  switch (comando) {
    case CC_IZQUIERDA:
      EstadoJuego = Ey_DESPLAZAR;
      DirMovimiento = DM_IZQ;
      break;
    case CC_DERECHA:
      EstadoJuego = Ey_DESPLAZAR;
      DirMovimiento = DM_DER;
      break;
    case CC_ARRIBA:
      EstadoJuego = Ey_DESPLAZAR;
      DirMovimiento = DM_ARR;
      break;
    case CC_ABAJO:
      EstadoJuego = Ey_DESPLAZAR;
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
  EstadoJuego = Ey_INACTIVO;
  Pantalla.fillScreen(CFondo);
  crearCelda();
  crearCelda();
  Dibujar();
}

void crearCelda() {
  bool Encontrado = false;
  do {
    int i = random(4);
    int j = random(4);
    if (ValorCelda[i][j] == 0) {
      ValorCelda[i][j] = (rand() % 10) / 9 + 1;;
      Encontrado = true;
    }
  } while (!Encontrado);
}
