#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "pgm.h"

#define RANK_MASTER     0

struct img_size {
    uint64_t rows;
    uint64_t cols;
};

struct img_channel {
    uint8_t* data;
    struct img_size len;
};

struct kernel {
    int* data;
    int side_len;
};

void convolucion(const struct img_channel* in, const struct kernel* kern, struct img_channel* out) {
    const int (*kernd)[kern->side_len] = kern->data;
    int k = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            k = k + kernd[i][j];
        }
    }

  uint8_t (*ind)[in->len.cols] = in->data;
  uint8_t (*outd)[out->len.cols] = out->data;
  for (int x = 1; x < in->len.cols-1; x++){
    for (int y = 1; y < in->len.rows-1; y++){
      int suma = 0;
      for (int i = 0; i < 3; i++){
        for (int j = 0; j < 3; j++){
            suma = suma + ind[(y-1)+j][(x-1)+i] * kernd[i][j];
        }
      }
      if(k==0)
        outd[y][x] = suma;
      else
        outd[y][x] = suma/k;
    }
  }
}

static void convolucion_paralelizada(unsigned char** orig, int** kern, unsigned char** dest, size_t rows, size_t cols) {
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    struct img_size img_size;
    int* lkern;
    MPI_Alloc_mem(3*3*sizeof(int), MPI_INFO_NULL, &lkern);
    if (rank == RANK_MASTER) {
        img_size.rows = rows;
        img_size.cols = cols;
        memcpy(lkern, kern[0], 3*3*sizeof(int));
    }
    MPI_Bcast(&img_size, 2, MPI_UINT64_T, RANK_MASTER, MPI_COMM_WORLD);
    MPI_Bcast(lkern, 3*3, MPI_INT, RANK_MASTER, MPI_COMM_WORLD);
    struct kernel lkern_c = { .data = lkern, .side_len = 3 };
    MPI_Barrier(MPI_COMM_WORLD);

    rows = img_size.rows;
    cols = img_size.cols;
    size_t rows_per_proc = rows / nprocs;

    uint8_t* lorig = 0;
    MPI_Alloc_mem(rows_per_proc*cols*sizeof(uint8_t), MPI_INFO_NULL, &lorig);
    struct img_channel lorig_c = {
        .data = lorig,
        .len = { .rows = rows_per_proc, .cols = cols }
    };
    MPI_Scatter(rank == RANK_MASTER ? orig[0] : 0, rows_per_proc*cols, MPI_UNSIGNED_CHAR, lorig, rows_per_proc*cols, MPI_UNSIGNED_CHAR, RANK_MASTER, MPI_COMM_WORLD);

    uint8_t* ldest;
    MPI_Alloc_mem(rows_per_proc*cols*sizeof(uint8_t), MPI_INFO_NULL, &ldest);
    struct img_channel ldest_c = {
        .data = ldest,
        .len = { .rows = rows_per_proc, .cols = cols }
    };

    convolucion(&lorig_c, &lkern_c, &ldest_c);

    MPI_Gather(ldest, rows_per_proc*cols, MPI_UNSIGNED_CHAR, rank == RANK_MASTER ? dest[0] : 0, rows_per_proc*cols, MPI_UNSIGNED_CHAR, RANK_MASTER, MPI_COMM_WORLD);

    MPI_Free_mem(lorig);
    MPI_Free_mem(lkern);
    MPI_Free_mem(ldest);
}

int main(int argc, char *argv[]){
    int Largo, Alto;
    int i, j;

    if (argc != 3) {
        fprintf(stderr, "ERROR: Uso: ./%s [.pgm a difuminar] [.pgm donde escribir la salida]\n", argv[0]);
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
