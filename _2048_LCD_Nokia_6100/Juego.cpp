#include <Arduino.h>
#include "Juego.h"
#include "LCD_S1D15G10.h"

//Enumeracion de los estados del automata finito de juego
enum ESTADO_JUEGO {
  EJ_INACTIVO, EJ_DESPLAZAR, EJ_COMBINAR, EJ_CREAR,
};

//Enumeracion de las direcciones de movimiento
enum DIR_MOVIMIENTO {
  DM_IZQ, DM_DER, DM_ARR, DM_ABA,
};

//Colores de las celdas para cada numero
const char paletaColor[16] = {
  146,  //2
  255,  //4
  235,  //8
  75,   //16
  31,   //32
  29,   //64
  28,   //128
  156,  //256
  252,  //512
  240,  //1024
  224,  //2048
  224,  //4096
  224,  //8192
  224,  //16384
  224,  //32768
  224,  //65536
};

//Variables de estado del automata finito de juego
bool bJuegoNuevo = true;                 //Bandera que indica se esta iniciando un juego
ESTADO_JUEGO estadoJuego = EJ_INACTIVO;  //Estado actual del automata
DIR_MOVIMIENTO dirMovimiento;            //Direccion de movimiento cuando se desplaza
bool bConsolidar = false;                //Bandera que indica si se deben consolidar las celdas al mover
bool bCombinar = false;                  //Bandera que indica si se deben animar las celdas combinadas
bool bCrear = false;                     //Bandera que indica si se debe crear una nueva celda tras mover
bool bAnimando = false;                  //Bandera que indica si se debe realizar una animacion
byte pasoAnimacion = 0;                  //Conteo del paso actual de animacion

//Arreglo con los valores de todas las celdas del tablero. Notese que solo se guardan las potencias de
//2 a la que se elevan los numeros, asi si se guarda un 2, en realidad se guarda un 1, mientras que para
//un 4 se guarda un 2, para un 1024 se guarda un 10, y asi sucesivamente. Los espacios vacios se marcan
//con valor de cero
byte valorCelda[4 * 4] = {
   0,  0,  0,  0,
   0,  0,  0,  0,
   0,  0,  0,  0,
   0,  0,  0,  0,
};

//Arreglo que marca las celdas que se estan desplazando en un momento dado
bool celdaDesplazada[4 * 4] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
};

//Arreglo que marca las celdas que fueron combinadas durante un movimiento dado
bool celdaCombinada[4 * 4] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
};

//Arreglo que marca las celdas que fueron creadas en un momento dado
bool celdaCreada[4 * 4] = {
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
  0, 0, 0, 0,
};

//Indices relativos que apuntan a celdas contiguas. Esto es una optimizacion para unidimensionalizar
//los accesos dentro de los arreglos, explotando la geometria de 4x4 de los mismos.
const char posCelIzq = -1;
const char posCelDer = 1;
const char posCelArr = -4;
const char posCelAba = 4;

//Declaracion previa de las funciones locales
static void reiniciarJuego();
static void hacerAnimacion();
static void crearCelda();
//static void dibujarTablero();

void accion(COMANDO_CONTROL comando) {
  //Si el estado del juego esta realizando una accion previa, el nuevo
  //comando se ignora
  if (estadoJuego != EJ_INACTIVO) return;

  //Si el estado de juego es inactivo, se actua de acuerdo al comando recibido
  switch (comando) {
    case CC_IZQUIERDA:
      estadoJuego = EJ_DESPLAZAR;
      dirMovimiento = DM_IZQ;
      break;
    case CC_DERECHA:
      estadoJuego = EJ_DESPLAZAR;
      dirMovimiento = DM_DER;
      break;
    case CC_ARRIBA:
      estadoJuego = EJ_DESPLAZAR;
      dirMovimiento = DM_ARR;
      break;
    case CC_ABAJO:
      estadoJuego = EJ_DESPLAZAR;
      dirMovimiento = DM_ABA;
      break;
    case CC_REINICIAR:
      reiniciarJuego();
      break;
  }
}

