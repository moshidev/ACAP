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
    size_t thread_count;
    const double* vector;
    size_t vector_len;
    size_t* solution_offset;
    pthread_mutex_t* solution_mutex;
};

struct pthread_calc_max {
    const double* vector;
    const size_t vector_len;
    pthread_t* thread;
    struct pthread_calc_max_context* context;
    size_t thread_count;
    size_t* solution_offset;
    pthread_mutex_t* solution_mutex;
};

static struct pthread_calc_max make_pthread_calc_max(size_t thread_count, const double* vector, size_t vector_len) {
    struct pthread_calc_max ret = {
        .vector = vector,
        .vector_len = vector_len,
        .thread = calloc(thread_count, sizeof(pthread_t)),
        .context = calloc(thread_count, sizeof(struct pthread_calc_max_context)),
        .solution_offset = calloc(1, sizeof(double)),
        .solution_mutex = calloc(1, sizeof(pthread_mutex_t)),
        .thread_count = thread_count
    };
    pthread_mutex_init(ret.solution_mutex, NULL_PTR);
    for (size_t i = 0; i < thread_count; i++) {
        ret.context[i].thread_id = i;
        ret.context[i].thread_count = thread_count;
        ret.context[i].vector = vector;
        ret.context[i].vector_len = vector_len;
        ret.context[i].solution_offset = ret.solution_offset;
        ret.context[i].solution_mutex = ret.solution_mutex;
    }
    return ret;
}

static void destroy_pthread_calc_max(struct pthread_calc_max* p) {
    pthread_mutex_destroy(p->solution_mutex);
    free(p->thread);
    free(p->context);
    free(p->solution_offset);
    free(p->solution_mutex);
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

    size_t len = c->vector_len / c->thread_count;
    if (c->thread_id == c->thread_count-1) {
        len += c->vector_len % c->thread_count;
    }

    size_t offset = len * c->thread_id;
    const double* v = c->vector + offset;
    size_t lsolution_offset = calc_max_seq(v, len) + offset;
    pthread_mutex_lock(c->solution_mutex);
    if (c->vector[lsolution_offset] > c->vector[*c->solution_offset]) {
        *c->solution_offset = lsolution_offset;
    }
    pthread_mutex_unlock(c->solution_mutex);

    pthread_exit(NULL_PTR);
}

size_t calc_max_parallel(size_t thread_count, const double* v, size_t vlen) {
    struct pthread_calc_max pexec = make_pthread_calc_max(thread_count, v, vlen);
    
    for (size_t i = 0; i < thread_count; i++) {
        pthread_create(&pexec.thread[i], NULL, pthread_calc_max_worker, &pexec.context[i]);
    }

    size_t max_i = 0;
    for (size_t i = 0; i < thread_count; i++) {
        pthread_join(pexec.thread[i], NULL_PTR);

        if (v[*pexec.solution_offset] > v[max_i]) {
            max_i = *pexec.solution_offset;
        }
    }

    destroy_pthread_calc_max(&pexec);
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