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

    /*
    */
    Vertexset vs;
    
    Vertexset_Init(&vs, 500, MPI_COMM_WORLD);
    

    
    Vertexset_Add(&vs, rank);
    Vertexset_Add(&vs, 31-rank);
    Vertexset_Allreduce_Pure(&vs, VERTEXSET_OR);
    
    /*
    */
    
    for (size_t i = 0; i < vs.size; i++) {
        //printf("%ld ", vs.sizesAll[i]);
        printf("%d ", vs.sparseArray[i]);
    }
    printf("\n");
    

    /*
    // dynamic
    int sizes[4] = {2,1,5,7};
    int recievedSizes[4];
    
    MPI_Allgather(&sizes[rank], 1, MPI_INT, &recievedSizes, 1, MPI_INT, MPI_COMM_WORLD);
    
    for (size_t i = 0; i < 4; i++) {
        printf("%d ", recievedSizes[i]);
    }
    printf("\n");
    */
    



    /*
    int *sendarr, *recvarr;
    sendarr = (int*) malloc(2*sizeof(int));
    recvarr = (int*) malloc(8*sizeof(int));
    
    int counts[4] = {2, 2, 2, 2};
    int displ[4] = {0,2,4,6}; // evectively stride over counts
    
    for (int i = 0; i < 2; i++) {
        sendarr[i] = rank*10 + i;
        recvarr[i + 2*rank] = rank*10 + i;
    }

    MPI_Allgatherv(MPI_IN_PLACE, 0, MPI_INT,
    recvarr, counts, displ, MPI_INT,
    MPI_COMM_WORLD);

    if (true) {
        for (int i = 0; i < 8; i++) {
            printf("%d ", recvarr[i]);
        }
        printf("\n");
        
    }
    */
    
    
    MPI_Finalize();
    return 0;
}



