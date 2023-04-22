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

static void assert_set_len(size_t va_len, size_t vb_len, char* command, char* va_arg, char* vb_arg) {
	if (va_len == 0 || va_len == 0 || va_arg[0] == '-' || vb_arg[0] == '-') {
		fprintf(stderr, "O bien no has introducido un número o bien es un número menor o igual a cero.\n");
		fprintf(stderr, txt_err_dist);
		fprintf(stderr, "Uso: %s [# conjunto A] [# conjunto B]\n", command);
		exit(1);
	}
}

KHASH_SET_INIT_INT(i32)

int main(int argc, char** argv) {
	assert_argc(argc, argv);

	size_t va_len = strtoul(argv[1], 0, 0);
	size_t vb_len = strtoul(argv[2], 0, 0);
	size_t smallest_v_len = va_len < vb_len ? va_len : vb_len;
	size_t biggest_v_len = va_len >= vb_len ? va_len : vb_len;

	assert_set_len(va_len, vb_len, argv[0], argv[1], argv[2]);

	printf("Reserva memoria...\n");
	int32_t* va = calloc(va_len, sizeof(int32_t));
	int32_t* vb = calloc(vb_len, sizeof(int32_t));

	printf("Inicializa vectores...\n");
	for (size_t i = 0; i < biggest_v_len; i++) {
		if (i < va_len) {
			va[i] = i%1000;
		}
		if (i < vb_len) {
			vb[i] = i%39;
		}
	}

	printf("Inicializa hashset...\n");
	khash_t(i32)* hset_a = kh_init(i32);
	for (size_t i = 0; i < va_len; i++) {
		int retc;
		kh_put(i32, hset_a, va[i], &retc);
		if (retc < 0) {
			fprintf(stderr, "o.o no puedo insertar la clave %d en el hashset. ¿Por qué?\n", va[i]);
			fprintf(stderr, "Error fatal.\n");
			exit(39);
		}
	}

	printf("Calcula el # de la intersección de los dos conjuntos...\n");
	khash_t(i32)* hset_b = kh_init(i32);
	size_t intersect_count = 0;
	for (size_t i = 0; i < vb_len; i++) {
		int retc;
		kh_put(i32, hset_b, vb[i], &retc);
		if (retc == 1) {
			khint_t it = kh_get(i32, hset_a, vb[i]);
			bool is_missing = (it == kh_end(hset_a));
			intersect_count += is_missing ? 0 : 1;
		}
	}

	printf("Cardinalidad encontrada: %zu\n", intersect_count);

	return 0;
}
