#include "khash.h"
#include <pthread.h>
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

int main(int argc, char** argv) {
	assert_argc(argc, argv);

	size_t va_len = strtoul(argv[1], 0, 0);
	size_t vb_len = strtoul(argv[2], 0, 0);
	char smallest_v_char = va_len < vb_len ? 'a' : 'b';
	size_t smallest_v_len = smallest_v_char == 'a' ? va_len : vb_len; 

	assert_set_len(va_len, vb_len, argv[0], argv[1], argv[2]);

	printf("Reserva memoria...\n");
	int32_t* va = calloc(va_len, sizeof(int32_t));
	int32_t* vb = calloc(vb_len, sizeof(int32_t));
	int32_t* smallest_v = smallest_v_char == 'a' ? va : vb;

	printf("Inicializa vectores...\n");
	for (size_t i = 0; i < smallest_v_len; i++) {
		if (i < va_len) {
			va[i] = i%1000;
		}
		if (i < vb_len) {
			vb[i] = i%1000;
		}
	}

	return 0;
}
