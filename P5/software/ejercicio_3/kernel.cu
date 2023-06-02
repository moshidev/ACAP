#include "cuda.h"
#include "cuda_runtime.h"
#include "device_launch_parameters.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <locale.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>
#ifdef WIN32
#include <Windows.h>
int gettimeofday(struct timeval* tp, struct timezone* tzp)
{
    // Note: some broken versions only have 8 trailing zero's, the correct epoch has 9 trailing zero's
    // This magic number is the number of 100 nanosecond intervals since January 1, 1601 (UTC)
    // until 00:00:00 January 1, 1970 
    static const uint64_t EPOCH = ((uint64_t)116444736000000000ULL);

    SYSTEMTIME  system_time;
    FILETIME    file_time;
    uint64_t    time;

    GetSystemTime(&system_time);
    SystemTimeToFileTime(&system_time, &file_time);
    time = ((uint64_t)file_time.dwLowDateTime);
    time += ((uint64_t)file_time.dwHighDateTime) << 32;

    tp->tv_sec = (long)((time - EPOCH) / 10000000L);
    tp->tv_usec = (long)(system_time.wMilliseconds * 1000);
    return 0;
}
#else
#include <sys/time.h>
#endif

static double get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        printf("Error en la medicion de tiempo CPU!!\n");
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

#define NULL_PTR    0
#define STRTOL_INFER_BASE   0

typedef struct matrix {
    size_t n;
    size_t m;
    double* v;
} matrix_t;

#define acc(matrix, i, j) \
    ((matrix).v[i*(matrix).m+j])

int matrix_alloc(size_t n, size_t m, matrix_t* matrix) {
    double* vector = (double*)calloc(n*m, sizeof(double));
    if (vector) {
        matrix->n = n;
        matrix->m = m;
        matrix->v = vector;
        return 0;
    }
    return -1;
}

void matrix_destroy(matrix_t* matrix) {
    matrix->n = 0;
    matrix->m = 0;
    free(matrix->v);
    matrix->v = 0;
}

void matrix_print(matrix_t m) {
    for (size_t i = 0; i < m.n; i++) {
        for (size_t j = 0; j < m.m; j++) {
            printf("%.2f", acc(m, i, j));
            putchar(j != m.m-1 ? '\t' : '\n');
        }
    }
}

int matrix_mul(matrix_t A, matrix_t B, matrix_t* dst) {
    if (A.m != B.n) {
        return -1;
    }
    int errDst = matrix_alloc(A.n, B.m, dst);
    if (errDst != 0) {
        return -2;
    }

    const size_t suml = A.m;
    for (size_t i = 0; i < dst->n; i++) {
        for (size_t j = 0; j < dst->m; j++) {
            double sum = 0.0;
            for (size_t w = 0; w < suml; w++) {
                sum += acc(A, i, w) * acc(B, w, j);
            }
            acc(*dst, i, j) = sum;
        }
        /*if (i % (dst->n/100) == 0) {
            printf("%.2f%%\r", 100.0*i/dst->n);
            fflush(stdout);
        }*/
    }

    return 0;
}

void randomize(double* v, size_t v_len) {
    int seed = time(NULL_PTR);
    srand(seed);
    printf("Semilla utilizada: %d\n", seed);
    size_t p = 0;
    for (size_t i = 0; i < v_len; i++) {
        if (p >= 100000000) {
            p = 0;
            printf("Inicializados %zu elementos aleatorios (%f%%).\r", i, 100.0*((double)i)/v_len);
        }
        ++p;
        v[i] = rand()*rand();
    }
    printf("Inicializados %zu elementos aleatorios (%f%%).\n", v_len, 100.0);
}

void memExit39(void) {
    fprintf(stderr, "ª >w< son demasiados datos! no he podido reservar un trozo de memoria contigua tan grande!\n");
    fprintf(stderr, "Error fatal\n");
    exit(39);
}

