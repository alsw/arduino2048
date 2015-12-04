//Programa usado para tomar todos los headers generados por gimp (cuadros
//individuales) y consolidarlos en un solo header de salida

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char nombreArchivoSal[] = "../_2048_LCD_Nokia_6100/Sprites.h";
const char nombreArchivoEnt[] = "../Sprites/Sprites.h";

int main() {
  FILE *pArchSal;
  FILE *pArchEnt;
  unsigned int tamArchEnt;
  char *datosEnt;
  char *datosImg;
  unsigned int pIni, pFin;
  char bufferNum[33];
  unsigned int datoNum;
  unsigned int nDatos = 0;

  //Abre el archivo para escritura
  pArchSal = fopen(nombreArchivoSal, "w");
  if (!pArchSal) {
    printf("No se pudo abrir archivo de salida");
    return 0;
  }

  //Abre el archivo para lectura
  pArchEnt = fopen(nombreArchivoEnt, "r");
  if (!pArchEnt) {
    printf("No se pudo abrir archivo de entrada");
    return 0;
  }

  //Determina su tamano
  fseek(pArchEnt, 0, SEEK_END);
  tamArchEnt = ftell(pArchEnt);
  fseek(pArchEnt, 0, SEEK_SET);

  //Aloja memoria suficiente para cargarlo
  datosEnt = malloc(tamArchEnt+1);

  //Lee el contenido y le anexa un 0 terminador
  fread(datosEnt, 1, tamArchEnt, pArchEnt);
  datosEnt[tamArchEnt] = 0;

  //Busca la posicion de inicio de los datos de imagen
  datosImg = strstr(datosEnt, "static char header_data[] = {");
  datosImg = strstr(datosImg, "\t");
  datosImg++;

  //Extrae todas las lineas de texto con los datos de imagen y las traslada al
  //archivo de salida
  pIni = 0;
  pFin = 0;
  while (datosImg[pIni] != '}') {
    if (datosImg[pIni] >= '0' && datosImg[pIni] <= '9') {
      pFin = pIni;
      while(datosImg[pFin] >= '0' && datosImg[pFin] <= '9') pFin++;
      strncpy(bufferNum, &datosImg[pIni], pFin-pIni);
      bufferNum[pFin-pIni] = 0;
      datoNum = atoi(bufferNum);
      fprintf(pArchSal, "%3i,", datoNum);
      pIni = pFin;
      nDatos++;
      if (nDatos % 32 == 0) 
        fprintf(pArchSal, "\n");
    }
    else
      pIni++;
  }

/*
    //Busca la posicion despues del siguiente tabulador
    subCad1 = strstr(subCad1, "\t") + 1;
    //Si la posicion alcanza o excede el final, termina de procesar el archivo
    if () break;

    //Busca la posicion del siguiente fin de linea
    subCad2 = strstr(subCad1, "\n");
    //Determina cuantos bytes (caracteres) tiene la fila
    nBytes = (int) (subCad2 - subCad1);

    //Imprime el contenido de la fila, precedido de 2 espacios y continuado
    //por un fin de linea
    fprintf(pArchSal, "  ");
    fwrite(subCad1, 1, nBytes, pArchSal);
    fprintf(pArchSal, "\n");
  }
*/

  //Una vez terminado, desaloja la memoria y cierra el archivo de entrada
  free(datosEnt);
  fclose(pArchEnt);

  //Desaloja todos los recursos
  fclose(pArchSal);

  return 0;
}