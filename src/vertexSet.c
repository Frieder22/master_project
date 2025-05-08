
#include "vertexSet.h"
#include "bitmap_custom.h"
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <mpi.h>
#include <assert.h>
#include <stdio.h>


void Vertexset_Init(Vertexset* vs, uint32_t maxsize){
    vs->size = 0;
    vs->maxsize = maxsize;

    // init bitarray
    size_t size_bitarray = (maxsize + 1) / sizeof(unsigned long long);
    vs->size_bitarray = size_bitarray;
    vs->bitarray = (unsigned long long*) malloc(size_bitarray * sizeof(unsigned long long));

    // init sparse array 
    // allocate to much memory to fill up whole array with all possible
    // vertices (no dublicates)
    vs->sparseArray = (uint32_t*) malloc(maxsize * sizeof(uint32_t));

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
        if (!Bitmap_Test(vs->bitarray, vertex)){
            vs->size++;
        }
        Bitmap_Set(vs->bitarray, vertex);
    } else {
        vs->sparseArray[vs->size] = vertex;
        vs->size++;
    }
};

bool Vertexset_Contains(Vertexset* vs, uint32_t vertex){
    assert(vertex < vs->maxsize);
    if (vs->isdense) {
        return Bitmap_Test(vs->bitarray, vertex);
    } else {
        for (size_t i = 0; i < vs->size; i++) {
            if (vs->sparseArray[i] == vertex) {
                return true;
            }
        }
        return false;
    }
};

bool Vertexset_TransformToDense(Vertexset* vs){
    size_t true_count = 0;
    Bitmap_Clean(vs->bitarray, vs->size_bitarray);
    for (size_t i = 0; i < vs->size; i++){
        uint32_t vertex = vs->sparseArray[i];
        if (!Bitmap_Test(vs->bitarray, vertex)) {
            true_count++;
        }
        Bitmap_Set(vs->bitarray, vertex);
    }
    vs->size = true_count;
    vs->isdense = true;
};

bool Vertexset_TransformToSparse(Vertexset* vs){
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
                    countAdded += 1;
                }
                word >>= 1; //shift one to right
            }
        }
    }
    // Check if all 1s are represented in the sparse array
    assert(countAdded == vs->size);
    vs->isdense = false;    
}

void Vertexset_Allreduce(Vertexset* vs, int VERTEXSET_OP);


void Vertexset_Deinit(Vertexset* vs){
    free(vs->bitarray);
    free(vs->sparseArray);
};

