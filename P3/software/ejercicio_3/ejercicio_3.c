#include "khash.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

const char txt_err_dist[] = "No puedo ejecutar el ejercicio si no me indicas la longitud de los conjuntos sobre los que calcular la distancia de Jaccard >:c\n";

static double get_wall_time() {
	struct timeval time;
	gettimeofday(&time,NULL);
	return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static void assert_argc(int argc, char** argv) {
	if (argc != 3) {
		fprintf(stderr, txt_err_dist);
		fprintf(stderr, "Uso: %s [# conjunto A] [# conjunto B]\n", argv[0]);
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

static pair_kh_i32_t mk_pair_kh_i32_t(size_t nthreads, int32_t* va, size_t va_len, int32_t* vb, size_t vb_len) {
	pair_kh_i32_t khset;

	if (nthreads == 1) {
		khset.a = mk_kh_i32_set_from_vector(va, va_len);
		khset.b = mk_kh_i32_set_from_vector(vb, vb_len);
	}
	else {
		/* Aquí queremos paralelizar estas dos inicializaciones */
		khset.a = mk_kh_i32_set_from_vector(va, va_len);
		khset.b = mk_kh_i32_set_from_vector(vb, vb_len);
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

int main(int argc, char** argv) {
	assert_argc(argc, argv);

	size_t va_len = strtoul(argv[1], 0, 0);
	size_t vb_len = strtoul(argv[2], 0, 0);
	size_t smallest_v_len = va_len < vb_len ? va_len : vb_len;
	size_t biggest_v_len = va_len >= vb_len ? va_len : vb_len;

	assert_v_len(va_len, vb_len, argv[0], argv[1], argv[2]);

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
			va[i] = i%1000;
		}
		if (i < vb_len) {
			vb[i] = i%39;
		}
	}

	printf("Inicializa hashsets...\n");
	pair_kh_i32_t pset = mk_pair_kh_i32_t(1, va, va_len, vb, vb_len);

	printf("Calcula el # de la intersección de los dos conjuntos...\n");
	size_t intersect_count = 0;
	for (khint_t it_b = kh_begin(pset.b); it_b != kh_end(pset.b); ++it_b) {
		if (kh_exist(pset.b, it_b)) {
			khint_t it_a = kh_get(i32, pset.a, kh_key(pset.b, it_b));
			bool is_missing = (it_a == kh_end(pset.a));
			intersect_count += is_missing ? 0 : 1;
		}
	}

	printf("Cardinalidad encontrada: %zu\n", intersect_count);

	destroy_pair_kh_i32_t(pset);
	free(va);	//... espera, realmente hacen falta vectores para nuestro cometido? Además, esto es menos seguro que una arqueta sin tapa XD
	free(vb);

	return 0;
}
