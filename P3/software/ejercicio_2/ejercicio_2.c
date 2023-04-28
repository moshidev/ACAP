#include <math.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#define NULL_PTR    0
#define STRTOL_INFER_BASE   0

static double get_wall_time() {
    struct timeval time;
    gettimeofday(&time,NULL);
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

struct pthread_calc_max;

struct pthread_calc_max_context {
    size_t thread_id;
    struct pthread_calc_max* parent;
};

struct pthread_calc_max {
    const double* vector;
    size_t vector_len;
    pthread_t* thread;
    struct pthread_calc_max_context* context;
    size_t thread_count;
    size_t solution_offset;
    pthread_mutex_t solution_mutex;
};

static struct pthread_calc_max* make_pthread_calc_max(size_t thread_count, const double* vector, size_t vector_len) {
    struct pthread_calc_max* ret = malloc(sizeof(struct pthread_calc_max) + thread_count*sizeof(pthread_t) + thread_count*sizeof(struct pthread_calc_max_context));
    if (!ret) {
        return 0;
    }
    ret->vector = vector;
    ret->vector_len = vector_len;
    ret->thread = (pthread_t*) (((uint8_t*)ret) + sizeof(struct pthread_calc_max));
    ret->context = (struct pthread_calc_max_context*) (((uint8_t*)ret) + sizeof(struct pthread_calc_max) + thread_count*sizeof(pthread_t));
    ret->solution_offset = 0;
    pthread_mutex_init(&ret->solution_mutex, NULL_PTR);
    ret->thread_count = thread_count;
    for (size_t i = 0; i < thread_count; i++) {
        ret->context[i].thread_id = i;
        ret->context[i].parent = ret;
    }
    return ret;
}

static void destroy_pthread_calc_max(struct pthread_calc_max* p) {
    pthread_mutex_destroy(&p->solution_mutex);
    free(p);
}

size_t calc_max_seq(const double* v, size_t vlen) {
    double max = -INFINITY;
    size_t max_i = -1;
    for (size_t i = 0; i < vlen; i++) {
        if (v[i] > max) {
            max = v[i];
            max_i = i;
        }
    }
    return max_i;
}

void* pthread_calc_max_worker(void* pthread_user_data) {
    struct pthread_calc_max_context* c = (struct pthread_calc_max_context*)pthread_user_data;
    struct pthread_calc_max* p = c->parent;

    size_t len = p->vector_len / p->thread_count;
    if (c->thread_id == p->thread_count-1) {
        len += p->vector_len % p->thread_count;
    }

    size_t offset = len * c->thread_id;
    const double* v = p->vector + offset;
    size_t lsolution_offset = calc_max_seq(v, len) + offset;
    pthread_mutex_lock(&p->solution_mutex);
    if (p->vector[lsolution_offset] > p->vector[p->solution_offset]) {
        p->solution_offset = lsolution_offset;
    }
    pthread_mutex_unlock(&p->solution_mutex);

    pthread_exit(c);
}

size_t calc_max_parallel(size_t thread_count, const double* v, size_t vlen) {
    struct pthread_calc_max* pexec = make_pthread_calc_max(thread_count, v, vlen);
    if (!pexec) {
        fprintf(stderr, "ª >w< no he podido reservar memoria para los trabajadores!\n");
        fprintf(stderr, "Error fatal.\n");
        exit(38);
    }

    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(&pexec->thread[i], NULL, pthread_calc_max_worker, &pexec->context[i]);
    }

    size_t max_i = 0;
    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(pexec->thread[i], NULL_PTR);

        if (v[pexec->solution_offset] > v[max_i]) {
            max_i = pexec->solution_offset;
        }
    }

    destroy_pthread_calc_max(pexec);
    return max_i;
}

void assert_good_args(int argc, char** argv) {
    const char* bad_input_help_text = "No puedo ejecutar el programa si no me indicas ni el número de hebras ni de qué tamaño quieres el array de números aleatorios.\n";
    const char* usage_help_text = "Uso: %s [número de hebras] [longitud]\n";
    if (argc != 3) {
        fprintf(stderr, "%s", bad_input_help_text);
        fprintf(stderr, usage_help_text, argv[0]);
        exit(1);
    }

    size_t nthreads = strtol(argv[1], NULL_PTR, STRTOL_INFER_BASE);
    size_t vector_len = strtol(argv[2], NULL_PTR, STRTOL_INFER_BASE);
    if (nthreads == 0 || vector_len == 0) {
        fprintf(stderr, "%s", bad_input_help_text);
        fprintf(stderr, usage_help_text, argv[0]);
        fprintf(stderr, "Leído: %s %zu %zu", argv[0], nthreads, vector_len);
        exit(2);
    }
}

int main(int argc, char** argv) {
    assert_good_args(argc, argv);

    /* Leemos los argumentos que se nos proporcionan */
    size_t thread_count = strtol(argv[1], NULL_PTR, STRTOL_INFER_BASE);
    size_t vector_len = strtol(argv[2], NULL_PTR, STRTOL_INFER_BASE);

    /* Inicializamos un vector de double valores aleatorios */
    double* vector = calloc(vector_len, sizeof(double));
    if (!vector) {
        fprintf(stderr, "ª >w< son demasiados datos! no he podido reservar un trozo de memoria contigua tan grande!\n");
        fprintf(stderr, "Error fatal\n");
        exit(39);
    }
    for (size_t i = 0; i < vector_len; i++) {
        *(((int*)vector)+i) = rand();
        *(((int*)vector)+i+1) = rand();
    }

    /* Encontramos el máximo del vector utilizando pthreads */
    double wallt_parallel = get_wall_time();
    size_t parallel_index = calc_max_parallel(thread_count, vector, vector_len);
    wallt_parallel = get_wall_time()-wallt_parallel;
    printf("Máximo encontrado de forma paralela: %f en %f segundos.\n", vector[parallel_index], wallt_parallel);

    /* Encontramos el máximo del vector de forma secuencial */
    double wallt_seq = get_wall_time();
    size_t seq_index = calc_max_seq(vector, vector_len);
    wallt_seq = get_wall_time()-wallt_seq;
    printf("Máximo encontrado de forma secuencial: %f en %f segundos.\n", vector[seq_index], wallt_seq);

    /* Liberamos los recursos reservados */
    free(vector);

    return 0;
}