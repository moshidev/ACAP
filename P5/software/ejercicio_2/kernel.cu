
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

static bool is_power_of_two(int n) {
    if (n <= 0) {
        return false;
    }

    return ceil(log2(n)) == floor(log2(n));
}

static void assert_good_args(int argc, char** argv) {
    const char* bad_input_help_text = "No puedo ejecutar el programa si no me indicas ni el número de hebras ni de qué tamaño quieres el array de números aleatorios. "
        "El tamaño del vector debe ser múltiplo del tamaño de bloque. "
        "El número de hebras debe ser múltiplo de dos.\n";
    const char* usage_help_text = "Uso: %s [número de hebras] [longitud]\n";
    if (argc != 3) {
        fprintf(stderr, "%s", bad_input_help_text);
        fprintf(stderr, usage_help_text, argv[0]);
        exit(1);
    }

    size_t nthreads = strtol(argv[1], NULL_PTR, STRTOL_INFER_BASE);
    size_t vector_len = strtol(argv[2], NULL_PTR, STRTOL_INFER_BASE);
    if (nthreads == 0 || vector_len == 0 || vector_len % nthreads != 0 || nthreads > 512 || !is_power_of_two(nthreads)) {
        fprintf(stderr, "%s", bad_input_help_text);
        fprintf(stderr, usage_help_text, argv[0]);
        fprintf(stderr, "Leído: %s %zu %zu", argv[0], nthreads, vector_len);
        exit(2);
    }
}

__global__ void maxv(double* v, size_t vlen, double* block_max) {
    extern __shared__ double sdata[];
    const unsigned tid = threadIdx.x;
    const unsigned gridSize = blockDim.x * gridDim.x;
    unsigned i = blockIdx.x * blockDim.x + threadIdx.x;

    sdata[tid] = -INFINITY;
    if (i < vlen) {
        sdata[tid] = v[i];
        i += gridSize;
    }
    while (i < vlen) {
        sdata[tid] = v[i] > sdata[tid] ? v[i] : sdata[tid];
        i += gridSize;
    }
    __syncthreads();

    for (unsigned s = blockDim.x / 2; s > 0; s >>= 1) {
        if (tid < s) {
            sdata[tid] = sdata[tid + s] > sdata[tid] ? sdata[tid + s] : sdata[tid];
        }
        __syncthreads();
    }

    if (tid == 0) {
        block_max[blockIdx.x] = sdata[0];
    }
}

size_t calc_max_idx_seq(const double* v, size_t vlen) {
    double max = -INFINITY;
    size_t max_i = (size_t) - 1;
    for (size_t i = 0; i < vlen; i++) {
        if (v[i] > max) {
            max = v[i];
            max_i = i;
        }
    }
    return max_i;
}

int main(int argc, char** argv) {
    setlocale(LC_ALL, "");

    /* Comprueba los parámetros introducidos, abortando si no son correctos */
    assert_good_args(argc, argv);

    /* Leemos los argumentos que se nos proporcionan */
    int nthreads = strtol(argv[1], NULL_PTR, STRTOL_INFER_BASE);
    size_t vector_len = strtol(argv[2], NULL_PTR, STRTOL_INFER_BASE);
    int nblocks = 3072;

    /* Inicializamos un vector de double valores aleatorios */
    int seed = time(NULL_PTR);
    srand(seed);
    printf("Semilla utilizada: %d\n", seed);
    double* vector = (double*)calloc(vector_len, sizeof(double));
    if (!vector) {
        fprintf(stderr, "ª >w< son demasiados datos! no he podido reservar un trozo de memoria contigua tan grande!\n");
        fprintf(stderr, "Error fatal\n");
        exit(39);
    }
    size_t p = 0;
    for (size_t i = 0; i < vector_len; i++) {
        if (p >= 100000000) {
            p = 0;
            printf("Inicializados %zu elementos (%f%%).\r", i, 100.0*((double)i)/vector_len);
        }
        ++p;
        vector[i] = rand()*rand();
    }
    printf("Inicializados %zu elementos (%f%%).\n", vector_len, 100.0);

    /* Monitoriza */
    cudaEvent_t global_start, global_end;
    cudaEventCreate(&global_start);
    cudaEventCreate(&global_end);
    cudaEventRecord(global_start);

    /* Lanza kernel */
    double* dev_vector;
    double* dev_block_max;
    cudaMalloc(&dev_vector, vector_len*sizeof(double));
    cudaMalloc(&dev_block_max, nblocks*sizeof(double));
    cudaMemcpy(dev_vector, vector, vector_len * sizeof(double), cudaMemcpyHostToDevice);

    maxv << <nblocks, nthreads, nthreads * sizeof(double) >> > (dev_vector, vector_len, dev_block_max);
    maxv << <1, nthreads, nthreads * sizeof(double) >> > (dev_block_max, nblocks, dev_block_max);

    double cuda_max;
    cudaMemcpy(&cuda_max, dev_block_max, 1 * sizeof(double), cudaMemcpyDeviceToHost);

    /* Monitoriza */
    cudaEventRecord(global_end);
    cudaEventSynchronize(global_end);

    cudaDeviceSynchronize();
    double cpu_s = get_wall_time();
    double mmm = vector[calc_max_idx_seq(vector, vector_len)];
    cpu_s = get_wall_time() - cpu_s;
    printf("Esperado %f obtenido %f\n", mmm, cuda_max);

    /* Imprime los resultados de la ejecución */
    float elapsed_ms;
    cudaEventElapsedTime(&elapsed_ms, global_start, global_end);
    printf("Tiempo total de ejecución del kernel<<<%d, %d>>> sobre %zu números en coma flotante de precisión doble [ms]: %.8f\n", nblocks, nthreads, vector_len, elapsed_ms);
    printf("Tiempo de CPU [ms] %f, Speedup %f\n", cpu_s * 1000.0, cpu_s * 1000.0 / elapsed_ms);
    assert(mmm == cuda_max);

    /* Destruye los recursos utilizados */
    cudaEventDestroy(global_start);
    cudaEventDestroy(global_end);
    cudaFree(dev_block_max);
    cudaFree(dev_vector);
    free(vector);

    return 0;
}