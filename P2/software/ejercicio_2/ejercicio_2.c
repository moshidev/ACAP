#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include "pgm.h"

#define RANK_MASTER     0

static const int ridge_kernel_3[3][3] = {
    [0] = {-1, -1, -1},
    [1] = {-1, +1, -1},
    [2] = {-1, -1, -1},
};

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

static int kernel_sum(const struct kernel* kern) {
    const int (*kernd)[kern->side_len] = (int (*)[kern->side_len])kern->data;
    int sum = 0;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            sum = sum + kernd[i][j];
        }
    }
    return sum;
}

static int weight_entry(int y, int x, const struct img_channel* src, const struct kernel* kern) {
    const int (*kernd)[kern->side_len] = (int (*)[kern->side_len])kern->data;
    const uint8_t (*srcd)[src->len.cols] = (uint8_t (*)[kern->side_len])src->data;
    int suma = 0;
    for (int i = 0; i < kern->side_len; i++){
        for (int j = 0; j < kern->side_len; j++){
            suma = suma + srcd[(y-1)+i][(x-1)+j] * kernd[i][j];
        }
    }
    return suma;
}

void convolucion(const struct img_channel* in, const struct kernel* kern, struct img_channel* out) {
    const int (*kernd)[kern->side_len] = (int (*)[kern->side_len])kern->data;
    int kernsum = kernel_sum(kern);
    kernsum = kernsum == 0 ? 1 : kernsum;

    const struct img_size len = in->len;
    const uint8_t (*ind)[len.cols] = (uint8_t (*)[len.cols])in->data;
    uint8_t (*outd)[len.cols] = (uint8_t (*)[len.cols])out->data;
    for (int y = 1; y < in->len.rows-1; y++) {
        for (int x = 1; x < in->len.cols-1; x++) {
            outd[y][x] = weight_entry(y, x, in, kern)/kernsum;
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
    if (argc != 3) {
        fprintf(stderr, "ERROR: Uso: ./%s [.pgm a difuminar] [.pgm donde escribir la salida]\n", argv[0]);
        return 1;
    }

    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    unsigned char** Original = 0;
    int LargoOrig = 0;
    int AltoOrig = 0;
    unsigned char** Salida   = 0;
    int** nucleo = 0;
    if (rank == RANK_MASTER) {
        Original = pgmread(argv[1], &LargoOrig, &AltoOrig);
        Salida   = (unsigned char**)GetMem2D(LargoOrig, AltoOrig, sizeof(unsigned char));
        nucleo = (int**)GetMem2D(3, 3, sizeof(int));
        memcpy(nucleo[0], ridge_kernel_3, sizeof(ridge_kernel_3));
    }

    convolucion_paralelizada(Original, nucleo, Salida, LargoOrig, AltoOrig);

    if (rank == RANK_MASTER) {
        pgmwrite(Salida, argv[2], LargoOrig, AltoOrig);

        Free2D((void**) nucleo, 3);

        Free2D((void**) Original, LargoOrig);
        Free2D((void**) Salida, LargoOrig);
    }

    MPI_Finalize();
    return 0;
}
