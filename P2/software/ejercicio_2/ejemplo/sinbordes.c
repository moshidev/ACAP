/*
   Línea de compilación: gcc main.c pgm.c -o main

   Ejemplo del uso de:

   pgmread
   pgmwrite

   Las matrices que contienen a las imágenes son declaradas como uchar **

   % % % % % % % %

   Ejemplo 1:

   unsigned char **Original = (unsigned char **)pgmread("entrada.pgm", &Largo, &Alto);
   La imagen PGM se lee de la línea de comando (argv[1]). La función
   pgmread regresa tres valores:

   1. la imagen leída       (Original)
   2. el largo de la imagen (Largo)
   3. el alto de la imagen  (Alto)

   % % % % % % % %

   Ejemplo 2:

   pgmwrite(Salida, "salida.pgm", Largo, Alto);
   La imagen Salida es escrita al disco con el nombre de negativo.pgm, la
   imagen resulta en formato PGM. La imagen se escribe desde el inicio (0,0)
   hasta (Largo, Alto).
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pgm.h"

void convolucion(unsigned char** Original, int** nucleo, unsigned char** Salida, int Alto, int Largo) {
  int x, y;
  int suma;
  int k = 0;
  int i, j;
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      k = k + nucleo[i][j];

  for (x = 1; x < Largo-1; x++){
    for (y = 1; y < Alto-1; y++){
      suma = 0;
      for (i = 0; i < 3; i++){
        for (j = 0; j < 3; j++){
          unsigned srci = (y-1)+i;
          unsigned srcj = (x-1)+j;
          suma = suma + Original[srci][srcj] * nucleo[i][j];
        }
      }
      if(k==0)
        Salida[y][x] = suma;
      else    
        Salida[y][x] = suma/k;
    }
  }
}

/* * * * *          * * * * *          * * * * *          * * * * */

int main(int argc, char *argv[]){
  int Largo, Alto;
  int i, j;

  if (argc != 3) {
    fprintf(stderr, "ERROR: Uso: ./%s [.pgm a difuminar] [.pgm donde escribir la salida]", argv[0]);
    return 1;
  }
  
  unsigned char** Original = pgmread(argv[1], &Largo, &Alto);
  unsigned char** Salida   = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));

  int** nucleo = (int**) GetMem2D(3, 3, sizeof(int));
  for (i = 0; i < 3; i++)
    for (j = 0; j < 3; j++)
      nucleo[i][j] = -1;
  nucleo[1][1] = 1;

  convolucion(Original, nucleo, Salida, Largo, Alto); // pgmread devuelve un Array2D con las columnas intercambiadas con las filas

  pgmwrite(Salida, argv[2], Largo, Alto);

  Free2D((void**) nucleo, 3);

  Free2D((void**) Original, Largo);
  Free2D((void**) Salida, Largo);
}
