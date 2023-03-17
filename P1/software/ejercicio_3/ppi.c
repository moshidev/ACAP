/**
 * Daniel Pedrosa Montes © 2023
 * Ejercicio 3b de la primera práctica de ACAP.
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

typedef double (*calcula_ppi_t)(int index, int size, int steps);

static double piLeibniz(int index, int size, int steps){
	double partpi = 0.0;
	double num = index % 2 ? -1.0 : 1.0;
	double denom = 1.0 + index*2.0;
	int len = index + size;
	for(int i = index; i<len; i++){
		partpi += num/denom;
		num = -1.0*num; // Alternamos el signo
		denom += 2.0;
	}
	return 4.0*partpi;
}

static double piRectangles(int index, int size, int steps){
	double width = 1.0/steps;
	double sum = 0.0, x;
	int lim = index + size;
	for(int i = index; i<lim; i++){
		x = (i + 0.5)*width;
		sum += 4.0/(1.0 + x*x);
	}
	return sum*width;
}

struct resultado {
	double pi;
	double cput;
	double wallt;
};

/**
 * @warning devuelve basura para cualquier world_rank distinto de 0.
 */
static struct resultado evalua(calcula_ppi_t metodo, int steps, int world_rank, int num_procs) {
	double wallt = get_wall_time();
	double local_cput = get_cpu_time();

	bool bingus = world_rank == (num_procs-1);
	int size = steps / num_procs;
	if (bingus) {
		size += steps % num_procs; // Priorizamos la claridad del código. No es el reparto más equitativo cuando tenemos pocos pasos.
	}
	int index = bingus ? steps - size : world_rank * size;

	double local_pi = metodo(index, size, steps);
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

	if(argc!=2){	//El primer argumento siempre es el nombre del programa
		printf("Uso: ./prog esfuerzo\n");
	}else{
		int steps = atoi(argv[1]);
		if(steps<=0 && world_rank == MASTER_RANK){
            printf("num_iter,num_procs,leibniz_cput,leibniz_wallt,rectangles_cput,rectangles_wallt,leibniz_pi,rectangles_pi\n");
		}else{
			struct resultado leibniz = evalua(piLeibniz, steps, world_rank, num_procs);
			struct resultado rectangulos = evalua(piRectangles, steps, world_rank, num_procs);
            
			if (world_rank == MASTER_RANK) {
				printf("%d,%d,%lf,%lf,%lf,%lf,%lf,%lf\n", steps, num_procs, leibniz.cput, leibniz.wallt, rectangulos.cput, rectangulos.wallt, leibniz.pi, rectangulos.pi);
			}
		}
	}

	MPI_Finalize();

	return 0;
}