__global__ void kernelMatrixMul(double* va, double* vb, size_t n, size_t m, size_t l, double* dst) {
    size_t i = threadIdx.x + blockIdx.x * blockDim.x;

    while (i < n) {
        for (size_t j = 0; j < l; j++) {
            double sum = 0.0;
            for (size_t w = 0; w < m; w++) {
                sum += va[i * m + w] * vb[w * l + j];
            }
            dst[i * l + j] = sum;
        }
        i += blockDim.x * gridDim.x;
    }
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");

    /* Inicializamos dos matrices */
    matrix_t A, B;
    int errA = matrix_alloc(1000, 500, &A);
    int errB = matrix_alloc(500, 700, &B);
    if (errA != 0 || errB != 0) {
        memExit39();
    }
    //randomize(A.v, A.n*A.m);
    //randomize(B.v, B.n*B.m);
    for (size_t i = 0; i < A.n*A.m; i++) {
        A.v[i] = i%5+1;
    }
    for (size_t i = 0; i < B.n * B.m; i++) {
        B.v[i] = i%5+1;
    }

    /* Multiplicamos ambas matrices en la CPU */
    double cpu_s = get_wall_time();
    matrix_t productAB;
    int errMul = matrix_mul(A, B, &productAB);
    if (errMul != 0) {
        memExit39();
    }
    if (productAB.n*productAB.m < 9*9) {
        printf("Matriz A:\n");
        matrix_print(A);
        printf("Matriz B:\n");
        matrix_print(B);
        printf("Producto de ambas matrices (CPU):\n");
        matrix_print(productAB);
    }
    cpu_s = get_wall_time() - cpu_s;
    printf("t_cpu_s:%.8f\n", cpu_s);

    /* Monitoriza */
    cudaEvent_t global_start, global_end;
    cudaEventCreate(&global_start);
    cudaEventCreate(&global_end);
    cudaEventRecord(global_start);

    /* Multiplicamos ambas matrices en la GPU */
    double *dev_va, *dev_vb, *dev_vdst;
    int nthreads = 512, nblocks = 3070;
    cudaMalloc(&dev_va, A.n * A.m * sizeof(double));
    cudaMalloc(&dev_vb, B.n * B.m * sizeof(double));
    cudaMalloc(&dev_vdst, A.n * B.m * sizeof(double));

    cudaMemcpy(dev_va, A.v, A.n * A.m * sizeof(double), cudaMemcpyHostToDevice);
    cudaMemcpy(dev_vb, B.v, B.n * B.m * sizeof(double), cudaMemcpyHostToDevice);
    kernelMatrixMul << <nblocks, nthreads >> > (dev_va, dev_vb, A.n, A.m, B.m, dev_vdst);
    matrix_t productAB_GPU;
    matrix_alloc(A.n, B.m, &productAB_GPU);
    cudaMemcpy(productAB_GPU.v, dev_vdst, A.n * B.m * sizeof(double), cudaMemcpyDeviceToHost);

    /* Monitoriza */
    cudaEventRecord(global_end);
    cudaEventSynchronize(global_end);
    float gpu_ms;
    cudaEventElapsedTime(&gpu_ms, global_start, global_end);
    printf("t_gpu_s:%.8f\n", gpu_ms/1000.0f);

    if (productAB_GPU.n * productAB_GPU.m < 9 * 9) {
        printf("Producto de ambas matrices (GPU):\n");
        matrix_print(productAB_GPU);
    }
    assert(memcmp(productAB.v, productAB_GPU.v, productAB.n * productAB.m * sizeof(double)) == 0);


    /* Destruye los recursos utilizados */
    matrix_destroy(&A);
    matrix_destroy(&B);
    matrix_destroy(&productAB);
    matrix_destroy(&productAB_GPU);
    cudaFree(dev_va);
    cudaFree(dev_vb);
    cudaFree(dev_vdst);

    return 0;
}