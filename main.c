#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "main.h"
#define REPR 3

int main(int argc, char *argv[]) {
    int numtasks, rank;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    coordinator *coords = (coordinator *)calloc(REPR, sizeof(coordinator));
    int coordinatorNumber = -1;
    int *array;
    int arraySize;
    int numberOfWorkers = numtasks - REPR;

    if (rank == 0) {
        readFile("cluster0.txt", coords, 0);
        arraySize = atoi(argv[1]);

        sendArray(0, 1, coords[0].workers, coords[0].length);
        sendArray(0, 2, coords[0].workers, coords[0].length);
        recvCluster(1, coords);
        recvCluster(2, coords);
        printTopology(rank, coords);
        sendCoordinatorToWorker(0, coords);
        sendTopologyToWorker(0, coords);

        array = (int *)calloc(arraySize, sizeof(int));
        for (int i = 0; i < arraySize; i++) {
            array[i] = i;
        }

        sendArray(0, 1, array, arraySize);
        sendArray(0, 2, array, arraySize);

        int splitSize = splitArrayToWorkers(array, coords, arraySize, numberOfWorkers, 0);

        int *gather0, *gather1, *gather2;
        int gatherSize0, gatherSize1, gatherSize2; 
        gatherArrayFromWorkers(0, coords, &gather0, splitSize);
        gatherSize1 = recvArray(1, &gather1);
        gatherSize2 = recvArray(2, &gather2);

        int index = 0;
        for (int i = 0; i < splitSize; i++) {
            array[index++] = gather0[i];
        }
        for (int i = 0; i < gatherSize1; i++) {
            array[index++] = gather1[i];
        }
        for (int i = 0; i < gatherSize2; i++) {
            array[index++] = gather2[i];
        }
        printf("Rezultat: ");
        for (int i = 0; i < arraySize; i++) {
            printf("%d ", array[i]);
        }
        printf("\n");

    } else if (rank == 1) {
        readFile("cluster1.txt", coords, 1);

        sendArray(1, 0, coords[1].workers, coords[1].length);
        sendArray(1, 2, coords[1].workers, coords[1].length);
        recvCluster(0, coords);
        recvCluster(2, coords);
        printTopology(rank, coords);
        sendCoordinatorToWorker(1, coords);
        sendTopologyToWorker(1, coords);

        arraySize = recvArray(0, &array);
        int splitSize = splitArrayToWorkers(array, coords, arraySize, numberOfWorkers, 1);

        int *gather1;
        gatherArrayFromWorkers(1, coords, &gather1, splitSize);
        sendArray(1, 0, gather1, splitSize);
        printf("M(1,0)\n");

    } else if (rank == 2) {
        readFile("cluster2.txt", coords, 2);

        sendArray(2, 0, coords[2].workers, coords[2].length);
        sendArray(2, 1, coords[2].workers, coords[2].length);
        recvCluster(0, coords);
        recvCluster(1, coords);
        printTopology(rank, coords);
        sendCoordinatorToWorker(2, coords);
        sendTopologyToWorker(2, coords);

        arraySize = recvArray(0, &array);
        int splitSize = splitArrayToWorkers(array, coords, arraySize, numberOfWorkers, 2);

        int *gather2;
        gatherArrayFromWorkers(2, coords, &gather2, splitSize); 
        sendArray(2, 0, gather2, splitSize);
        printf("M(2,0)\n"); 

    } else {
        MPI_Recv(&coordinatorNumber, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        recvTopologyToWorker(coordinatorNumber, coords);
        printTopology(rank, coords);

        arraySize = recvArray(coordinatorNumber, &array);
        for (int i = 0; i < arraySize; i++) {
            array[i] = 2 * array[i];
        }

        sendArray(rank, coordinatorNumber, array, arraySize);
        printf("M(%d,%d)\n", rank, coordinatorNumber);
    }

    MPI_Finalize();
}

/**
 * Function that reads data from a file and adds it to an array of type coordinator.
 * @param filenName - path of the file.
 * @param coords - array of coordinators where coords[n] stores the child processes for
 * the nth coordinator.
 * @param coordinatorNumber - rank of the coordinator process. 
 */
void readFile(char *fileName, coordinator *coords, int coordinatorNumber) {
    FILE *fp;
    int numberOfWorkers;
    int *workers;

    fp = fopen(fileName, "r");
    fscanf(fp, "%d", &numberOfWorkers);
    workers = (int *)calloc(numberOfWorkers, sizeof(int));
    for (int i = 0; i < numberOfWorkers; i++) {
        fscanf(fp, "%d", &workers[i]);
    }

    coords[coordinatorNumber].workers = workers;
    coords[coordinatorNumber].length = numberOfWorkers;
}

/**
 * Function that receives a cluster.
 * @param from - rank of the sending process.
 * @param coords - array of coordinators to store the received array
 */
void recvCluster(int from, coordinator *coords) {
    MPI_Recv(&coords[from].length, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    coords[from].workers = (int *)calloc(coords[from].length, sizeof(int));
    MPI_Recv(coords[from].workers, coords[from].length, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}

/**
 * Function that sends an array.
 * @param from - rank of the process which sends the information.
 * @param dest - rank of the process which receives the information.
 * @param array - array to be send.
 * @param length - length of the array.
 */
void sendArray(int from, int dest, int *array, int length) {
    MPI_Send(&length, 1, MPI_INT, dest, 0, MPI_COMM_WORLD);
    printf("M(%d,%d)\n", from, dest);
    MPI_Send(array, length, MPI_INT, dest, 0, MPI_COMM_WORLD);
    printf("M(%d,%d)\n", from, dest);
}

/**
 * Function that receives an array.
 * @param from - rank of the sending process.
 * @param array - array for storing the received information.
 * @return - length of the received array.
 */
int recvArray(int from, int **array) {
    int length = 0;
    MPI_Recv(&length, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    *array = (int *)calloc(length, sizeof(int));
    MPI_Recv(*array, length, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    return length;
}

/**
 * Function that sends the coordinator to all its child workers.
 * @param coordNumber - number of the coordinator to be send.
 * @param coords - array of workers
 */
void sendCoordinatorToWorker(int coordNumber, coordinator *coords) {
    for (int i = 0; i < coords[coordNumber].length; i++) {
        MPI_Send(&coordNumber, 1, MPI_INT, coords[coordNumber].workers[i], 0, MPI_COMM_WORLD);
        printf("M(%d,%d)\n", coordNumber, coords[coordNumber].workers[i]);
    }
}

/**
 * Function that sends full topology to child workers for a given coordinator process.
 * @param coordNumber - rank of the coordinator process.
 * @param coords - array of workers.
 */
void sendTopologyToWorker(int coordNumber, coordinator *coords) {
    for (int i = 0; i < coords[coordNumber].length; i++) {
        sendArray(coordNumber, coords[coordNumber].workers[i], coords[0].workers, coords[0].length);
        sendArray(coordNumber, coords[coordNumber].workers[i], coords[1].workers, coords[1].length);
        sendArray(coordNumber, coords[coordNumber].workers[i], coords[2].workers, coords[2].length);
    }
}

/**
 * Function that receives the topology and stores it.
 * @param from - rank of coordinator proccess which sends the topology.
 * @param coords - array in which we store the topology.
 */
void recvTopologyToWorker(int from, coordinator *coords) {
    MPI_Recv(&coords[0].length, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    coords[0].workers = (int *)calloc(coords[0].length, sizeof(int));
    MPI_Recv(coords[0].workers, coords[0].length, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Recv(&coords[1].length, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    coords[1].workers = (int *)calloc(coords[1].length, sizeof(int));
    MPI_Recv(coords[1].workers, coords[1].length, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    MPI_Recv(&coords[2].length, 1, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    coords[2].workers = (int *)calloc(coords[2].length, sizeof(int));
    MPI_Recv(coords[2].workers, coords[2].length, MPI_INT, from, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
}


/**
 * Function which performs the split operation to child workers of a given coordinator. Depending on
 * the coordinator rank and also on the workers each coordinator will split a chunk of the initial array.
 * @param array - represents the initial array that will be split.
 * @param coords - array of workers.
 * @param arraySize - size of the array that will be split.
 * @param numberOfWorkers - total number of all workers.
 * @param coordinatorRank - rank of the coordinator which performs the split.
 * @return - total split length for each coordinator.
 */
int splitArrayToWorkers(int *array, coordinator *coords, int arraySize, int numberOfWorkers, int coordinatorRank) {
    int chunkSize = arraySize / numberOfWorkers;
    int reminder = arraySize % numberOfWorkers;

    int arrayIndex = 0;
    int splitSize = 0;
    for (int i = 0; i < coordinatorRank; i++) {
        arrayIndex += chunkSize * coords[i].length;   
    }
    for (int i = 0; i < coords[coordinatorRank].length; i++) {
        if (coordinatorRank == REPR - 1 && i == coords[coordinatorRank].length - 1) {
            chunkSize += reminder;
        }
        int *auxArray = (int *)calloc(chunkSize, sizeof(int));
        for (int j = 0; j < chunkSize; j++) {
            auxArray[j] = array[arrayIndex++];
        }
        sendArray(coordinatorRank, coords[coordinatorRank].workers[i], auxArray, chunkSize);
        splitSize += chunkSize;
        printf("M(%d,%d)\n", coordinatorRank, coords[coordinatorRank].workers[i]);
        free(auxArray);
    }

    return splitSize;
}

/**
 * Function that gather the split arrays into one single array
 * @param coordinatorNumber - rank of the coordinator process.
 * @param coords - array of workers.
 * @param result - array used for gather operation.
 * @param splitSize - size of the result(gathered array).
 */
void gatherArrayFromWorkers(int coordinatorNumber, coordinator *coords, int **result, int splitSize) {
    int index;
    int *aux;

    *result = (int *)calloc(splitSize, sizeof(int));
    index = 0;
    for (int i = 0; i < coords[coordinatorNumber].length; i++) {
        int arraySize = recvArray(coords[coordinatorNumber].workers[i], &aux);
        for (int j = 0; j < arraySize; j++) {
            (*result)[index++] = aux[j];
        }
    }
}

/**
 * Function that prints the topology for a given process.
 * @param rank - rank of the process.
 * @param coords - array in which the topology is stored.
 */
void printTopology(int rank, coordinator *coords) {
    printf("%d -> ", rank);
    for (int i = 0; i < REPR; i++) {
        printf("%d:", i);
        for (int j = 0; j < coords[i].length; j++) {
            if (j != coords[i].length - 1) {
                printf("%d,", coords[i].workers[j]);
            } else {
                printf("%d", coords[i].workers[j]);
            }
        }
        if (i != 2) {
            printf(" ");
        }
    }
    printf("\n");
}
