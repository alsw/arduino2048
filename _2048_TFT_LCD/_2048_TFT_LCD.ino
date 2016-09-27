#include <Adafruit_GFX.h> // Libreria de graficos
#include <Adafruit_TFTLCD.h> // Libreria de LCD
#include <SparkFun_APDS9960.h> //Libreria del Sensor de Gestos

#define LCD_CS A3 // Definimos los pines del LCD
#define LCD_CD A2 // para poder visualizar elementos graficos
#define LCD_WR A1
#define LCD_RD A0
#define LCD_RESET A4

#define CFondo 0xbd74

int Colores[13] = {
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




//Enumeracion de los comandos de control de juego
enum COMANDO_CONTROL {
  CC_IZQUIERDA, CC_DERECHA, CC_ARRIBA, CC_ABAJO, CC_REINICIAR,
};

Adafruit_TFTLCD Pantalla(LCD_CS, LCD_CD, LCD_WR, LCD_RD, LCD_RESET); // Instancia LCD

//Instancia de objeto para la clase del sensor de gestos
SparkFun_APDS9960 Sensor = SparkFun_APDS9960();

int GrosorX = Pantalla.width() / 4 ;
int GrosorY = GrosorX;
//int GrosorY = Pantalla.height() / 4;

static int ValorCelda[4][4] = {
  { 1,  5,  9,  4},
  { 2, 6,  10,  8},
  { 3,  7,  11,  0},
  { 4, 8,  12,  0}
};



void setup() {
  //Se inicializa el puerto serie
  Serial.begin(9600);

  //La semilla del generador de numeros pseudo-aleatorios se alimenta con un pin de ADC abierto
  randomSeed(analogRead(0));

  //Se inicializa el sensor de gestos
  Sensor.init();
  Sensor.enableGestureSensor(false);
  Sensor.setLEDDrive(LED_DRIVE_12_5MA);  //Este par de calibraciones se hace porque el sensor esta muy sensible
  Sensor.setGestureGain(GGAIN_1X);       //por default. Mermar la ganancia y la luz mejora la respuesta.

  //Iniciar la pantalla con el driver correcto
  Pantalla.begin(0x9325); // Iniciamos el LCD especificando el controlador ILI9341.
  Pantalla.setRotation(2);
  Pantalla.fillScreen(CFondo);
  Dibujar();
}

void loop() {
  LeerSerial();
 // LeerGestos(); problemas I2C
  
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
        break;
      case 'a':
        Accion(CC_IZQUIERDA);
        break;
      case 's':
        Accion(CC_ABAJO);
        break;
      case 'd':
        Accion(CC_DERECHA);
        break;
      case 'r':
        Accion(CC_REINICIAR);
        break;
    }
  }
}

void Accion(COMANDO_CONTROL comando) {

}
