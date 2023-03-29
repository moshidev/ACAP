/**
 * Daniel Pedrosa Montes © 2023
 * Resolución del primer ejercicio de la segunda práctica de Arquitectura y Computación de Altas Prestaciones.
 */

#include <mpi.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdbool.h>

#define RANK_MASTER         0
#define RANK_TOUPPER        1
#define RANK_REALSUMROOT    2
#define RANK_INTSUM         3

#define TAG_TOUPPER         1
#define TAG_REALSUMROOT     2

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
    fprintf(stdout, "KILL EM ALL!!! O w O :knife:.\n");
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

static void toupper_worker(void) {
    char s[512];
    while (true) {
        MPI_Status status;
        MPI_Recv(s, 512, MPI_CHAR, RANK_MASTER, TAG_TOUPPER, MPI_COMM_WORLD, &status);
        s[511] = '\0';
        for (int i = 0; i < 512 && s[i] != '\0'; i++) {
            s[i] = toupper(s[i]);
        }
        MPI_Send(s, status._ucount, MPI_CHAR, RANK_MASTER, TAG_TOUPPER, MPI_COMM_WORLD);
    }
}

static void toupper_stub(char s[512], unsigned count) {
    count = count > 512 ? 512 : count;
    MPI_Send(s, count, MPI_CHAR, RANK_TOUPPER, TAG_TOUPPER, MPI_COMM_WORLD);
    MPI_Recv(s, count, MPI_CHAR, RANK_TOUPPER, TAG_TOUPPER, MPI_COMM_WORLD, 0);
}

static void handle_option_1(void) {
    int chars_read = 0;
    char sentence[512];

    scanf("%511[^\n]%n", sentence, &chars_read);
    if (chars_read == 0) {
        fprintf(stderr, "None read. Nothing to uppercase.\n");
    }
    else {
        if (chars_read == 511) {
            wipe_stdin_line();
            sentence[511] = '\0';
            fprintf(stderr, " **** Warning. Sentence truncated to the first 511 characters. **** \n");
        }
        toupper_stub(sentence, chars_read+1);
        printf("%s\n", sentence);
    }
}

struct realsumroot {
    double summatory;
    double sqrt_of_summatory;
};

static void realsumroot_worker(void) {
    double* sum_list = 0;
    while (true) {
        double results[2] = {0.0};
        MPI_Status status;
        int count;
        MPI_Probe(RANK_MASTER, TAG_REALSUMROOT, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_DOUBLE, &count);
        MPI_Alloc_mem(count*sizeof(double), MPI_INFO_NULL, &sum_list);
        MPI_Recv(sum_list, count, MPI_DOUBLE, RANK_MASTER, TAG_REALSUMROOT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (size_t i = 0; i < count; i++) {
            results[0] += sum_list[i];
        }
        results[1] = sqrt(results[0]);
        MPI_Send(results, 2, MPI_DOUBLE, RANK_MASTER, TAG_REALSUMROOT, MPI_COMM_WORLD);
        MPI_Free_mem(sum_list);
        sum_list = 0;
    }
}

static struct realsumroot realsumroot_stub(const double real_list[], size_t length) {
    double recv[2];
    MPI_Send(real_list, length, MPI_DOUBLE, RANK_REALSUMROOT, TAG_REALSUMROOT, MPI_COMM_WORLD);
    MPI_Recv(recv, 2, MPI_DOUBLE, RANK_REALSUMROOT, TAG_REALSUMROOT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    struct realsumroot res = {
        .summatory = recv[0],
        .sqrt_of_summatory = recv[1],
    };
    return res;
}

static void handle_option_2(void) {
    const double real_list[] = {1.1, 2.2, 3.3, 4.4, 5.5, 6.6, 7.7, 8.8, 9.9, 10.1};
    const struct realsumroot res = realsumroot_stub(real_list, sizeof(real_list)/sizeof(double));
    printf("summatory,sqrt_of_summatory\n%.46f,%.46f\n", res.summatory, res.sqrt_of_summatory);
}

static void intsum_worker(void) {
    while (true) {
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
                wipe_stdin_line();
                handle_option_1();
                break;
            case '2':
                handle_option_2();
                break;
            case '3':
                fprintf(stderr, "Not implemented.\n"); 
                break;
            case '4':
                fprintf(stderr, "Not implemented.\n"); 
                break;
            default:
                wipe_stdin_line();
                fprintf(stderr, "Expected a decimal number in the range [0,4].\n");
                break;
        }
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
        case RANK_REALSUMROOT:
            realsumroot_worker();
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