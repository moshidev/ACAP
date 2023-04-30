/**
 * Daniel Pedrosa Montes © 2023
 * Ejercicio 4 de la tercera práctica de ACAP.
 * MPI_Reduce: https://mpitutorial.com/tutorials/mpi-reduce-and-allreduce/
 */

// Calculo aproximado de PI mediante la serie de Leibniz e integral del cuarto de circulo
// https://es.wikipedia.org/wiki/Serie_de_Leibniz
// N.C. Cruz, Universidad de Granada, 2023
#include <mpi.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <sys/time.h>

#define MASTER_RANK 0

static double get_wall_time() {
    struct timeval time;
    assert(gettimeofday(&time,NULL) == 0);
    return (double)time.tv_sec + (double)time.tv_usec * .000001;
}

static double get_cpu_time() {
    return (double)clock() / CLOCKS_PER_SEC;
}

typedef struct calcula_ppi_params {
	int index;
	int size;
	int steps;
} calcula_ppi_params_t;

typedef void* (*calcula_ppi_t)(calcula_ppi_params_t*);

static void* piLeibniz(calcula_ppi_params_t* p){
	double partpi = 0.0;
	double num = p->index % 2 ? -1.0 : 1.0;
	double denom = 1.0 + p->index*2.0;
	int len = p->index + p->size;
	for(int i = p->index; i<len; i++){
		partpi += num/denom;
		num = -1.0*num; // Alternamos el signo
		denom += 2.0;
	}
	double* r = malloc(sizeof(double));
	*r = 4.0*partpi;
	pthread_exit(r);
}

static void* piRectangles(calcula_ppi_params_t* p){
	double width = 1.0/p->steps;
	double sum = 0.0, x;
	int lim = p->index + p->size;
	for(int i = p->index; i<lim; i++){
		x = (i + 0.5)*width;
		sum += 4.0/(1.0 + x*x);
	}
	double* r = malloc(sizeof(double));
	*r = sum*width;
	pthread_exit(r);
}

struct resultado {
	double pi;
	double cput;
	double wallt;
};

/**
 * @warning devuelve basura para cualquier world_rank distinto de 0.
 */
static struct resultado evalua(calcula_ppi_t metodo, int steps, int world_rank, int num_procs, int threads_per_proc) {
	double wallt = get_wall_time();
	double local_cput = get_cpu_time();

	/* Cálculo a nivel de MPI */
	bool bingus = world_rank == (num_procs-1);
	int size = steps / num_procs;
	if (bingus) {
		size += steps % num_procs;
	}
	int index = bingus ? steps - size : world_rank * size;

	/* Cálculo a nivel de hebra */
	int tsize = size / threads_per_proc;
	pthread_t thread[threads_per_proc];
	calcula_ppi_params_t argumentos_metodo[threads_per_proc];
	for (int i = 0; i < threads_per_proc; i++) {
		argumentos_metodo[i].size = i == threads_per_proc-1 ? tsize + (size % threads_per_proc) : tsize;
		argumentos_metodo[i].index = i == threads_per_proc-1 ? index + size - argumentos_metodo[i].size : index + (i * tsize);
		argumentos_metodo[i].steps = steps;
		pthread_create(&thread[i], NULL, (void*(*)(void*))metodo, &argumentos_metodo[i]);
	}

	double local_pi = 0.0;
	for (int i = 0; i < threads_per_proc; i++) {
		double* thread_local_pi;
		pthread_join(thread[i], (void**)&thread_local_pi);
		local_pi += *thread_local_pi;
		free(thread_local_pi);
	}

	local_cput = get_cpu_time() - local_cput;
	wallt = get_wall_time() - wallt;
	const double local_res[2] = {local_pi, local_cput};
	double global_res[2];
	MPI_Reduce(local_res, global_res, 2, MPI_DOUBLE, MPI_SUM, MASTER_RANK, MPI_COMM_WORLD);

	struct resultado resultado = {
		.pi = global_res[0],
		.cput = global_res[1],
		.wallt = wallt,
	};

	return resultado;
}

int main(int argc, char* argv[]){
	MPI_Init(&argc, &argv);
	int world_rank, num_procs;
	MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

	if(argc!=3){
		printf("Uso: %s [n. iter] [hilos por proceso]\n", argv[0]);
	}else{
		int steps = atoi(argv[1]);
		int threads_per_proc = atoi(argv[2]);
		if (threads_per_proc <= 0) {
			if (world_rank == MASTER_RANK) {
				printf("¡Necesito hebras que trabajen para mi cometido!\n");
			}
		}
 		else if(steps<=0){
			if (world_rank == MASTER_RANK) {
				printf("¡Necesito un número mínimo de pasos!\n");
			}
		}
		else {
			if (world_rank == MASTER_RANK) {
            	printf("num_iter,num_procs,threads_per_proc,leibniz_cput,leibniz_wallt,rectangles_cput,rectangles_wallt,leibniz_pi,rectangles_pi\n");
			}
			struct resultado leibniz = evalua(piLeibniz, steps, world_rank, num_procs, threads_per_proc);
			struct resultado rectangulos = evalua(piRectangles, steps, world_rank, num_procs, threads_per_proc);
            
			if (world_rank == MASTER_RANK) {
				printf("%d,%d,%d,%lf,%lf,%lf,%lf,%lf,%lf\n", steps, num_procs, threads_per_proc, leibniz.cput, leibniz.wallt, rectangulos.cput, rectangulos.wallt, leibniz.pi, rectangulos.pi);
			}
		}
	}

	MPI_Finalize();

	return 0;
}