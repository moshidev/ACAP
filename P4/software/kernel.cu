#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>     // srand, rand
#include <time.h>       // time
#include <locale.h>
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
#include <cuda.h>
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

#define IMG_DEPTH 256

struct parametros {
    size_t niter;
    size_t img_len;
    uint16_t nblocks;
    uint16_t nthreads_per_block;
};

typedef struct {
    uint8_t* v;
    size_t len_i;
    size_t len_j;
} img_t;

static double get_wall_time() {
    struct timeval time;
    if (gettimeofday(&time, NULL)) {
        printf("Error en la medicion de tiempo CPU!!\n");
        return 0;
    }
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static img_t rand_1D_img(size_t len_j) {
    srand(0);
    img_t img = {(uint8_t*)malloc(len_j), 1, len_j};
    for (unsigned long i = 0; i < len_j; i++) {
        img.v[i] = rand() % IMG_DEPTH;
    }
    return img;
}

static void histogramaCPU(unsigned char* img, size_t nBytes, unsigned int* histo) {
    double t1 = get_wall_time();
    for (int i = 0; i < IMG_DEPTH; i++)
        histo[i] = 0;//Inicializacion
    for (size_t i = 0; i < nBytes; i++) {
        histo[img[i]]++;
    }
    double t2 = get_wall_time();
    printf("Tiempo de CPU (s): %.4lf\n", t2-t1);
}

__global__ void kernelHistograma(unsigned char* imagen, size_t size, unsigned int* ghisto) {
    __shared__ unsigned int lhisto[IMG_DEPTH];
    for (unsigned t = threadIdx.x; t < IMG_DEPTH; t += blockDim.x) {
        lhisto[t] = 0;
        t += blockDim.x;
    }

    size_t i = threadIdx.x + blockIdx.x * blockDim.x;
    int offset = blockDim.x * gridDim.x;

    __syncthreads();
    while (i < size) {
        atomicAdd(&lhisto[imagen[i]], 1);
        i += offset;
    }

    __syncthreads();
    for (unsigned t = threadIdx.x; t < IMG_DEPTH; t += blockDim.x) {
        atomicAdd(&ghisto[t], lhisto[t]);
    }
}

static void assert_good_algorithm_histogram_cuda(struct parametros p) {
    unsigned char* imagen = (unsigned char*)rand_1D_img(p.img_len).v;

    /* Calcula el histograma en la CPU */
    unsigned int histoCPU[IMG_DEPTH];
    histogramaCPU(imagen, p.img_len, histoCPU);

    /* Calcula el histograma en la GPU */
    unsigned int histoGPU[IMG_DEPTH];
    unsigned char* dev_imagen = 0;
    unsigned int* dev_histo = 0;
    cudaMalloc((void**)&dev_imagen, p.img_len);
    cudaMemcpy(dev_imagen, imagen, p.img_len, cudaMemcpyHostToDevice);
    cudaMalloc((void**)&dev_histo, IMG_DEPTH * sizeof(unsigned int));
    cudaMemset(dev_histo, 0, IMG_DEPTH * sizeof(unsigned int));
    kernelHistograma << <p.nblocks, p.nthreads_per_block >> > (dev_imagen, p.img_len, dev_histo);
    cudaMemcpy(histoGPU, dev_histo, IMG_DEPTH * sizeof(unsigned int), cudaMemcpyDeviceToHost);

    /* Comprueba que son el mismo histograma */
    assert(memcmp(histoCPU, histoGPU, IMG_DEPTH) == 0);
    printf("Calculo correcto!!\n");
    cudaFree(dev_imagen);
    cudaFree(dev_histo);
    free(imagen);
}

struct parametros get_params(int argc, char** argv) {
    struct parametros p;
    const char txt_parametros[] = "<número bloques> <hebras por bloque> <número de iteraciones del benchmark> <tamaño de la imagen en bytes>";
    if (argc != 5) {
        fprintf(stderr, "No puedo ejecutar el programa si no me indicas qué tengo que hacer.\n");
        fprintf(stderr, "Uso: %s %s.\n", argv[0], txt_parametros);
        exit(1);
    }
    else {
        unsigned long nblocks = strtoul(argv[1], NULL, 0);
        unsigned long nthreads_per_block = strtoul(argv[2], NULL, 0);
        unsigned long niter = strtoul(argv[3], NULL, 0);
        unsigned long img_len = strtoul(argv[4], NULL, 0);
        cudaDeviceProp prop;
        cudaGetDeviceProperties(&prop, 0);
        if (nblocks > UINT16_MAX || nthreads_per_block > (unsigned)prop.maxThreadsPerBlock) {
            fprintf(stderr, "Parámetros de ejecución incompatibles con la tarjeta gráfica.\n");
            fprintf(stderr, "Número de bloques esperados: menor igual que %d ; obtenidos: %d\n", UINT16_MAX, nblocks);
            fprintf(stderr, "Número de hilos por bloque esperados: menor igual que %d ; obtenidos: %d\n", prop.maxThreadsPerBlock, nthreads_per_block);
            fprintf(stderr, "Uso: %s %s.\n", argv[0], txt_parametros);
            exit(1);
        }
        p.nblocks = (uint16_t)nblocks;
        p.nthreads_per_block = (uint16_t)nthreads_per_block;
        p.niter = (size_t)niter;
        p.img_len = (size_t)img_len;
    }
    return p;
}

int main(int argc, char** argv) {
    /* Comprueba los parámetros introducidos, abortando si no son correctos */
    setlocale(LC_ALL, "");
    struct parametros p = get_params(argc, argv);

    /* Comprueba que la implementación CUDA es válida */
    assert_good_algorithm_histogram_cuda(p);

    /* Inicializa la memoria del dispositivo */
    unsigned char* imagen = (unsigned char*)rand_1D_img(p.img_len).v;
    cudaEvent_t global_start, global_end;
    cudaEventCreate(&global_start);
    cudaEventCreate(&global_end);
    cudaDeviceSynchronize();
    cudaEventRecord(global_start);
    unsigned char* dev_imagen = 0;
    unsigned int* dev_histo = 0;
    cudaMalloc((void**)&dev_imagen, p.img_len);
    cudaMemcpy(dev_imagen, imagen, p.img_len, cudaMemcpyHostToDevice);
    cudaMalloc((void**)&dev_histo, IMG_DEPTH * sizeof(unsigned int));

    /* Inicializa los recursos para el benchmark */
    cudaEvent_t iter_start, iter_end;
    cudaEventCreate(&iter_start);
    cudaEventCreate(&iter_end);
    float gpu_compute_ms = 0.0f;

    /* Ejecuta el benchmark tantas veces como se indique */
    if (p.niter > 0) {
        cudaMemset(dev_histo, 0, IMG_DEPTH * sizeof(unsigned int));
        kernelHistograma << <p.nblocks, p.nthreads_per_block >> > (dev_imagen, p.img_len, dev_histo);  // la primera iteración suele tardar más que las posteriores
    }
    for (int iter = 0; iter < p.niter-1; iter++) {
        cudaMemset(dev_histo, 0, IMG_DEPTH * sizeof(unsigned int));
        cudaDeviceSynchronize();
        cudaEventRecord(iter_start);
        kernelHistograma << <p.nblocks, p.nthreads_per_block >> > (dev_imagen, p.img_len, dev_histo);
        cudaEventRecord(iter_end);
        cudaEventSynchronize(iter_end);

        float elapsed_ms;
        cudaEventElapsedTime(&elapsed_ms, iter_start, iter_end);
        gpu_compute_ms += elapsed_ms;
    }

    /* Trae a memoria local el último histograma de la última ejecución del benchmark */
    uint32_t histoGPU[IMG_DEPTH];
    cudaMemcpy(histoGPU, dev_histo, IMG_DEPTH * sizeof(unsigned int), cudaMemcpyDeviceToHost);
    cudaEventRecord(global_end);
    cudaEventSynchronize(global_end);

    /* Imprime por pantalla el tiempo medio de ejecución por benchmark */
    float gpu_avg_compute_seconds_per_iter = gpu_compute_ms / (p.niter * 1000.0f);
    printf("Tiempo medio de ejecucion del kernel<<<%d, %d>>> sobre %zu bytes [s]: %.4f\n", p.nblocks, p.nthreads_per_block, p.img_len, gpu_avg_compute_seconds_per_iter);
    float elapsed_ms;
    cudaEventElapsedTime(&elapsed_ms, global_start, global_end);
    float gpu_global_compute_seconds = elapsed_ms / 1000.0f;
    printf("Tiempo total de ejecución del benchmark del kernel<<<%d, %d>>> sobre %zu bytes [s]: %.4f\n", p.nblocks, p.nthreads_per_block, p.img_len, gpu_global_compute_seconds);

    /* Destruye los recursos utilizados */
    cudaEventDestroy(global_start);
    cudaEventDestroy(global_end);
    cudaEventDestroy(iter_start);
    cudaEventDestroy(iter_end);
    cudaFree(dev_imagen);
    cudaFree(dev_histo);
    free(imagen);

    return 0;
}