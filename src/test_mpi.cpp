#include "../include/helper.h"
#include <stdio.h>
#include <vector>
#include <mpi.h>





int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    testRead(rank, size);


    // printf("asdads %d \n", rank);

    MPI_Finalize();
    return 0;
}