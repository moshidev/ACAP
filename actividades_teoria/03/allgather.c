#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

#define RANK_MASTER 0

void comprueba_invocacion_correcta(int argc, char** argv) {
    int rank, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    if (argc != 2) {
        MPI_Finalize();
        if (rank == RANK_MASTER) {
            fprintf(stderr, "No puedo ejecutar el programa si no me dices el texto a formar y distribuir.\n");
            fprintf(stderr, "Uso: %s [string]\n", argv[0]);
        }
        exit(0);
    }

    if (strlen(argv[1]) < nprocs) {
        MPI_Finalize();
        if (rank == RANK_MASTER) {
            fprintf(stderr, "No puedo ejecutar el programa si el tamaño del string es inferior al número de procesos.\n");
            fprintf(stderr, "Uso: %s [string]\n", argv[0]);
        }
        exit(0); 
    }
}

int main(int argc, char** argv) {
    int nprocs, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
    
    comprueba_invocacion_correcta(argc, argv);

    /* Calculamos la carga de trabajo correspondiente a cada proceso. */
    char* input_text;
    int chars_per_proc;
    if (rank == RANK_MASTER) {
        printf("\x1b[1;31mSoy Sir Proceso 0. Me han pedido que os comunique el siguiente mensaje:\n\x1b[0m\x1b[1m%s\x1b[0m\n\n", argv[1]);
        int input_text_len = strlen(argv[1]) + 1;
        chars_per_proc = input_text_len / nprocs + (input_text_len % nprocs ? 1 : 0);

        MPI_Alloc_mem(sizeof(char)*chars_per_proc * nprocs, MPI_INFO_NULL, &input_text);
        strcpy(input_text, argv[1]);
    }

    /* Comunicamos a los procesos la longitud de la carga */
    MPI_Bcast(&chars_per_proc, 1, MPI_INT, RANK_MASTER, MPI_COMM_WORLD);

    /* Comunicamos a cada proceso su carga correspondiente */
    char* lpartial_text;
    MPI_Alloc_mem(sizeof(char)*chars_per_proc, MPI_INFO_NULL, &lpartial_text);
    MPI_Scatter(input_text, chars_per_proc, MPI_CHAR, lpartial_text, chars_per_proc, MPI_CHAR, RANK_MASTER, MPI_COMM_WORLD);

    printf("Señor Proceso %d, le comunico que \x1b[1m%s\x1b[0m\n", rank, lpartial_text);   // MPI no garantiza que todos los procesos puedan escribir al stdout.

    /* Agrupamos en cada proceso la carga de todos los procesos, en orden */
    char* lfull_text;
    MPI_Alloc_mem(sizeof(char)*chars_per_proc * nprocs, MPI_INFO_NULL, &lfull_text);
    MPI_Allgather(lpartial_text, chars_per_proc, MPI_CHAR, lfull_text, chars_per_proc, MPI_CHAR, MPI_COMM_WORLD);

    printf("Soy Sir Proceso %d. Hago saber que \x1b[1m%s\x1b[0m\n", rank, lfull_text);

    if (rank == RANK_MASTER) {
        MPI_Free_mem(input_text);
    }
    MPI_Free_mem(lpartial_text);
    MPI_Free_mem(lfull_text);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}