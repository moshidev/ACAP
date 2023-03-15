/**
 * Daniel Pedrosa Montes © 2023
 * Cálculo del producto vectorial con MPI a partir del cuarto ejemplo del primer seminario de ACAP.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

//N.C. Cruz (UGR). Resolution proposal for the 4th example of the first seminar

#define MASTER 0
#define DESTROY_TAG 666
#define NORMAL_TAG 1

static int job(const int* u, const int* v, int numItems) {
	int sum = 0;
	
	for (int i = 0; i < numItems; i++) {
		sum += u[i]*v[i];
	}

	return sum;	
}

static void worker(int rank, int numProcs){
	MPI_Status status;
	MPI_Probe(MASTER, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
	if(status.MPI_TAG == NORMAL_TAG){
		int numItems = 0;
		MPI_Get_count(&status, MPI_INT, &numItems);//printf("[%d] Expected: %d\n", rank, numItems);
		int* u = malloc(sizeof(int)*numItems);
		int* v = malloc(sizeof(int)*numItems);
		MPI_Recv(u, numItems, MPI_INT, MASTER, NORMAL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(v, numItems, MPI_INT, MASTER, NORMAL_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		int partialResult = job(u, v, numItems);
		MPI_Send(&partialResult, 1, MPI_INT, MASTER, NORMAL_TAG, MPI_COMM_WORLD);
		free(u);
		free(v);
	}
}

static int scalarProduct(const int* u, const int* v, int dimension, int numProcs) {
	int numWorkers = numProcs - 1;
	int pack = dimension / numProcs;
	int offset = dimension % numProcs;
	int focus = 0, sentSize = 0;
	for(int i = 0; i<numWorkers; i++){
		sentSize = (pack + (i<offset));//printf("[%d, %d)\n", focus, focus+sentSize);
		MPI_Send(&(u[focus]), sentSize, MPI_INT, (i+1), NORMAL_TAG, MPI_COMM_WORLD);
		MPI_Send(&(v[focus]), sentSize, MPI_INT, (i+1), NORMAL_TAG, MPI_COMM_WORLD);
		focus += sentSize;
	}
	//------------------
	MPI_Request request[numWorkers];
	int buffer[numWorkers];
	int resultado = 0;
	for (int i = 0; i<numWorkers; i++){
		MPI_Irecv(&buffer[i], 1, MPI_INT, (i+1), NORMAL_TAG, MPI_COMM_WORLD, &request[i]);
	}
	resultado += job(u + focus, v + focus, dimension - focus);
	for (int i = 0; i<numWorkers; i++){
		MPI_Status status;
		MPI_Wait(&request[i], &status);
		resultado += buffer[i];
	}
	return resultado;
}

static void masterTask(int rank, int numProcs, int dataSize){
	int* vector = malloc(sizeof(int)*dataSize);
	for(int i = 0; i<dataSize; i++){
		vector[i] = i;
	}
	//------------------
	int resultado = scalarProduct(vector, vector, dataSize, numProcs);
	printf("El resultado es: %d\n", resultado);
	//------------------	
	free(vector);
}

void shutDown(int numProcs){
	for(int i = 1; i<numProcs; i++){//Do not count the master!
		MPI_Send(0, 0, MPI_INT, i, DESTROY_TAG, MPI_COMM_WORLD);//https://stackoverflow.com/questions/10403211/mpi-count-of-zero-is-often-valid
	}
}

int main(int argc, char* argv[]){
	int rank, numProcs;
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &numProcs);
	if(rank==MASTER){
		if(numProcs<2){
			printf("Error: At least 2 processes required!\n");
			shutDown(numProcs);
		}else{
			if(argc!=2){ // Name & Param
			printf("Error: Vector size expected\n");
			shutDown(numProcs);
		}else{
			int dataSize = atoi(argv[1]);
			if(dataSize<=0){
				printf("Error: Invalid data size\n");
				shutDown(numProcs);
			}else{
				masterTask(rank, numProcs, dataSize); 
			}
		}
		}
	}else{
		worker(rank, numProcs);
	}
	MPI_Finalize();
	return 0;
}
