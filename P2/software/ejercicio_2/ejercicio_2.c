#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "pgm.h"

#define RANK_MASTER     0

void convolucion(unsigned char** Original, int** nucleo, unsigned char** Salida, int Largo, int Alto) {
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
            suma = suma + Original[(x-1)+i][(y-1)+j] * nucleo[i][j];
        }
      }
      if(k==0)
        Salida[x][y] = suma;
      else
        Salida[x][y] = suma/k;
    }
  }
}

static void convolucion_paralelizada(unsigned char** orig, int** kern, unsigned char** dest, size_t cols, size_t rows) {
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == RANK_MASTER) {
        convolucion(orig, kern, dest, cols, rows);
    }
}

int main(int argc, char *argv[]){
    int Largo, Alto;
    int i, j;

    if (argc != 3) {
        fprintf(stderr, "ERROR: Uso: ./%s [.pgm a difuminar] [.pgm donde escribir la salida]", argv[0]);
        return 1;
    }

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    unsigned char** Original = 0;
    unsigned char** Salida   = 0;
    int** nucleo = 0;
    if (rank == RANK_MASTER) {
        Original = pgmread(argv[1], &Largo, &Alto);
        Salida   = (unsigned char**)GetMem2D(Largo, Alto, sizeof(unsigned char));
        nucleo = (int**) GetMem2D(3, 3, sizeof(int));
        for (i = 0; i < 3; i++) {
            for (j = 0; j < 3; j++) {
                nucleo[i][j] = -1;
            }
        }
        nucleo[1][1] = 1;
    }

    convolucion_paralelizada(Original, nucleo, Salida, Largo, Alto);

    if (rank == RANK_MASTER) {
        pgmwrite(Salida, argv[2], Largo, Alto);

        Free2D((void**) nucleo, 3);

        Free2D((void**) Original, Largo);
        Free2D((void**) Salida, Largo);
    }

    MPI_Finalize();
    return 0;
}
