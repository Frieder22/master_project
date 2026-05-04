#if !defined(TESTSTRUCTURE)
#define TESTSTRUCTURE

#include "vertexSet.h"
#include <stdlib.h>

typedef void (*vsFunc) (Vertexset*, int);

enum Testcase{
    ITERATOR,
    UNION_BUTTERFLY,
    UNION_MOSTLY_HALFING,
    NAIVE,
    ALLREDUCE_BUTTERFLY,
    ALLREDUCE_BUTTERFLY_REVERSE,
    ALLREDUCE_SHIFT,
    VERTEXSET_TEST,
    ALLGATHER_BUTTERFLY,
    ALLGATHER_BUTTERFLY_REVERSE,
    ALLGATHER_SHIFT
};

vsFunc getVertexsetFunc(int funcname){
    switch (funcname) {
    case UNION_BUTTERFLY:
        return Vertexset_Union_Butterfly;
    case UNION_MOSTLY_HALFING:
        return Vertexset_Union_Shift;
    case NAIVE:
        return Vertexset_Allreduce_Naive;
    case ALLREDUCE_BUTTERFLY:
        return Vertexset_Allreduce_Butterfly;
    case ALLREDUCE_BUTTERFLY_REVERSE:
        return Vertexset_Allreduce_Butterfly_ReverseBits;
    case ALLREDUCE_SHIFT:
        return Vertexset_Allreduce_Shift;
    case ALLGATHER_BUTTERFLY:
        return Vertexset_Allgather_Butterfly;
    case ALLGATHER_BUTTERFLY_REVERSE:
        return Vertexset_Allgather_Butterfly_ReverseBits;
    case ALLGATHER_SHIFT:
        return Vertexset_Allgather_Shift;
    default:
        exit(1);
    }
}

/**
 * returns true, if works for any number of processors 
 */
bool getCommPattern(int funcname){
    switch (funcname) {
    case UNION_BUTTERFLY:
        return false;
    case UNION_MOSTLY_HALFING:
        return true;
    case ALLREDUCE_BUTTERFLY:
        return false;
    case ALLREDUCE_BUTTERFLY_REVERSE:
        return false;
    case ALLREDUCE_SHIFT:
        return true;
    case ALLGATHER_BUTTERFLY:
        return false;
    case ALLGATHER_BUTTERFLY_REVERSE:
        return false;
    case ALLGATHER_SHIFT:
        return true;
    default:
        exit(1);
    }
}

#endif // TESTSTRUCTURE

