#include <Arduino.h>
#include "LCD_S1D15G10.h"

//Definicion de los pines de la pantalla y las mascaras de bits asociadas al puerto
const int pinLCD_nRES = 3;
#define bit_nRES 0b00001000
const int pinLCD_SI = 4;
#define bit_SI   0b00010000
const int pinLCD_SCL = 5;
#define bit_SCL  0b00100000
const int pinLCD_nCS = 6;
#define bit_nCS  0b01000000

//Definicion de los comandos de la pantalla LCD
const byte cmdLCD_DISCTL = 0xCA;
const byte cmdLCD_COMSCN = 0xBB;
const byte cmdLCD_OSCON = 0xD1;
const byte cmdLCD_SLPOUT = 0x94;
const byte cmdLCD_PWRCTR = 0x20;
const byte cmdLCD_DISINV = 0xA7;
const byte cmdLCD_DATCTL = 0xBC;
const byte cmdLCD_VOLCTR = 0x81;
const byte cmdLCD_DISON = 0xAF;
const byte cmdLCD_RGBSET8 = 0xCE;
const byte cmdLCD_PASET = 0x75;
const byte cmdLCD_CASET = 0x15;
const byte cmdLCD_RAMWR = 0x5C;

//Arreglo en memoria flash con los sprites
PROGMEM const byte img[16 * 32 * 32] = {
  #include "Sprites.h"
};

//Declaracion previa de las funciones locales
static inline void escribirComandoLCD(byte comando);
static inline void escribirDatoLCD(byte dato);

void rect(int x, int y, int ancho, int alto, int color)
{
  unsigned int i;
  unsigned int nPix;

  //Se elije el rango de direcciones de pagina/posicon
  //vertical (PASET)
  escribirComandoLCD(cmdLCD_PASET);
  escribirDatoLCD(y+1);
  escribirDatoLCD(y+alto+1-1);

  //Se hace el mapeo para las columnas/posicion horizontal
  //(CASET)
  escribirComandoLCD(cmdLCD_CASET);
  escribirDatoLCD(x+3);
  escribirDatoLCD(x+ancho+3-1);

  //Se calcula la cantidad total de pixeles a dibujar
  nPix = ancho*alto;

  //Se envian los datos a la memoria (RAMWR)
  escribirComandoLCD(cmdLCD_RAMWR);
  for (i=0; i<nPix; i++)
    escribirDatoLCD(color);
}

void sprite(byte n, byte x, byte y) {
  unsigned int offsetSpr = n<<10;  //Multiplica por 1024
  unsigned int i;

  escribirComandoLCD(cmdLCD_PASET);
  escribirDatoLCD(y+1);
  escribirDatoLCD(y+1+32-1);

  escribirComandoLCD(cmdLCD_CASET);
  escribirDatoLCD(x+3);
  escribirDatoLCD(x+34);

  escribirComandoLCD(cmdLCD_RAMWR);
  for (i = 0; i<1024; i++) {
    escribirDatoLCD(pgm_read_byte(&img[offsetSpr+i]));
  }
}

void limpiarPantalla(byte color) {
  unsigned int i;
  const unsigned int nPix = 130*130;

  //Se elije el rango de direcciones de pagina/posicon
  //vertical (PASET)
  escribirComandoLCD(cmdLCD_PASET);
  escribirDatoLCD(0);
  escribirDatoLCD(129);

  //Se hace el mapeo para las columnas/posicion horizontal
  //(CASET)
  escribirComandoLCD(cmdLCD_CASET);
  escribirDatoLCD(2);
  escribirDatoLCD(131);

  //Se envian los datos a la memoria (RAMWR)
  escribirComandoLCD(cmdLCD_RAMWR);
  for (i=0; i<nPix; i++)
    escribirDatoLCD(color);
}

