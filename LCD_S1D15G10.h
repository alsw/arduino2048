#ifndef LCD_S1D15G10_h_Incluida
#define LCD_S1D15G10_h_Incluida

#include <Arduino.h>

//Pines usados por la pantalla
extern const int pinLCD_nCS;
extern const int pinLCD_SCL;
extern const int pinLCD_SI;
extern const int pinLCD_nRES;

//Funciones exportadas por el modulo
void rect(int x, int y, int ancho, int alto, int color);
void sprite(byte n, byte x, byte y);
void limpiarPantalla(byte color);
void inicializarLCD();

#endif //LCD_S1D15G10_h_Incluida