void hacerLogicaJuego() {
  char i, i_ini, i_fin, i_inc;
  char j, j_ini, j_fin, j_inc;
  char posCel;
  char posCelDest;

  //Se verifica si es la primera vez que se itera la logica del juego
  if (bJuegoNuevo) {
    //De ser asi, al inicio se crea una celda
    crearCelda();
    //Acto seguido se programa la creacion de la siguiente celda, lo que causara un evento de animacion
    estadoJuego = EJ_CREAR;
    bCrear = true;

    //Luego se limpia la pantalla
    rect(0, 0, 128, 128, 0xFF);

    //Finalmente se desmarca la bandera
    bJuegoNuevo = false;
  }

  switch (estadoJuego) {
    case EJ_INACTIVO:
      //En este estado no se hace nada. Reservado para acciones que ocurren solas, sin entrada de usuario
      break;
    case EJ_DESPLAZAR:
      //La logica de juego no se actualiza mientras se hace una animacion, asi se da tiempo de que las
      //animaciones terminen entre pasos de logica.
      if (!bAnimando) {
        //Las siguientes constantes establecen los parametros de iteracion y las posiciones relativas de
        //las celdas de destino hacia donde se hace el movimiento
        switch (dirMovimiento) {
          case DM_IZQ:
            i_ini = 1; i_fin = 4; i_inc = 1;    //Se iteran solo las 3 columnas de la derecha desde la izquierda
            j_ini = 0; j_fin = 4; j_inc = 1;    //Se iteran todas las filas
            posCelDest = posCelIzq;             //La celda de destino es hacia la izquierda
            break;
          case DM_DER:
            i_ini = 2; i_fin = -1; i_inc = -1;  //Se iteran solo las 3 columnas de la izquierda desde la derecha
            j_ini = 0; j_fin = 4; j_inc = 1;    //Se iteran todas las filas
            posCelDest = posCelDer;             //La celda de destino es hacia la derecha
            break;
          case DM_ARR:
            i_ini = 0; i_fin = 4; i_inc = 1;    //Se iteran todas las columnas
            j_ini = 1; j_fin = 4; j_inc = 1;    //Se iteran solo las 3 filas de abajo desde arriba
            posCelDest = posCelArr;             //La celda de destino es hacia arriba
            break;
          case DM_ABA:
            i_ini = 0; i_fin = 4; i_inc = 1;    //Se iteran todas las columnas
            j_ini = 2; j_fin = -1; j_inc = -1;  //Se iteran solo las 3 filas de arriba desde abajo
            posCelDest = posCelAba;             //La celda de destino es hacia abajo
            break;
        }

        //Se verifica si una consolidacion esta pendiente
        if (bConsolidar) {
          //De estar pendiente se desplazan todas las celdas en el arreglo valorCelda, asimismo se
          //procesan todas las posibles combinaciones

          //Se recorre el arreglo de acuerdo a los parametros de iteracion establecidos segun la
          //direccion de movimiento
          for (i=i_ini; i!=i_fin; i+=i_inc) {
            for (j=j_ini; j!=j_fin; j+=j_inc) {
              //Se calcula la posicion de la celda de origen en el arreglo
              posCel = j*4 + i;

              //Se verifica si la celda de origen esta marcada para desplazamiento
              if (celdaDesplazada[posCel]) {
                //Si esta marcada, se verifica si la celda de destino esta ocupada
                if (!valorCelda[posCel + posCelDest])
                  //Si la celda de destino esta libre, la de origen la suplanta
                  valorCelda[posCel + posCelDest] = valorCelda[posCel];
                else {
                  //Si la celda de destino esta ocupada, se combinan. Notese que los valores del
                  //arreglo en si representan las potencias de 2 de los numeros que albergan, asi
                  //que para duplicar el valor de la celda solo se incrementa la potencia
                  valorCelda[posCel + posCelDest]++;

                  //Luego se marca la celda de destino como combinada y se marca la bandera de
                  //combinacion para animar posteriormente
                  celdaCombinada[posCel + posCelDest] = true;
                  bCombinar = true;
                }

                //La celda ya se desplazo, asi que se desmarca
                celdaDesplazada[posCel] = false;

                //Asimismo su ubicacion ahora esta libre
                valorCelda[posCel] = 0;
              }
            }
          }

          //Tras terminar la consolidacion se desmarca la bandera
          bConsolidar = false;
        }

        //A continuacion se verifica si es posible comenzar o continuar moviendo celdas en la direccion
        //seleccionada

        //Se recorre el arreglo de acuerdo a los parametros de iteracion establecidos segun la direccion
        //de movimiento
        for (i=i_ini; i!=i_fin; i+=i_inc) {
          for (j=j_ini; j!=j_fin; j+=j_inc) {
            //Se calcula la posicion de la celda de origen en el arreglo
            posCel = j*4 + i;

            //Se determina si la celda de origen se puede desplazar. Para que ocurra, la misma debe estar
            //ocupada y su destino debe cumplir cualquiera de 3 condiciones:
            // - La posicion de destino esta libre
            // - La posicion de destino esta por moverse (esta marcada para desplazamiento), asi se moveran a la vez
            // - La posicion de destino tiene el mismo valor que la de origen (se combinaran), pero ninguna de las
            //   dos celdas esta marcada como combinada previamente
            if (valorCelda[posCel] && (!valorCelda[posCel + posCelDest] || celdaDesplazada[posCel + posCelDest] ||
                (valorCelda[posCel + posCelDest] == valorCelda[posCel] && !celdaCombinada[posCel] &&
                 !celdaCombinada[posCel + posCelDest])))
            {
              //Si se puede desplazar, se marca para desplazamiento
              celdaDesplazada[posCel] = true;
              //Asimismo debera consolidarse este cambio en el arreglo tras terminar la animacion
              bConsolidar = true;
            }
            else
              //Si la celda no puede desplazarse, simplemente se desmarca
              celdaDesplazada[posCel] = false;
          }
        }

        //Se determina el siguiente proceso en base a las banderas
        if (bConsolidar) {
          //Si en este proceso se programo una consolidacion (se desplazo al menos una celda), se activa
          //la bandera de animacion
          bAnimando = true;

          //Al mismo tiempo se activa la bandera de creacion, ya que mover celdas implica que se creara otra
          bCrear = true;
        }
        else if (bCombinar)
          //Sino se desplazaron celdas, pero se marco la bandera de combinacion, se pasa a ese estado
          estadoJuego = EJ_COMBINAR;
        else if (bCrear)
          //Si no se marco la bandera de combinacion, pero si la de crear, se creara una celda nueva
          estadoJuego = EJ_CREAR;
        else
          //Si no se desplazan celdas, ni se combinan, ni se crean, entonces no se pudo hacer nada,
          //asi que se pasa a inactividad
          estadoJuego = EJ_INACTIVO;
      }

      break;
    case EJ_COMBINAR:
      //La logica de juego espera a que las animaciones terminen
      if (!bAnimando) {
        //Si no se esta animando, se verifica si esta activada la bandera de combinar
        if (bCombinar) {
          //Si esta activada, se realizara la animacion
          bAnimando = true;

          //Acto seguido se desmarca la bandera de combinar
          bCombinar = false;
        }
        else {
          //Si la bandera de combinar esta desactivada, se limpian todas las celdas marcadas
          //para combinar
          for (i=0; i<16; i++)
            celdaCombinada[i] = false;

          //A continuacion se crea una nueva celda
          estadoJuego = EJ_CREAR;
        }
      }

      break;
    case EJ_CREAR:
      //La logica de juego espera a que las animaciones terminen
      if (!bAnimando) {
        //Si no se esta animando, se verifica si esta activada la bandera de crear
        if (bCrear) {
          //Si esta activada, se procedera a crear una celda nueva en un espacio vacio
          crearCelda();

          //Se marca la bandera de animacion
          bAnimando = true;

          //Se desmarca la bandera de crear
          bCrear = false;
        }
        else {
          //Si la bandera de crear esta desactivada, se limpian todas las celdas marcadas
          //para crear
          for (i=0; i<16; i++)
            celdaCreada[i] = false;

          //A continuacion se pasa a inactividad, pues crear celdas es siempre el ultimo paso
          estadoJuego = EJ_INACTIVO;
        }
      }

      break;
  }

  //Como paso siguiente de la logica, se efectuan las animaciones
  hacerAnimacion();
}

