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
    } else {
        // setting for sparse representation
        it->sparseIdx = 0;
    } 
};

bool vertexSetIterator_next(vertexSetIterator* it){
    if (it->isdense) {
        size_t wordIx;
        uint64_t word;
        for (wordIx = it->wordIdx; wordIx < it->vs->size_bitarray; wordIx++) {
            word = it->vs->bitArray[wordIx];
            for (size_t i = 0; i < 64; i++) {
                /* code */
            }
            
        }
        
    }
    
};

void vertexSetIterator_reset(vertexSetIterator* it);

void vertexSetIterator_Deinit(vertexSetIterator* it);
