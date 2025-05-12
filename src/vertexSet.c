#include "vertexSet.h"
#include "bitmap_custom.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <stdio.h>

int log2_floor(int x){
    int i;
    for (i = 0; i < 32; i++) {
        x = x >> 1;
        if (x <= 0){
            break;
        }
    }
    printf("%d ", i);
    printf("\n");
};

void Vertexset_Init(Vertexset* vs, uint32_t maxsize, MPI_Comm MPI_COMM){
    vs->maxsize = maxsize;
    
    // init bitarray
    size_t size_bitarray = (maxsize + 1) / sizeof(unsigned long long);
    vs->size_bitarray = size_bitarray;
    vs->bitarray = (unsigned long long*) malloc(size_bitarray * sizeof(unsigned long long));
    
    // init MPI stuff
    vs->MPI_COMM = MPI_COMM;
    MPI_Comm_rank(MPI_COMM, &vs->mpi_rank);
    MPI_Comm_size(MPI_COMM, &vs->mpi_size);
    
    // init sparse array 
    // allocate to much memory to fill up whole array with all possible
    // vertices (no dublicates)
    vs->sizeSparse = 0;
    vs->sparseArray = (uint32_t*) malloc(maxsize * sizeof(uint32_t));
    // same for buffer
    vs->bufferArray = (uint32_t*) malloc(maxsize * sizeof(uint32_t));
    // buffer for informations of other ranks
    vs->sizesAll = (int*) malloc(vs->mpi_size * sizeof(int));
    vs->displ= (int*) malloc((vs->mpi_size + 1) * sizeof(int));

    // check if systems definition of ULL is compatible with this implementation
    assert(sizeof(unsigned long long) == 8); // 8 byte (=64 bit)
    // calculate size, where dense variant is more efficient
    vs->sizeCrit = size_bitarray * sizeof(unsigned long long) / sizeof(uint32_t);

    // set default as sparse
    vs->isdense = false;
};

void Vertexset_Add(Vertexset* vs, uint32_t vertex){
    assert(vertex < vs->maxsize);
    if (vs->isdense) {
        Bitmap_Set(vs->bitarray, vertex);
    } else {
        vs->sparseArray[vs->sizeSparse] = vertex;
        vs->sizeSparse++;
    }
};

bool Vertexset_Contains(Vertexset* vs, uint32_t vertex){
    assert(vertex < vs->maxsize);
    if (vs->isdense) {
        return Bitmap_Test(vs->bitarray, vertex);
    } else {
        for (size_t i = 0; i < vs->sizeSparse; i++) {
            if (vs->sparseArray[i] == vertex) {
                return true;
            }
        }
        return false;
    }
};

bool Vertexset_TransformToDense(Vertexset* vs){
    // only do, if it's sparse
    if (!vs->isdense){
        Bitmap_Clean(vs->bitarray, vs->size_bitarray);
        for (size_t i = 0; i < vs->sizeSparse; i++){
            uint32_t vertex = vs->sparseArray[i];
            Bitmap_Set(vs->bitarray, vertex);
        }
        vs->isdense = true;
    }
    
};

bool Vertexset_TransformToSparse(Vertexset* vs){
    // only do, if it is dense
    if (vs->isdense){
        unsigned long long word;
        size_t countAdded = 0;
        for (size_t i = 0; i < vs->size_bitarray; i++){
            // find a word thats not 0
            if (vs->bitarray[i] != 0ULL){
                word = vs->bitarray[i]; 
                // find the 1s in the word
                for (int j = 0; j < ulong_bits; j++) {
                    if (word &1 == 1) { //found a 1
                        // add to sparse list
                        vs->sparseArray[countAdded] = i * ulong_bits + j;
                        countAdded ++;
                    }
                    word >>= 1; //shift one to right
                }
            }
        }
        vs->sizeSparse = countAdded;
        vs->isdense = false;    
    }
}

void Vertexset_Allreduce(Vertexset* vs, int VERTEXSET_OP){
    // no other variant is implemented yet
    // use only sparse communication
    Vertexset_TransformToSparse(vs);
    Vertexset_Allreduce_Pure(vs, VERTEXSET_OP);
}


void Vertexset_Allreduce_Pure(Vertexset* vs, int VERTEXSET_OPERATION){
    assert(VERTEXSET_OPERATION == VERTEXSET_OR); //no other version is implemented
    if (vs->isdense) {
        MPI_Allreduce(MPI_IN_PLACE, vs->bitarray, vs->size_bitarray, MPI_UINT64_T, MPI_BOR, MPI_COMM_WORLD);
    } else {
        // get size information from other ranks
        int size_int = (int) vs->sizeSparse;
        MPI_Allgather(&size_int, 1, MPI_INT, vs->sizesAll, 1, MPI_INT, vs->MPI_COMM);

        // calculate displacements with exxclusive scan
        vs->displ[0] = 0;
        for (int i = 1; i < vs->mpi_size + 1; i++){
            vs->displ[i] = vs->displ[i-1] + vs->sizesAll[i-1];
        }

        // gather of all subarrays
        MPI_Allgatherv(vs->sparseArray, vs->sizeSparse, MPI_UINT32_T, vs->bufferArray, vs->sizesAll, (vs->displ), MPI_UINT32_T, vs->MPI_COMM);

        printf("first sparse element: %d \n", vs->displ[0]);
        // switch pointer
        vs->sparseArray = vs->bufferArray;
        vs->bufferArray = NULL;

        // set size
        vs->sizeSparse = vs->displ[vs->mpi_size];
    }
};


void Vertexset_Allreduce_Dynamic(Vertexset* vs, int VERTEXSET_OPERATION){
    // Check if OR operation is used (other is not implemented yet)
    assert(VERTEXSET_OPERATION == VERTEXSET_OR);

    // Check if size of Communicatior is power of 2
    assert((vs->mpi_size & (vs->mpi_size - 1)) == 0);
    
    // all processes should have a 
    if (vs->isdense) {
        Vertexset_TransformToSparse(vs);
    }

    // calculate numver of communication rounds
    int iterations = log2_floor(vs->mpi_size);
    
    
    for (int round = 0; round < iterations; round++) {
        
    }
    

};


void Vertexset_PrintSet(Vertexset* vs){
    if(vs->isdense){
        Vertexset_TransformToSparse(vs);
    }
    int max_elements = 30;
    printf("Elements in rank %d: ", vs->mpi_rank);
    for (size_t i = 0; i < vs->sizeSparse; i++) {
        printf("%d ", vs->sparseArray[i]);
        if (i > max_elements) {
            break;
        }
        
    }
    printf("\n");    
};


void Vertexset_Deinit(Vertexset* vs){
    free(vs->bitarray);
    free(vs->sparseArray);
};