static void reiniciarJuego() {
  byte i;

  //Se limpian todos los arreglos que contienen los datos del tablero
  for (i=0; i<16; i++) {
    valorCelda[i] = 0;
    celdaDesplazada[i] = 0;
    celdaCombinada[i] = 0;
    celdaCreada[i] = 0;
  }

  //Limpia todas las variables del automata finito del juego
  bJuegoNuevo = true;
  estadoJuego = EJ_INACTIVO;
  bConsolidar = false;
  bCombinar = false;
  bCrear = false;
  bAnimando = false;
  pasoAnimacion = 0;
}

static void hacerAnimacion() {
  byte i, j;
  byte posX, posY;
  byte posCel;
  byte offsetPos;

  switch (estadoJuego) {
    case EJ_INACTIVO:
      //De momento no se hace nada, pero se reserva para animaciones latentes mientras no ocurre entrada de usuario
      break;
    case EJ_DESPLAZAR:
      //En este estado se desplazan las celdas una a una
      for (j=0; j<4; j++) {
        for (i=0; i<4; i++) {
          //Pre calcula las variables con que se operaran en la iteracion
          posCel = j*4 + i;  //Indice de la celda que se esta desplazando
          posX = i*32;       //Posicion en X de la celda (sin desplazar)
          posY = j*32;       //Posicion en Y de la celda (sin desplazar)
          offsetPos = (pasoAnimacion + 1)*8;  //Desplazamiento en pixeles

          //Solo se animan aquellas celdas que estan marcadas para desplazamiento
          if (celdaDesplazada[posCel]) {
            //Se dibuja la celda desplazada de acuerdo a la direccion de movimiento
            switch (dirMovimiento) {
              case DM_IZQ:
                //Coloca el sprite de la celda en una posicion desplazada
                sprite(valorCelda[posCel] - 1, posX - offsetPos, posY);
                //Si la celda proviene del extremo del tablero o bien la celda de donde proviene esta vacia,
                //se limpia el espacio que ocupaba
                if (i == 3 || !valorCelda[posCel + posCelDer]) rect(posX + 32 - offsetPos, posY, 8, 32, 0xFF);
                break;
              case DM_DER:
                sprite(valorCelda[posCel] - 1, posX + offsetPos, posY);
                if (i == 0 || !valorCelda[posCel + posCelIzq]) rect(posX + offsetPos - 8, posY, 8, 32, 0xFF);
                break;
              case DM_ARR:
                sprite(valorCelda[posCel] - 1, posX, posY - offsetPos);
                if (j == 3 || !valorCelda[posCel + posCelAba]) rect(posX, posY + 32 - offsetPos, 32, 8, 0xFF);
                break;
              case DM_ABA:
                sprite(valorCelda[posCel] - 1, posX, posY + offsetPos);
                if (j == 0 || !valorCelda[posCel + posCelArr]) rect(posX, posY + offsetPos - 8, 32, 8, 0xFF);
                break;
            }
          }
        }
      }

      //Se avanza la animacion un paso
      pasoAnimacion++;
      if (pasoAnimacion >= 4) {
        //Si la animacion alcanzo 4 pasos se termina la misma
        bAnimando = false;
        pasoAnimacion = 0;  //Se limpia la variable para la proxima vez
      }

      break;
    case EJ_COMBINAR:
      //En esta etapa se animan las celdas una a una
      for (j=0; j<4; j++) {
        for (i=0; i<4; i++) {
          //Pre calcula las variables con que se operaran en la iteracion
          posCel = j*4 + i;  //Indice de la celda que se esta animando
          posX = i*32;       //Posicion en X de la celda
          posY = j*32;       //Posicion en Y de la celda
          offsetPos = pasoAnimacion/2;  //Desplazamiendo del borde en pixeles

          //Solo se animan aquellas celdas que estan marcadas para combinacion
          if (celdaCombinada[posCel]) {
            //En el primer paso de animacion se dibuja el sprite completo
            if (pasoAnimacion == 0)
              sprite(valorCelda[posCel] - 1, posX, posY);

            if (pasoAnimacion < 4) {
              //En los primeros 4 pasos se anima el borde ampliado

              //Borde externo
              rect(posX + 1 - offsetPos, posY + 1 - offsetPos, 30 + offsetPos*2, 1, 0x00);   //Superior
              rect(posX + 1 - offsetPos, posY + 30 + offsetPos, 30 + offsetPos*2, 1, 0x00);  //Inferior
              rect(posX + 1 - offsetPos, posY + 1 - offsetPos, 1, 30 + offsetPos*2, 0x00);   //Izquierdo
              rect(posX + 30 + offsetPos, posY + 1 - offsetPos, 1, 30 + offsetPos*2, 0x00);  //Derecho

              //Relleno (solo bordes)
              rect(posX + 2 - offsetPos, posY + 2 - offsetPos, 28 + offsetPos*2, 1, paletaColor[valorCelda[posCel] - 1]);    //Superior
              rect(posX + 2 - offsetPos, posY + 29 + offsetPos, 28 + offsetPos*2, 1, paletaColor[valorCelda[posCel] - 1]);   //Inferior
              rect(posX + 2 - offsetPos, posY + 2 - offsetPos, 1, 28 + offsetPos*2, paletaColor[valorCelda[posCel] - 1]);    //Izquierdo
              rect(posX + 29 + offsetPos, posY + 2 - offsetPos, 1, 28 + offsetPos*2, paletaColor[valorCelda[posCel] - 1]);   //Derecho
            }
            else {
              //En los ultimos 4 pasos se anima el borde reducido

              //Borde externo
              rect(posX - 1 + offsetPos, posY - 1 + offsetPos, 34 - offsetPos*2, 1, 0x00);   //Superior
              rect(posX - 1 + offsetPos, posY + 32 - offsetPos, 34 - offsetPos*2, 1, 0x00);  //Inferior
              rect(posX - 1 + offsetPos, posY - 1 + offsetPos, 1, 34 - offsetPos*2, 0x00);   //Izquierdo
              rect(posX + 32 - offsetPos, posY - 1 + offsetPos, 1, 34 - offsetPos*2, 0x00);  //Derecho

              //Limpieza externa (color de fondo)
              rect(posX - 2 + offsetPos, posY - 2 + offsetPos, 36 - offsetPos*2, 1, 0xFF);   //Superior
              rect(posX - 2 + offsetPos, posY + 33 - offsetPos, 36 - offsetPos*2, 1, 0xFF);  //Inferior
              rect(posX - 2 + offsetPos, posY - 2 + offsetPos, 1, 36 - offsetPos*2, 0xFF);   //Izquierdo
              rect(posX + 33 - offsetPos, posY - 2 + offsetPos, 1, 36 - offsetPos*2, 0xFF);  //Derecho
            }
          }
        }
      }

      //Se incrementa la cuenta de pasos de animacion
      pasoAnimacion++;
      if (pasoAnimacion >= 8) {
        //Cuando la cuenta llega a 8, la animacion se detiene
        bAnimando = false;
        pasoAnimacion = 0;
      }

      break;
    case EJ_CREAR:
      //En esta etapa se animan las celdas una a una
      for (j=0; j<4; j++) {
        for (i=0; i<4; i++) {
          //Pre calcula las variables con que se operaran en la iteracion
          posCel = j*4 + i;  //Indice de la celda que se esta animando
          posX = i*32;       //Posicion en X de la celda animada
          posY = j*32;       //Posicion en Y de la celda animada
          offsetPos = pasoAnimacion*2;  //Desplazamiento del borde en pixeles

          //Solo se animan aquellas celdas que esten marcadas para animacion
          if (celdaCreada[posCel]) {
            if (pasoAnimacion < 6) {
              //En los primeros 6 pasos de animacion se hace crecer el borde

              //Dibuja el borde
              rect(posX + 14 - offsetPos, posY + 14 - offsetPos, offsetPos*2 + 4, 1, 0x00);  //Superior
              rect(posX + 14 - offsetPos, posY + 17 + offsetPos, offsetPos*2 + 4, 1, 0x00);  //Inferior
              rect(posX + 14 - offsetPos, posY + 14 - offsetPos, 1, offsetPos*2 + 4, 0x00);  //Izquierdo
              rect(posX + 17 + offsetPos, posY + 14 - offsetPos, 1, offsetPos*2 + 4, 0x00);  //Derecho

              //Dibuja el relleno
              rect(posX + 15 - offsetPos, posY + 15 - offsetPos, offsetPos*2+2, offsetPos*2+2, paletaColor[valorCelda[posCel] - 1]);
            }
            else
              //En el ultimo paso de animacion se dibuja el sprite completo de la celda
              sprite(valorCelda[posCel]-1, posX, posY);
          }
        }
      }

      //Incrementa la cuenta de pasos de animacion
      pasoAnimacion++;
      if (pasoAnimacion >= 7) {
        //Si la cuenta llega a 7, se termina la animacion
        bAnimando = false;
        pasoAnimacion = 0;
      }

      break;
  }
}

