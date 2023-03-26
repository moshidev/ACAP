/**
 * Daniel Pedrosa Montes © 2023
 * Resolución del primer ejercicio de la segunda práctica de Arquitectura y Computación de Altas Prestaciones.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define RANK_MASTER     0
#define RANK_TOUPPER    1
#define RANK_REALSUM    2
#define RANK_INTSUM     3

static void assert_numProcs_is_4(int numProcs, int rank) {
    if (numProcs != 4) {
        MPI_Finalize();
        if (rank == 0) {
            fprintf(stderr, "Number of processes launched are %d. Expected 4.\n", numProcs);
            exit(1);
        }
        exit(0);
    }
}

static void kill_em_uwu(void) {
    fprintf(stdout, "KILL EM ALL!!! >>>> w <<<<.\n");
    MPI_Abort(MPI_COMM_WORLD, 0);
    MPI_Finalize();
    exit(0);
}

static void wipe_stdin_line(void) {
    char input = 0;
    while (input != '\n') {
        input = getchar();
    }
}

static void master_routine(void) {
    while (true) {
        char input = getchar();
        switch (input) {
            case '\n':
                break;
            case '0':
                kill_em_uwu();
                break;
            case '1':
                fprintf(stderr, "Not implemented.\n"); 
                break;
            case '2':
                fprintf(stderr, "Not implemented.\n"); 
                break;
            case '3':
                fprintf(stderr, "Not implemented.\n"); 
                break;
            case '4':
                fprintf(stderr, "Not implemented.\n"); 
                break;
            default:
                fprintf(stderr, "Expected a decimal number in the range [0,4].\n");
                wipe_stdin_line();
                break;
        }
    }
}

static void toupper_worker(void) {
    while (true) {
    }
}

static void realsum_worker(void) {
    while (true) {
    }
}

static void intsum_worker(void) {
    while (true) {
    }
}

int main(int argc, char* argv[]) {
    MPI_Init(&argc, &argv);
    int rank, numProcs;
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);

    assert_numProcs_is_4(numProcs, rank);
    switch(rank) {
        case RANK_MASTER:
            master_routine();
            break;
        case RANK_TOUPPER:
            toupper_worker();
            break;
        case RANK_REALSUM:
            realsum_worker();
            break;
        case RANK_INTSUM:
            intsum_worker();
            break;
        default:
            fprintf(stderr, "w-wait. how!?\?!?!?!?! this mpi instance is brukeeen 4 shure.\n"); // < Beware of the trigraphs OwO
            exit(39);
            break;
    }

    MPI_Finalize();
    exit(0);
}