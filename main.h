#ifndef MAIN_H
#define MAIN_H

typedef struct coordinator {
    int *workers;
    int length;
}coordinator;

void readFile(char *fileName, coordinator *coords, int coordinatorNumber);
void recvCluster(int from, coordinator *coords);
void sendArray(int from, int dest, int *array, int length);
int recvArray(int from, int **array);
void sendCoordinatorToWorker(int coordNumber, coordinator *coords);
void sendTopologyToWorker(int coordNumber, coordinator *coords);
void recvTopologyToWorker(int from, coordinator *coords);
int splitArrayToWorkers(int *array, coordinator *coords, int arraySize, int numberOfWorkers, int coordinatorRank);
void gatherArrayFromWorkers(int coordinatorNumber, coordinator *coords, int **result, int splitSize);
void printTopology(int rank, coordinator *coords);

#endif