void inicializarLCD() {
  digitalWrite(pinLCD_nRES, LOW);
  delay(100);
  digitalWrite(pinLCD_nRES, HIGH);
  delay(100);

  //Envia el comando de control de display (DISCTL)
  escribirComandoLCD(cmdLCD_DISCTL);
  escribirDatoLCD(0x00);
  escribirDatoLCD(0x20);
  escribirDatoLCD(0x00);

  //Envia el comando de escaneo comun (COMSCN)
  escribirComandoLCD(cmdLCD_COMSCN);
  escribirDatoLCD(0x01);

  //Enciende el oscilador interno de la LCD (OSCON)
  escribirComandoLCD(cmdLCD_OSCON);

  //Saca a la pantalla de estado de baja energia (SLPOUT)
  escribirComandoLCD(cmdLCD_SLPOUT);

  //Envia el comando de control de poder (PWRCTR)
  escribirComandoLCD(cmdLCD_PWRCTR);
  escribirDatoLCD(0x0F);

  //Envia el comando para invertir los colores. Si no se hace
  //los colores salen negativos (DISINV)
  escribirComandoLCD(cmdLCD_DISINV);

  //Envia el comando de control de datos (DATCTL)
  escribirComandoLCD(cmdLCD_DATCTL);
  escribirDatoLCD(0x03); //00
  escribirDatoLCD(0x00);
  escribirDatoLCD(0x01);

  //Envia el comando de control de volumen/contraste (VOLCTR)
  escribirComandoLCD(cmdLCD_VOLCTR);
  escribirDatoLCD(32);
  escribirDatoLCD(3);

  //Espera unos momentos a que la pantalla inicialice
  delay(100);

  //Luego enciende el display (DISON)
  escribirComandoLCD(cmdLCD_DISON);

  //Finalmente inicializa la paleta de colores (RGBSET8)
  escribirComandoLCD(cmdLCD_RGBSET8);
  for (byte i=0; i<8; i++)
    escribirDatoLCD(i*15/7);
  for (byte i=0; i<8; i++)
    escribirDatoLCD(i*15/7);
  for (byte i=0; i<4; i++)
    escribirDatoLCD(i*15/3);
  //Nota: La paleta de colores no es mas que un ajuste de
  //rango dinamico dentro de los mismos canales, de manera
  //que se puede mapear de forma lineal o no lineal cada
  //canal (RGB), puesto que la resolucion fisica (4 bits)
  //es mayor que la resolucion logica para 8 bits de color
  //(3 bits o 2 bits, en el caso del azul). Se elige un
  //mapeo lineal por motivos de practicidad.
}

static inline void escribirComandoLCD(byte comando) {
  //Precarga en los registros las constantes a usar para
  //manejar el puerto de I/O
  register const byte D0C0 = bit_nRES;
  register const byte D0C1 = bit_nRES | bit_SCL;
  register const byte D1C0 = bit_nRES | bit_SI;
  register const byte D1C1 = bit_nRES | bit_SI | bit_SCL;

  //Activa en bajo el chip select de la pantalla
  PORTD = bit_nRES | bit_SCL;

  //Se indica con un cero en SI que se enviara un comando,
  //a la vez que se pulsa el reloj
  PORTD = bit_nRES;
  PORTD = bit_nRES | bit_SCL;

  //Nota: El siguiente juego de instrucciones se encuentra
  //sumamente optimizado, al punto que se desenrolla el bucle
  //de 8 iteraciones y se minimiza la cantidad de saltos y
  //escrituras a puerto de I/O

  //Envia los 8 bits del dato, colocando primero a cero tanto
  //SCL como SI, y luego colocando el uno o cero logico al
  //mismo tiempo que se genera un flanco de subida de reloj
  //(Esto no deberia de funcionar asi, dado que el dato
  //deberia colocarse previamente al flanco de subida del
  //reloj, sin embargo esta estrategia de optimizacion
  //funciona, probablemente debido a que la hoja tecnica no
  //esta del todo correcta).
  //Por ota parte, notese que la escritura a I/O dentro del
  //if coloca el 1 logico, pero la siguiente escritura
  //colocara el cero logico independientemente de si se
  //cumplio el if. En ambos casos sin embargo, solo ocurrira
  //una transicion de reloj
  PORTD = D0C0;
  if (comando & 0x80) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x40) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x20) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x10) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x08) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x04) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x02) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (comando & 0x01) PORTD = D1C1;
  PORTD = D0C1;

  //Tras terminar se desactiva el chip select de la pantalla
  PORTD = bit_nRES | bit_SCL | bit_nCS;
}

static inline void escribirDatoLCD(byte dato) {
  register const byte D0C0 = bit_nRES;
  register const byte D0C1 = bit_nRES | bit_SCL;
  register const byte D1C0 = bit_nRES | bit_SI;
  register const byte D1C1 = bit_nRES | bit_SI | bit_SCL;

  //Activa el chip select de la pantalla
  PORTD = bit_nRES | bit_SCL;

  //Se indica con un uno en SI que se enviara un dato, a la
  //vez que se pulsa el reloj
  PORTD = bit_nRES | bit_SI;
  PORTD = bit_nRES | bit_SI | bit_SCL;

  //De manera similar a la rutina de envio de comandos, se
  //envia los 8 bits del dato
  PORTD = D0C0;
  if (dato & 0x80) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x40) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x20) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x10) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x08) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x04) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x02) PORTD = D1C1;
  PORTD = D0C1;

  PORTD = D0C0;
  if (dato & 0x01) PORTD = D1C1;
  PORTD = D0C1;

  //Tras terminar se desactiva el chip select de la pantalla
  PORTD = bit_nRES | bit_SCL | bit_nCS;
}
