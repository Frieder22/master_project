#include "vertexSet.h"
#include "vertexSetIterator.h"
#include "bitmap_custom.h"

void vertexSetIterator_Init(vertexSetIterator* it, Vertexset* vs) {
    // set corresponding vertexx set
    it->vs = vs;

    // set corresponding representation
    it->isdense = vs->isdense;

    // setting for dense representation
    if (it->isdense) {
        it->wordIdx = 0;
        it->wordShift = 0;
    } else {
        // setting for sparse representation
        it->sparseIdx = 0;
    } 
};

bool vertexSetIterator_next(vertexSetIterator* it){
    if (it->isdense) {
        unsigned long long word;
        for ( size_t i = it->wordIdx; i < it->vs->size_bitarray; i++){
            // find a word thats not 0
            if (it->vs->bitArray[i] != 0ULL){
                word = it->vs->bitArray[i];  // NEEDS Correction!
                // find the 1s in the word
                for (int j = it->wordShift; j < ulong_bits; j++) {
                    if (word &1 == 1) { //found a 1
                        // save current vertex
                        it->currVertex = i * ulong_bits + j;

                        //shift one to right
                        word >>= 1; 

                        // save progress 
                        it->word = word;
                        it->wordIdx = i;
                        it->wordShift = j;
                        return true;
                    }
                    word >>= 1; //shift one to right
                }
            }
        }
        return false;
    } else {
        if (it->sparseIdx != it->vs->sizeSparse) {
            it->sparseIdx++;
            it->currVertex = it->vs->sparseArray[it->sparseIdx];
            return true;
        }
        return false;
    }
};

uint32_t vertexSetIterator_get(vertexSetIterator* it){
    return it->currVertex;
};

void vertexSetIterator_reset(vertexSetIterator* it){
    if (it->isdense) {
        it->wordIdx = 0;
        it->wordShift = 0;
        it->word = 0;
    } else {
        it->sparseIdx = 0;
    } 
};

void vertexSetIterator_Deinit(vertexSetIterator* it) {
};
