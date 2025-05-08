#include "vertexSet.h"
#include <stdio.h>
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
    
    Vertexset_Init(&vs, 500);
    
    Vertexset_TransformToDense(&vs);
    
    Vertexset_Add(&vs, rank);
    Vertexset_Add(&vs, 31-rank);
    Vertexset_Allreduce_Pure(&vs, VERTEXSET_OR);
    
    
    if (Vertexset_Contains(&vs, 31)){
        printf("Correct\n");
    }
    

    MPI_Finalize();
    return 0;
}