static void crearCelda() {
  byte i;
  char nCeldasVacias;
  byte posNuevaCelda;

  //Primero se cuenta la cantidad de celdas vacias
  nCeldasVacias = 0;
  for (i=0; i<16; i++)
    if (!valorCelda[i])
      nCeldasVacias++;

  //Se elige un numero al azar segun la cantidad de celdas vacias
  posNuevaCelda = random(nCeldasVacias);

  //Se recorre el tablero buscando la celda vacia indicada por el numero
  //generado al azar
  nCeldasVacias = 0;
  for (i=0; i<16; i++) {
    //Si la celda esta ocupada se ignora, solo cuentan las vacias
    if (!valorCelda[i]) {
      if (nCeldasVacias == posNuevaCelda) {
        //Si se esta en la posicion de celda vacia indicada por el numero
        //aleatorio, se marca la misma para creacion y se le asigna un 2
        //o 4 al azar (en realidad un 1 o un 2, dado que se usan potencias)
        celdaCreada[i] = true;
        valorCelda[i] = random(2) + 1;
        return;
      }
      else
        //Si aun no se llega a la posicion, simplemente incrementa la cantidad
        //de celdas vacias recorridas
        nCeldasVacias++;
    }
  }
}

/*
static void dibujarTablero() {
  byte i, j;

  //Esta funcion dibuja el tablero. Notese que no se usa realmente, ya que el
  //estado general del dibujo del tablero se origina y evoluciona a partir de las
  //animacion
  for (j=0; j<4; j++) {
    for (i=0; i<4; i++) {
      if (valorCelda[j*4 + i] == 0)
        rect(i*32, j*32, 32, 32, 0xFF);
      else
        sprite(valorCelda[j*4 + i] - 1, i*32, j*32);
    }
  }
}
*/
