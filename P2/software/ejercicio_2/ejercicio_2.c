#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
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

static double get_wall_time() {
    struct timeval time;
    gettimeofday(&time,NULL);
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static double get_cpu_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

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
            unsigned srci = (y-1)+i;
            unsigned srcj = (x-1)+j;
            suma = suma + srcd[srci][srcj] * kernd[i][j];
        }
    }
    return suma;
}

static void convolucion_selectiva(const struct img_channel* in, const struct kernel* kern, struct img_channel* out, int y1, int y2, int x1, int x2) {
    const int (*kernd)[kern->side_len] = (int (*)[kern->side_len])kern->data;
    int kernsum = kernel_sum(kern);
    kernsum = kernsum == 0 ? 1 : kernsum;

    const struct img_size len = in->len;
    const uint8_t (*ind)[len.cols] = (uint8_t (*)[len.cols])in->data;
    uint8_t (*outd)[len.cols] = (uint8_t (*)[len.cols])out->data;
    for (int y = y1; y < y2; y++) {
        for (int x = x1; x < x2; x++) {
            outd[y][x] = weight_entry(y, x, in, kern)/kernsum;
        }
    }
}

static void convolucion(const struct img_channel* in, const struct kernel* kern, struct img_channel* out) {
    convolucion_selectiva(in, kern, out, 1, in->len.rows-1, 1, in->len.cols-1);    
}

static struct img_channel MPI_Alloc_img_channel(uint64_t rows, uint64_t cols) {
    uint8_t* allocated_data;
    MPI_Alloc_mem(rows*cols*sizeof(uint8_t), MPI_INFO_NULL, &allocated_data);
    struct img_channel imgc = {
        .data = allocated_data,
        .len = { .rows = rows, .cols = cols }
    };
    return imgc;
}

static void MPI_Free_img_channel(struct img_channel* img_channel) {
    MPI_Free_mem(img_channel->data);
    memset(img_channel, 0, sizeof(struct img_channel));
}

static struct kernel MPI_Alloc_kernel(int side_len) {
    int* allocated_data;
    MPI_Alloc_mem(side_len*side_len*sizeof(int), MPI_INFO_NULL, &allocated_data);
    struct kernel kernel = {
        .data = allocated_data,
        .side_len = side_len
    };
    return kernel;
}

static void MPI_Free_kernel(struct kernel* kernel) {
    MPI_Free_mem(kernel->data);
    memset(kernel, 0, sizeof(struct kernel));
}

static void convolucion_paralelizada(unsigned char** orig, int** kern, unsigned char** dest, uint64_t rows, uint64_t cols) {
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    /* Comunica a todos los procesos tanto el kernel como el tamaño de la imagen  */
    struct img_size img_size;
    const int kern_side_len = 3;
    struct kernel conv_matrix = MPI_Alloc_kernel(kern_side_len);
    if (rank == RANK_MASTER) {
        img_size.rows = rows;
        img_size.cols = cols;
        memcpy(conv_matrix.data, kern[0], kern_side_len*kern_side_len*sizeof(int));
    }
    MPI_Bcast(&img_size, 2, MPI_UINT64_T, RANK_MASTER, MPI_COMM_WORLD);
    MPI_Bcast(conv_matrix.data, 3*3, MPI_INT, RANK_MASTER, MPI_COMM_WORLD);
    MPI_Barrier(MPI_COMM_WORLD);

    /* Establece en cada proceso el tamaño de la imagen original y el número de lineas que le corresponde procesar */
    rows = img_size.rows;
    cols = img_size.cols;
    uint64_t rows_per_proc = rows / nprocs;

    /* Reserva en cada proceso la memoria justa para procesar su trozo de imagen correspondiente */
    struct img_channel img_origl = MPI_Alloc_img_channel(rows_per_proc, cols);
    struct img_channel img_destl = MPI_Alloc_img_channel(rows_per_proc, cols);

    /* Distribuye, procesa y reune los trozos de la imagen */
    MPI_Scatter(rank == RANK_MASTER ? orig[0] : 0, rows_per_proc*cols, MPI_UNSIGNED_CHAR, img_origl.data, rows_per_proc*cols, MPI_UNSIGNED_CHAR, RANK_MASTER, MPI_COMM_WORLD);
    convolucion(&img_origl, &conv_matrix, &img_destl);
    MPI_Gather(img_destl.data, rows_per_proc*cols, MPI_UNSIGNED_CHAR, rank == RANK_MASTER ? dest[0] : 0, rows_per_proc*cols, MPI_UNSIGNED_CHAR, RANK_MASTER, MPI_COMM_WORLD);

    /* Libera en cada proceso memoria utilizada en la función */
    MPI_Free_img_channel(&img_origl);
    MPI_Free_img_channel(&img_destl);

    /* Procesa los trozos no facilmente paralelizables */
    if (rank == RANK_MASTER) {
        const struct img_channel img_orig = { .data = orig[0], .len = { .rows = rows, .cols = cols } };
        struct img_channel img_dest = { .data = dest[0], .len = { .rows = rows, .cols = cols } };
        for (int i = 1; i < nprocs; i++) {
            convolucion_selectiva(&img_orig, &conv_matrix, &img_dest, rows_per_proc*i-1, rows_per_proc*i+1, 1, cols-1);
        }
        if (rows % nprocs) {
            convolucion_selectiva(&img_orig, &conv_matrix, &img_dest, rows_per_proc*nprocs-1, rows-1, 1, cols-1);
        }
    }
    MPI_Free_kernel(&conv_matrix);
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

    double wall_time = get_wall_time();
    double lcpu_time = get_cpu_time();
    convolucion_paralelizada(Original, nucleo, Salida, LargoOrig, AltoOrig); // pgmread devuelve un Array2D con las columnas intercambiadas con las filas
    wall_time = get_wall_time() - wall_time;
    lcpu_time = get_cpu_time() - lcpu_time;
    double cpu_time;
    MPI_Reduce(&lcpu_time, &cpu_time, 1, MPI_DOUBLE, MPI_SUM, RANK_MASTER, MPI_COMM_WORLD);
    if (rank == RANK_MASTER) {
        printf("wall_time:%f,cpu_time:%f\n", wall_time, cpu_time);
    }

    if (rank == RANK_MASTER) {
        pgmwrite(Salida, argv[2], LargoOrig, AltoOrig);

        Free2D((void**) nucleo, 3);

        Free2D((void**) Original, LargoOrig);
        Free2D((void**) Salida, LargoOrig);
    }

    MPI_Finalize();
    return 0;
}
