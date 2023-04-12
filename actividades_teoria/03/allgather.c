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
    char* texto_introducido;
    int chars_per_proc;
    if (rank == RANK_MASTER) {
        printf("\x1b[1;31mSoy Sir Proceso 0. Me han pedido que os comunique el siguiente mensaje:\n\x1b[0m\x1b[1m%s\x1b[0m\n\n", argv[1]);
        int texto_introducido_len = strlen(argv[1]) + 1;
        chars_per_proc = texto_introducido_len / nprocs + (texto_introducido_len % nprocs ? 1 : 0);

        MPI_Alloc_mem(chars_per_proc * nprocs, MPI_INFO_NULL, &texto_introducido);
        strcpy(texto_introducido, argv[1]);
    }

    /* Comunicamos a los procesos la longitud de la carga */
    MPI_Bcast(&chars_per_proc, 1, MPI_INT, RANK_MASTER, MPI_COMM_WORLD);

    /* Comunicamos a cada proceso su carga correspondiente */
    char* ltexto_parcial;
    MPI_Alloc_mem(chars_per_proc, MPI_INFO_NULL, &ltexto_parcial);
    MPI_Scatter(texto_introducido, chars_per_proc, MPI_CHAR, ltexto_parcial, chars_per_proc, MPI_CHAR, RANK_MASTER, MPI_COMM_WORLD);

    printf("Señor Proceso %d, le comunico que \x1b[1m%s\x1b[0m\n", rank, ltexto_parcial);   // MPI no garantiza que todos los procesos puedan escribir al stdout.

    /* Agrupamos en cada proceso la carga de todos los procesos, en orden */
    char* ltexto_completo;
    MPI_Alloc_mem(chars_per_proc * nprocs, MPI_INFO_NULL, &ltexto_completo);
    MPI_Allgather(ltexto_parcial, chars_per_proc, MPI_CHAR, ltexto_completo, chars_per_proc, MPI_CHAR, MPI_COMM_WORLD);

    printf("Soy Sir Proceso %d. Hago saber que \x1b[1m%s\x1b[0m\n", rank, ltexto_completo);

    if (rank == RANK_MASTER) {
        MPI_Free_mem(texto_introducido);
    }
    MPI_Free_mem(ltexto_parcial);
    MPI_Free_mem(ltexto_completo);

    MPI_Barrier(MPI_COMM_WORLD);
    MPI_Finalize();
    return 0;
}