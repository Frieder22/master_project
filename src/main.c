#include "vertexSet.h"
#include <stdio.h>
#include <stdlib.h>
#include "mpi.h"

void printBinary(int num) {
    for (int i = sizeof(int) * 8 - 1; i >= 0; i--) {
        printf("%d", (num >> i) & 1);
    }
    printf("\n");
}

int main(int argc, char *argv[]) {
    int rank, size;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    Vertexset vs;
    Vertexset_Init(&vs, 500, MPI_COMM_WORLD);

    Vertexset_Add(&vs, 5 + rank);


    Vertexset_Allreduce_Dynamic(&vs, VERTEXSET_OR);

    //Vertexset_TransformToDense(&vs);
    Vertexset_PrintSet(&vs);
    
    MPI_Finalize();
    return 0;
}



