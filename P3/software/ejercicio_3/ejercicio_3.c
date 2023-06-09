/**
 * Daniel Pedrosa © 2023
 * Jaccard Index Parallel Algorithm playground
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "khash.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

const char txt_err_dist[] = "No puedo ejecutar el ejercicio si no me indicas la longitud de los conjuntos sobre los que calcular la distancia de Jaccard >:c\n";

static double get_wall_time() {
	struct timeval time;
	gettimeofday(&time,NULL);
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static double get_cpu_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

static void assert_argc(int argc, char** argv) {
	if (argc != 5) {
		fprintf(stderr, txt_err_dist);
		fprintf(stderr, "Uso: %s [num. hilos] [tam. intersección máx.] [# conjunto A] [# conjunto B]\n", argv[0]);
		exit(1);
	}
}

static void assert_v_len(size_t va_len, size_t vb_len, char* command, char* va_arg, char* vb_arg) {
	if (va_len == 0 || va_len == 0 || va_arg[0] == '-' || vb_arg[0] == '-') {
		fprintf(stderr, "O bien no has introducido un número o bien es un número menor o igual a cero.\n");
		fprintf(stderr, txt_err_dist);
		fprintf(stderr, "Uso: %s [# conjunto A] [# conjunto B]\n", command);
		exit(1);
	}
}

KHASH_SET_INIT_INT(i32)

static khash_t(i32)* mk_kh_i32_set_from_vector(int32_t* v, size_t v_len) {
	khash_t(i32)* kh_set = kh_init(i32);
	if (!kh_set) {
		return 0;
	}

	for (size_t i = 0; i < v_len; i++) {
		int retc;
		kh_put(i32, kh_set, v[i], &retc);

		if (retc < 0) {
			fprintf(stderr, "o.o no puedo insertar la llave %d en el hashset. ¿Por qué?\n", v[i]);
			kh_destroy(i32, kh_set);
			return 0;
		}
	}

	return kh_set;
}

typedef struct pair_kh_i32 {
	khash_t(i32)* a;
	khash_t(i32)* b;
} pair_kh_i32_t;

typedef struct vector_i32 {
	int32_t* v;
	size_t v_len;
} vector_i32_t;

static void* pthread_mk_kh_i32_set_from_vector(void* arg) {
	vector_i32_t* vector = (vector_i32_t*)arg;
	pthread_exit(mk_kh_i32_set_from_vector(vector->v, vector->v_len));
}

static pair_kh_i32_t mk_pair_kh_i32_t(size_t nthreads, int32_t* va, size_t va_len, int32_t* vb, size_t vb_len) {
	pair_kh_i32_t khset;

	if (nthreads == 1) {
		khset.a = mk_kh_i32_set_from_vector(va, va_len);
		khset.b = mk_kh_i32_set_from_vector(vb, vb_len);
	}
	else {
		pthread_t pthread;
		vector_i32_t vector_b = {.v = vb, .v_len = vb_len};
		pthread_create(&pthread, NULL, pthread_mk_kh_i32_set_from_vector, &vector_b);
		khset.a = mk_kh_i32_set_from_vector(va, va_len);
		pthread_join(pthread, (void**)&khset.b);
	}

	if (!khset.a) {
		fprintf(stderr, "Error fatal inicializando conjunto A.\n");
	}
	if (!khset.b) {
		fprintf(stderr, "Error fatal inicializando conjunto B.\n");
	}
	if (!khset.a || !khset.b) {
		exit(38);
	}

	return khset;
}

static void destroy_pair_kh_i32_t(pair_kh_i32_t p) {
	kh_destroy(i32, p.a);
	kh_destroy(i32, p.b);
}

struct partial_intersection_size {
	khint_t itb_ini;
	khint_t itb_fin;
	pair_kh_i32_t pset;
	size_t intersection_size;
};

void* pthread_calc_intersection_size(void* arg) {
	struct partial_intersection_size* p = (struct partial_intersection_size*)arg;

	p->intersection_size = 0;
	for (khint_t it_b = p->itb_ini; it_b != p->itb_fin; ++it_b) {
		if (kh_exist(p->pset.b, it_b)) {
			khint_t it_a = kh_get(i32, p->pset.a, kh_key(p->pset.b, it_b));
			bool is_missing = (it_a == kh_end(p->pset.a));
			p->intersection_size += is_missing ? 0 : 1;
		}
	}

	pthread_exit(0);
}

static size_t calc_intersection_size(size_t nthreads, pair_kh_i32_t pset) {
	pthread_t pthread[nthreads];
	struct partial_intersection_size partial[nthreads];
	for (int i = 0; i < nthreads; i++) {
		partial[i].pset = pset;
		partial[i].itb_ini = i * (kh_end(pset.b) / nthreads);
		partial[i].itb_fin = i == nthreads-1 ? kh_end(pset.b) : (i+1) * (kh_end(pset.b) / nthreads);
		pthread_create(&pthread[i], NULL, pthread_calc_intersection_size, &partial[i]);
	}

	size_t intersection_size = 0;
	for (int i = 0; i < nthreads; i++) {
		size_t partial_intersection_size;
		pthread_join(pthread[i], NULL);
		intersection_size += partial[i].intersection_size;
	}

	return intersection_size;
}

int main(int argc, char** argv) {
	assert_argc(argc, argv);

	size_t nthreads = strtoul(argv[1], 0, 0);
	size_t max_intersection_size = strtoul(argv[2], 0, 0);
	size_t va_len = strtoul(argv[3], 0, 0);
	size_t vb_len = strtoul(argv[4], 0, 0);
	size_t smallest_v_len = va_len < vb_len ? va_len : vb_len;
	size_t biggest_v_len = va_len >= vb_len ? va_len : vb_len;

	if (nthreads <= 0) {
		nthreads = 1;
	}
	assert_v_len(va_len, vb_len, argv[0], argv[3], argv[4]);

	printf("Reserva memoria...\n");
	int32_t* va = calloc(va_len, sizeof(int32_t));
	int32_t* vb = calloc(vb_len, sizeof(int32_t));
    if (!va || !vb) {
        fprintf(stderr, "ª >w< son demasiados datos! no he podido reservar un trozo de memoria contigua tan grande!\n");
        fprintf(stderr, "Error fatal\n");
        exit(39);
    }

	printf("Inicializa vectores...\n");
	for (size_t i = 0; i < biggest_v_len; i++) {
		if (i < va_len) {
			va[i] = i;
		}
		if (i < vb_len) {
			vb[i] = i%max_intersection_size;
		}
	}

	double wallt = get_wall_time();
	double cput = get_cpu_time();

	printf("Inicializa hashsets...\n");
	pair_kh_i32_t pset = mk_pair_kh_i32_t(nthreads, va, va_len, vb, vb_len);

	printf("Calcula el tamaño de la intersección de los dos conjuntos...\n");
	size_t intersection_size = calc_intersection_size(nthreads, pset);

	cput = get_cpu_time() - cput;
	wallt = get_wall_time() - wallt;
	printf("Cardinalidad de la intersección: %zu\n", intersection_size);
	size_t union_size = (kh_size(pset.a) + kh_size(pset.b) - intersection_size);
	double Jaccard_index = (double)intersection_size / (double)union_size;
	printf("Índice de Jaccard: %f\n", Jaccard_index);
	printf("wallt:%f,cput:%f\n", wallt, cput);

	destroy_pair_kh_i32_t(pset);
	free(va);
	free(vb);

	return 0;
}
